#!/bin/bash
export BUILD_CONF=Release
export BUILD_BITS=64
printf "==============================================\n"
printf " Building TASSE (%s %s-bit)...\n" $BUILD_CONF $BUILD_BITS
printf "==============================================\n"

. bin/build.sh tasse-con $BUILD_CONF $BUILD_BITS

printf "==============================================\n"
printf " Installing TASSE (%s %s-bit)...\n" $BUILD_CONF $BUILD_BITS
printf "==============================================\n"

. bin/install.sh tasse-con $BUILD_CONF $BUILD_BITS

printf "==============================================\n"
printf " All Done!\n"
printf "==============================================\n"
