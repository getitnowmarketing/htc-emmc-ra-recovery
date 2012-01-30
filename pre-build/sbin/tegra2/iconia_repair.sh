#!/sbin/sh

# Written by getitnowmarketing@gmail.com 01/25/12 for use in iconia recovery

MKE2FS="/sbin/mke2fs"
TUNE2FS="/sbin/tune2fs"
E2FSCK="/sbin/e2fsck"
FLEXBLK="/dev/block/mmcblk0p6"
SYSBLK="/dev/block/mmcblk0p3"
CACHEBLK="/dev/block/mmcblk0p4"
DATABLK="/dev/block/mmcblk0p8"

repair_cache()
{
	echo "formatting & rebuilding cache as ext4"
	$MKE2FS -t ext3 -b 4096 $CACHEBLK
	$E2FSCK -fp $CACHEBLK
	$TUNE2FS -O extents,uninit_bg,dir_index $CACHEBLK
	$E2FSCK -fpDC0 $CACHEBLK
	echo "done"

}

repair_system()
{
	echo "formatting & rebuilding system as ext4"
	$MKE2FS -t ext3 -b 4096 $SYSBLK
	$E2FSCK -fp $SYSBLK
	$TUNE2FS -O extents,uninit_bg,dir_index $SYSBLK
	$E2FSCK -fpDC0 $SYSBLK
	echo "done"

}

repair_data()
{
	echo "formatting & rebuilding data as ext4"
	$MKE2FS -t ext3 -b 4096 $DATABLK
	$E2FSCK -fp $DATABLK
	$TUNE2FS -O extents,uninit_bg,dir_index $DATABLK
	$E2FSCK -fpDC0 $DATABLK
	echo "done"

}

repair_flexrom()
{
	echo "formatting & rebuilding flexrom as ext4"
	$MKE2FS -t ext3 -b 4096 $FLEXBLK
	$E2FSCK -fp $FLEXBLK
	$TUNE2FS -O extents,uninit_bg,dir_index $FLEXBLK
	$E2FSCK -fpDC0 $FLEXBLK
	echo "done"

}


case $1 in
	cache)
		CHECKCACHE=`mount | grep /cache`
		if [ "$CHECKCACHE" == "" ]; then
			repair_cache
		else
			umount /cache 2>/dev/null
			if [ "$CHECKCACHE" == "" ]; then
				repair_cache
			else
				echo "error unmounting $1"
				exit 1			
			fi		
		fi
	;;
	
	data)
		CHECKDATA=`mount | grep /data`
		if [ "$CHECKDATA" == "" ]; then
			repair_data
		else
			umount /data 2>/dev/null
			if [ "$CHECKDATA" == "" ]; then
				repair_data
			else
				echo "error unmounting $1"
				exit 1			
			fi		
		fi
	;;

	system)
		CHECKSYS=`mount | grep /system`
		if [ "$CHECKSYS" == "" ]; then
			repair_system
		else
			umount /system 2>/dev/null
			if [ "$CHECKSYS" == "" ]; then
				repair_system
			else
				echo "error unmounting $1"
				exit 1			
			fi		
		fi
	;;


	flexrom)
		CHECKFLEX=`mount | grep /flexrom`
		if [ "$CHECKFLEX" == "" ]; then
			repair_flexrom
		else
			umount /flexrom 2>/dev/null
			if [ "$CHECKFLEX" == "" ]; then
				repair_flexrom
			else
				echo "error unmounting $1"
				exit 1			
			fi		
		fi
	;;

	--)
		echo "please use iconia_repair.sh help for useage"
	;;

	help)
		echo "Useage is iconia_repair.sh partition_to_repair"
		echo "Available partitions : cache, data, system, flexrom"
		echo "Example use is: iconia_repair.sh cache"
	;;
esac


exit 0
