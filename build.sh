#!/bin/sh
CC=cc
PRG=98tombr
set -e
set -x
$CC -o $PRG $PRG.c
