#!/bin/bash

source initrd.sh 
qemu-system-x86_64 -kernel kernel -initrd initrd.img
