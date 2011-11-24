#!/sbin/sh
echo \#!/sbin/sh > /tmp/mkboot/createnewboot.sh
echo /sbin/mkbootimg --kernel /tmp/mkboot/zImage --ramdisk /tmp/mkboot/boot.img-ramdisk.gz --cmdline \"$(cat /tmp/mkboot/boot.img-cmdline)\" --base $(cat /tmp/mkboot/boot.img-base) --output /tmp/mkboot/newboot.img >> /tmp/mkboot/createnewboot.sh
chmod 777 /tmp/mkboot/createnewboot.sh
/tmp/mkboot/createnewboot.sh
return $?
