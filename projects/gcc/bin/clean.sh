#!/bin/bash
CURRENT_DIR=$(pwd)
MAKE_CMD=make
printf "Cleaning %s (%s %s-bit)...\n" $1 $2 $3
cd $1
$MAKE_CMD clean MK_PROFILE=$2 MK_BITS=$3
cd $CURRENT_DIR
