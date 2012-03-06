/* 
 * misctool - program to read and write main version in misc partition on emmc based HTC devices
 *
 * Copyright (C) 2012 Dustin Knight - <getitnowmarketing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * opinion) any later version. See <http://www.gnu.org/licenses/gpl.html>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void usage()
{
	printf(
		"Usage:\n"
		" misctool <option> <value to write>\n"
		"<option>\n"
		" r: read main version from misc partition. No <value to write> needed\n"
		" w: write new value to main version in misc partition & read back new value\n"
		"<value to write>\n"
		" must have a length of 10 in format of 1.00.000.0\n\n"
		);
}		

int read_proc_emmc(char *device)
{

	char dev[256];
	char size[256];
	char erasesize[256];
	char name[256];
	char line[1024];
	char miscblktmp[11];
	
	FILE* pe = fopen("/proc/emmc", "r");

	if (pe == NULL) {
		printf("Error opening /proc/emmc\n");
		return -1;
	}

	while(fgets(line, sizeof(line), pe)) {
        line[strlen(line)-1] = '\0';

	// 4th field is enclosed in quotes so remove them
        sscanf(line, "%255s %255s %255s \"%[^\"]\n", dev, size, erasesize, name);

         	 if (!strcmp(name, "misc")) {
            		// trim off the : from mmcblk0pxx:
			strncpy(miscblktmp, dev, 10);
			miscblktmp[10]='\0';
		 }
	}

	fclose(pe);

	//printf("Misc partition found on %s\n", miscblktmp);
	strcpy (device, miscblktmp);

	return 0;
}

int read_misc_block(char *main_version)
{

	char device[65];
	int ret = read_proc_emmc(device);
	if (ret == 0) {
	
		char maintmp[12];
		char misc_device[128];
		sprintf(misc_device, "/dev/block/%s", device);
		FILE* misc = fopen(misc_device, "rb");

		if (misc == NULL) {
			printf("Error opening misc partition... must be run as root\n");
			return -1;
		}

		fseek(misc, 0xA0, SEEK_SET);
		fgets(maintmp, 12, misc);

		if (maintmp == NULL) {
			printf("Error reading main version from misc partition...\n");
			return -1;
		}

		fclose(misc);
		strcpy(main_version, maintmp);

		return 0;
	} else {
		return -1;
	}
}

int write_misc_block(char *write_version)
{

	char device[65];
	int ret = read_proc_emmc(device);
	if (ret == 0) {
	
		char maintmp[12];
		char misc_device[128];
		sprintf(misc_device, "/dev/block/%s", device);
		FILE* misc = fopen(misc_device, "wb");
	
		if (misc == NULL) {
			printf("Error opening misc partition... must be run as root\n");
			return -1;
		}
	
		fseek(misc, 0xA0, SEEK_SET);
		fwrite(write_version, 1, 10, misc);
		fflush(misc);  
		fclose(misc);

		return 0;
	} else {
		return -1;
	}
}
int misctool_main(int argc, char** argv)
{
	printf("\n");
	printf("=== misctool v1.0\n");
	printf("=== Read/Write HTC Main Version\n");
	printf("=== (c)2012 Getitnowmarketing Inc\n\n");

	if(argc !=2 && argc !=3) {
		usage();
		return 1;
	}

	struct stat st;
	if (0 !=stat("/proc/emmc", &st)) {
		printf("Error: Must be run on an emmc based htc\n");
		return -1;
	}

	switch(*argv[1]) {
		case 'r': ;
			// read main version from misc partition
			
			char main_version[12];
			int ret = read_misc_block(main_version);

			if (ret == 0) {
				printf("Main Version is %s\n", main_version);
				/* 
				printf("Main Version in hex is %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n\n", main_version[0], main_version[1], 
				main_version[2], main_version[3], main_version[4], main_version[5], main_version[6], main_version[7], main_version[8],
				main_version[9]);
				*/
			} else {
				return -1;
			}

			break;

		case 'w': ;
			// write new value to main version in misc partition

			if(argc !=3) {
				usage();
				return 1;
			}
			
			size_t size;
			size = strlen(argv[2]);
			if (size != 10){
				printf("Error: Value to write must be 10 characters in the form of ex. 4.08.605.3\n");
				 return -1;
			}

			write_misc_block(argv[2]);

			char post_write[12];
			int retval = read_misc_block(post_write);

			if (retval == 0) {
				printf("Read back of newly written Main Version is %s\n", post_write);
			} else {
				return -1;
			}

			break;

		default: ;
			usage();
			return 1;

	}

	printf("Done!!\n");
	return 0;
}

