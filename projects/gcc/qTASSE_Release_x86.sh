#!/bin/bash
export BUILD_CONF=Release
export BUILD_BITS=32
printf "==============================================\n"
printf " Building qTASSE (%s %s-bit)...\n" $BUILD_CONF $BUILD_BITS
printf "==============================================\n"

. bin/build.sh tasse-gui $BUILD_CONF $BUILD_BITS

printf "==============================================\n"
printf " All Done!\n"
printf "==============================================\n"
