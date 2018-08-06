#!/bin/bash

set -eo pipefail

source /usr/local/lib/u-boot-rockchip/loader-common

echo "Doing this will overwrite data stored on SPI Flash"
echo "  and it will require that you use eMMC or SD"
echo "  as your boot device."
echo ""

confirm

MNT_DEV=$(findmnt /boot/efi -n -o SOURCE)

write_nand() {
    if ! MTD=$(grep \"$1\" /proc/mtd | cut -d: -f1); then
        echo "$1 partition on MTD is not found"
        return 1
    fi

    echo "Writing /dev/$MTD with content of $2"
    flash_erase "/dev/$MTD" 0 0
    nandwrite "/dev/$MTD" < "$2"
}

write_nand loader "$SPI_LOADER"

echo Done.
