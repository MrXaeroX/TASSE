#!/bin/bash
CURRENT_DIR=$(pwd)
MAKE_CMD=make
printf "Installing %s (%s %s-bit)...\n" $1 $2 $3
cd $1
$MAKE_CMD install MK_PROFILE=$2 MK_BITS=$3
cd $CURRENT_DIR
