#!/bin/bash

set -eo pipefail

source /usr/local/lib/u-boot-rockchip/loader-common

echo "Doing this will overwrite data stored on SPI Flash"
echo "  and it will require that you use eMMC or SD"
echo "  as your boot device."
echo ""

if ! MTD=$(grep \"loader\" /proc/mtd | cut -d: -f1); then
    echo "loader partition on MTD is not found"
    exit 1
fi

version "/dev/${MTD/mtd/mtdblock}"
confirm

write_nand() {
    echo "Writing /dev/$MTD with content of $2"
    flash_erase "/dev/$MTD" 0 0
    nandwrite "/dev/$MTD" < "$2"
}

write_nand loader "$SPI_LOADER"

echo Done.
