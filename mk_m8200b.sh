#! /bin/bash

make UIMAGE_COMPRESSION=none uImage -j
#make modules

#make meson8b_skt.dtd
#make meson8b_skt.dtb
make meson8b_m200_1G.dtd
make meson8b_m200_1G.dtb

#cd ../root/g18
#find .| cpio -o -H newc | gzip -9 > ../ramdisk.img

#rootfs.cpio -- original buildroot rootfs, busybox
#m8rootfs.cpio -- build from buildroot
ROOTFS="rootfs.cpio"

#cd ..
./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./${ROOTFS} --second ./arch/arm/boot/dts/amlogic/meson8b_m200_1G.dtb --output ./m8200boot.img
ls -l ./m8200boot.img
echo "m8200boot.img for m8 baby done"
