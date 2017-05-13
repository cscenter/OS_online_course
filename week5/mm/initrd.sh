#!/bin/sh

DIR=initramfs
INITRD=$(pwd)/initrd.img

find $DIR -print0 | cpio --null -ov --format=newc > $INITRD
