#!/bin/bash

function usage()
{
	echo "append resources to resource image."
	echo "usage:"
	echo "sudo ./pack_resource <resources dir> <resource.img>"
}

function die()
{
	echo "Die:" $1
	exit -1
}

function pack_resource ()
{
	local TOOL=`realpath "$1"`
	local RESOURCES=`realpath "$2"`
	local IMAGE=$3
	local old_dir=$OLDPWD
	touch $IMAGE
	IMAGE=`realpath $IMAGE`
	if [ -d "$RESOURCES" ];then
		cd $RESOURCES
	fi
	$TOOL --pack --image=$IMAGE `find $RESOURCES -type f|sed "s#^$RESOURCES/##"` 
	if [ -d "$RESOURCES" ];then
		cd - > /dev/null
		export OLDPWD=$ori_dir
	fi
}

function append_resource ()
{
	local TOOL=`realpath "$1"`
	local RESOURCES=`realpath "$2"`
	local IMAGE=$3
	local TMP_DIR=.resource_tmp
	rm -r $TMP_DIR 2>/dev/null
	mkdir $TMP_DIR || die "failed to mkdir $TMP_DIR"
	if [ -f "$IMAGE" ];then
		echo "Unpacking old image($IMAGE):"
		$TOOL --unpack --verbose --image=$IMAGE $TMP_DIR 2>&1|grep entry|sed "s/^.*://"|xargs echo
	fi
	if [ -d "$RESOURCES" ];then
		cp -r $RESOURCES/* $TMP_DIR
	else
		cp -r $RESOURCES $TMP_DIR
	fi
	echo
	pack_resource $TOOL $TMP_DIR $IMAGE
	echo
	echo "Packed resources:"
	$TOOL --unpack --verbose --image=$IMAGE $TMP_DIR 2>&1|grep entry|sed "s/^.*://"|xargs echo
	rm -r $TMP_DIR 2>/dev/null
}

RESOURCES=$1
IMAGE=$2

if [ `id -u` -gt 0 ];then
	usage
	die
fi

if [ -z "$RESOURCES" ];then
	RESOURCES=resources
fi

if [ -z "$IMAGE" ];then
	IMAGE=resource.img
fi

echo "Append $RESOURCES to $IMAGE..."
echo

if [ ! -e "$RESOURCES" ];then
	die "resource not found $RESOURCES !"
fi

TOOL=`realpath resource_tool`
if [ ! -f $TOOL ];then
	die "tool not found $TOOL !"
fi

append_resource $TOOL $RESOURCES $IMAGE
