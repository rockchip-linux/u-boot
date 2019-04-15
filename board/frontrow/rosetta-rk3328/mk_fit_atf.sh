#!/bin/sh
#
# script to generate FIT image source for rk3399 boards with
# ARM Trusted Firmware and multiple device trees (given on the command line)
#
# usage: $0 <dt_name> [<dt_name> [<dt_name] ...]

[ -z "$BL31" ] && BL31="bl31.elf"

if [ ! -f $BL31 ]; then
	echo "WARNING: BL31 file $BL31 NOT found, resulting binary is non-functional" >&2
	BL31=/dev/null
fi

cat << __HEADER_EOF
/dts-v1/;

/ {
	description = "Configuration to load ATF before U-Boot";
	#address-cells = <1>;

	images {
		uboot@1 {
			description = "U-Boot (64-bit)";
			data = /incbin/("u-boot-nodtb.bin");
			type = "standalone";
			arch = "arm64";
			compression = "none";
			load = <0x00200000>;
		};
__HEADER_EOF

atf_cnt=1

for l in `readelf -l $BL31 | grep -A1 LOAD | gawk --non-decimal-data \
	'{if (NR % 2) {printf "%d:0x%x:", $2,$4} else {printf "%d\n", $1}}'`
do
	offset=${l%%:*}
	ll=${l#*:}
	phy_offset=${ll%:*}
	filesz=${ll##*:}

	#echo "$offset/$phy_offset/$filesz"

	of=bl31_${phy_offset}.bin
	dd if=$BL31 of=$of bs=1 skip=$offset count=$filesz

	out_string="${out_string}:${phy_offset}"

	cat << __ATF1_EOF
		atf@$atf_cnt {
			description = "ARM Trusted Firmware";
			data = /incbin/("$of");
			type = "firmware";
			arch = "arm64";
			compression = "none";
			load = <$phy_offset>;
__ATF1_EOF
	if [ "$atf_cnt" -eq 1 ]; then
		cat << __ATF2_EOF
			entry = <$phy_offset>;
__ATF2_EOF
		fi
	cat << __ATF3_EOF
		};
__ATF3_EOF
	atf_cnt=$((atf_cnt + 1))
done

cnt=1
for dtname in $*
do
	cat << __FDT_IMAGE_EOF
		fdt@$cnt {
			description = "$(basename $dtname .dtb)";
			data = /incbin/("$dtname");
			type = "flat_dt";
			compression = "none";
		};
__FDT_IMAGE_EOF
	cnt=$((cnt+1))
done

cat << __CONF_HEADER_EOF
	};
	configurations {
		default = "config@1";

__CONF_HEADER_EOF

cnt=1
for dtname in $*
do
	cat << __CONF_SECTION_EOF
		config@$cnt {
			description = "$(basename $dtname .dtb)";
			firmware = "uboot@1";
			loadables = "atf@1","atf@2";
			fdt = "fdt@1";
		};
__CONF_SECTION_EOF
	cnt=$((cnt+1))
done

cat << __ITS_EOF
	};
};
__ITS_EOF
