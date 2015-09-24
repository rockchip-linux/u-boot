#!/bin/bash

function usage()
{
	echo "append resources to resource image."
	echo "usage:"
	echo "./pack_resource <resources dir> <dst image> <old image> <resource_tool path>"
}

function die()
{
	echo "Die:" $1
	exit -1
}

function append_resource()
{
	local TOOL=$1
	local RESOURCES=$2
	local IMAGE=$3
	local OLD_IMAGE=$4
	local TMP_DIR=.resource_tmp
	rm -r $TMP_DIR 2>/dev/null
	mkdir $TMP_DIR || die "failed to mkdir $TMP_DIR"
	if [ -f "$OLD_IMAGE" ];then
		echo "Unpacking old image($OLD_IMAGE):"
		$TOOL --unpack --verbose --image=$OLD_IMAGE $TMP_DIR 2>&1|grep entry|sed "s/^.*://"|xargs echo
	fi
	if [ -d "$RESOURCES" ];then
		cp -r $RESOURCES/* $TMP_DIR
	else
		cp -r $RESOURCES $TMP_DIR
	fi
	echo
	$TOOL --pack --root=$TMP_DIR --image=$IMAGE `find $TMP_DIR -type f|sort`
	echo
	echo "Packed resources:"
	$TOOL --unpack --verbose --image=$IMAGE $TMP_DIR 2>&1|grep entry|sed "s/^.*://"|xargs echo
	rm -r $TMP_DIR 2>/dev/null
}

RESOURCES=$1
OLD_IMAGE=$2
IMAGE=$3
TOOL=$4

if [ -z "$RESOURCES" ];then
	RESOURCES=resources
fi

if [ -z "$IMAGE" ];then
	IMAGE=resource.img
fi

if [ -z "$OLD_IMAGE" ];then
    OLD_IMAGE=$IMAGE
fi

if [ -z "$TOOL" ];then
	TOOL=./resource_tool
fi

echo "Pack $RESOURCES & $OLD_IMAGE to $IMAGE ..."
echo

if [ ! -e "$RESOURCES" ];then
	die "resource not found $RESOURCES !"
fi

if [ ! -f $TOOL ];then
	die "tool not found $TOOL !"
fi

append_resource $TOOL $RESOURCES $IMAGE $OLD_IMAGE
