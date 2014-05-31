#!/bin/sh

if [ `id -u` -gt 0 ];then
	echo "need sudo..."
	exit
fi

./tools/resource_tool/pack_resource.sh tools/resource_tool/resources/ ../kernel/resource.img resource.img tools/resource_tool/resource_tool

