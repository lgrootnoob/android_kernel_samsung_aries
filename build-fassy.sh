#!/bin/bash
BUILDVERSION=notest-kk44-fassy-`date +%Y%m%d`
DATE_START=$(date +"%s")

make "cyanogenmod_fascinatemtd_defconfig"

KERNEL_DIR=`pwd`
MODULES_DIR=modules
make -j2
echo "KERNEL_DIR="$KERNEL_DIR
echo "MODULES_DIR="$MODULES_DIR

DATE_END=$(date +"%s")
DIFF=$(($DATE_END - $DATE_START))
echo "Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."
