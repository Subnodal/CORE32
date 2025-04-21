#!/bin/bash

mkdir -p assembler/build runtime/build

gcc assembler/*.c -o assembler/build/coreasm
gcc runtime/*.c -o runtime/build/core32