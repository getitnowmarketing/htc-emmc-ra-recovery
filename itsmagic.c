/* itsmagic.c 
 * Copyright 2011-2012 sc2k 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>

int itsmagic_main(int argc, char** argv)
{
	FILE* f;

	/* It's magic ;) */
	unsigned char data[] = {
		0x00, 0xFB, 0x30, 0x94,
		0x99, 0x01, 0x4F, 0x97,
		0x2E, 0x4C, 0x2B, 0xA5,
		0x18, 0x6B, 0xDD, 0x06
	};


	printf("Some magic trick by @sc2k on xda\n");
	printf("it tricks the bootloader to run any kernel you want.\n");
	printf("Thx @Acer for making this so easy..  why did you say the BL is locked if it isn't? :D\n\n");
	printf("USE AT YOUR OWN RISK!\n\n");

	f = fopen("/dev/block/mmcblk0p7", "wb");
	if (f == 0)
	{
		printf("Where is your APK partition? This will not work (sadly).. or are you running me as normal user instead of root?\n");
		return -1;
	}

	fseek(f, 0x84, SEEK_SET);
	fwrite(data, 1, 16, f);
	fwrite(data, 1, 16, f);
	fwrite(data, 1, 16, f);
	fwrite(data, 1, 16, f);
	fflush(f);  
	fclose(f);

	printf("Done. Have fun!\n");

	return 0;
}
