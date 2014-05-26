#!/bin/bash

# usage: . ./pack_resource.sh --pack|--append resources/ resource.img
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
		cd -
		export OLDPWD=$ori_dir
	fi
}

function append_resource ()
{
	local TOOL=`realpath "$1"`
	local RESOURCES=`realpath "$2"`
	local IMAGE=$3
	local TMP_DIR=/tmp/resource_out
	sudo rm -r $TMP_DIR
	mkdir $TMP_DIR
	if [ -f "$IMAGE" ];then
		sudo $TOOL --unpack --image=$IMAGE $TMP_DIR 1>/dev/null 2>&1
	fi
	if [ -d "$RESOURCES" ];then
		cp -r $RESOURCES/* $TMP_DIR
	else
		cp -r $RESOURCES $TMP_DIR
	fi
	pack_resource $TOOL $TMP_DIR $IMAGE
}

PACK_OPT="--pack"
APPEND_OPT="--append"

TOOL=`realpath resource_tool`
if [ ! -f $TOOL ];then
	echo "tool not found $TOOL !"
	return;
fi

OPT=$1
RESOURCES=$2
IMAGE=$3

if [ ! -e "$RESOURCES" ];then
	echo "resource not found $RESOURCES !"
	exit
fi

if [ -z "$IMAGE" ];then
	IMAGE=resource.img
fi

append_resource $TOOL $RESOURCES $IMAGE
