/* Getitnowmarketing added to define partions in roots.c */
/* define in boardconfig.mk as PARTITION_LAYOUT := PARTITION_LAYOUT_VIGOR */


#ifdef PARTITION_LAYOUT_DEFAULT

/* Layout for MECHA, VIVOW, LEXIKON etc */
/* No Longer used for HTC as partitions are dynamically detected via g_mmc_device */
/*
#define BOOTBLK "/dev/block/mmcblk0p22"
#define CACHEBLK "/dev/block/mmcblk0p27"
#define DATABLK "/dev/block/mmcblk0p26"
#define MISCBLK "/dev/block/mmcblk0p17"
#define RECOVERYBLK "/dev/block/mmcblk0p21"
#define SYSTEMBLK "/dev/block/mmcblk0p25"
*/
#endif

#ifdef PARTITION_LAYOUT_VIGOR

/* Layout for HTC VIGOR */
/* No Longer used for HTC as partitions are dynamically detected via g_mmc_device */
/*
#define BOOTBLK "/dev/block/mmcblk0p22"
#define CACHEBLK "/dev/block/mmcblk0p36"
#define DATABLK "/dev/block/mmcblk0p35"
#define MISCBLK "/dev/block/mmcblk0p24"
#define RECOVERYBLK "/dev/block/mmcblk0p23"
#define SYSTEMBLK "/dev/block/mmcblk0p29"
*/
#define INTERNALSDBLK "/dev/block/mmcblk0p37"

#endif

#ifdef PARTITION_LAYOUT_PYRAMID

/* Layout for HTC PYRAMID */
/* No Longer used for HTC as partitions are dynamically detected via g_mmc_device */
/*
#define BOOTBLK "/dev/block/mmcblk0p20"
#define CACHEBLK "/dev/block/mmcblk0p24"
#define DATABLK "/dev/block/mmcblk0p23"
#define MISCBLK "/dev/block/mmcblk0p31"
#define RECOVERYBLK "/dev/block/mmcblk0p21"
#define SYSTEMBLK "/dev/block/mmcblk0p22"
*/
#endif

#ifdef PARTITION_LAYOUT_SHOOTER

/* Layout for HTC SHOOTER */
/* No Longer used for HTC as partitions are dynamically detected via g_mmc_device */
/*
#define BOOTBLK "/dev/block/mmcblk0p21"
#define CACHEBLK "/dev/block/mmcblk0p25"
#define DATABLK "/dev/block/mmcblk0p24"
#define MISCBLK "/dev/block/mmcblk0p34"
#define RECOVERYBLK "/dev/block/mmcblk0p22"
#define SYSTEMBLK "/dev/block/mmcblk0p23"
*/
#endif

#ifdef PARTITION_LAYOUT_LGE_BRYCE

#define INTERNALSDBLK "/dev/block/mmcblk0p37"

#endif


