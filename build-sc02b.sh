#!/bin/bash
echo "Making KK44/CM-11 kernel for GS1 NTT-Docomo SC-02B"
BUILDVERSION=notest-kk44-sc02b-`date +%Y%m%d`-twrp
DATE_START=$(date +"%s")

make "cyanogenmod_galaxys_sc02b_defconfig"

KERNEL_DIR=`pwd`
OUTPUT_DIR=../output
CWM_DIR=../ramdisk-sc02b/stock/
MODULES_DIR=../ramdisk-sc02b/stock/system/lib/modules/

echo "KERNEL_DIR="$KERNEL_DIR
echo "OUTPUT_DIR="$OUTPUT_DIR
echo "CWM_DIR="$CWM_DIR
echo "MODULES_DIR="$MODULES_DIR

make modules

rm `echo $MODULES_DIR"/*"`
find $KERNEL_DIR -name '*.ko' -exec cp -v {} $MODULES_DIR \;
chmod 644 `echo $MODULES_DIR"/*"`

make zImage

cp arch/arm/boot/zImage $CWM_DIR"boot.img"
cd $CWM_DIR

zip -r `echo $BUILDVERSION`.zip *
mv  `echo $BUILDVERSION`.zip ../$OUTPUT_DIR"/"

DATE_END=$(date +"%s")
echo
DIFF=$(($DATE_END - $DATE_START))
echo "Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."
