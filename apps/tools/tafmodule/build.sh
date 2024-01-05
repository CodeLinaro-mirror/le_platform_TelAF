#!/bin/sh
# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "Enter target (sa515m or sa525m):"
read TARGET

if [ $TARGET == 'sa525m' ]
then
    echo "You chose sa525m"
        mkexe drvTool.cpp -d ./debug --cflags=-O2 --cflags=-fno-omit-frame-pointer \
        -X -std=c++11 -X -lstdc++ \
        -o ./bin/tafmodule -t sa525m -w ./ \
        -X "-O2" -C "-O2" \
        -l $LEGATO_ROOT/build/sa525m/framework/lib \
        -i $LEGATO_ROOT/build/sa525m/framework/include \
        -i $LEGATO_ROOT/framework/liblegato \
        -i $LEGATO_ROOT/framework/liblegato/linux \
        -i $TELAF_ROOT/include \
        -i ../deviceManager/ \
        --cflags=-DNO_LOG_CONTROL \
        --cxxflags=-DNO_LOG_CONTROL

elif [ $TARGET == 'sa515m' ]
then
    echo "You chose sa515m"
        mkexe drvTool.cpp -d ./debug --cflags=-O2 --cflags=-fno-omit-frame-pointer -C -march=armv7-a -C -mfloat-abi=hard -C -mfpu=neon \
        -L -mfloat-abi=hard -L -mfpu=neon -X -mfloat-abi=hard -X -mfpu=neon -X -march=armv7-a -X -mthumb -X -std=c++11 -X -lstdc++ \
        -o ./bin/tafmodule -t sa515m -w ./ \
        -l $LEGATO_ROOT/build/sa515m/framework/lib \
        -i $LEGATO_ROOT/build/sa515m/framework/include \
        -i $LEGATO_ROOT/framework/liblegato \
        -i $LEGATO_ROOT/framework/liblegato/linux \
        -i $TELAF_ROOT/include \
        -i ../deviceManager/ \
        --cflags=-DNO_LOG_CONTROL \
        --cxxflags=-DNO_LOG_CONTROL

else
    echo "Incorrect target"
fi
