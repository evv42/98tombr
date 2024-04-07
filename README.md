# 98tombr - Shows PC-98 partition tables and writes an MBR equivalent for use on modern systems.

## What it does

98tombr can read a disk image or a block device that contains a PC-98 partition table and display its contents.  
It can also write a MBR (Master Boot Record) equivalent, which enables accessing the partitions on a modern operating system. The drive can still be used on a PC-98 after this operation.  

## How it works

On modern PCs, drives that use MBR have their partition table stored on the first sector, at address 0x1BE, just after the bootstrap code area.  
The PC-98 stores its partition table on the second sector of the hard drive, at address 0x200.  
This means that we can have both systems on one drive.  
  
Basically, a partition entry on both systems is defined by a partition type, and a CHS (Cylinder,Head,Sector) start and end.
But, MBR also have a LBA (Logical Block Addressing) start and size, that permits usage of big drives. Modern OSes uses that part for determining partition location and size instead of CHS.  
Most PC-9821s seems to use an LBA-like system, which uses the 16-bit cylinder fields and a translation of 1 cylinder = 136 blocks.  
  
With that, this programs translates the first four partitions, and converts the partition types using a correspondance table.  
The PC-98 partitions types have been reverse-engineered from a set of drives, images, and some translated info from here: http://bauxite.sakura.ne.jp/wiki/mypad.cgi?p=PC-98x1%2Fdisk%2F%B6%E8%B2%E8%BE%F0%CA%F3.  
The PC-98 partition table information comes from the FreeBSD 8 source code (https://cgit.freebsd.org/src/tree/sys/sys/diskpc98.h?h=stable/8).  

For further information, please read the source code, and the Wikipedia article on MBR.  

## Limitations

This program is equivalent to the PC-98 DOS program CONV98AT, except it doesn't do partition remapping.  
What it means is that the MBR 98tombr makes shows at most the first four PC-98 partitions.  
This program is probably not exempt of bugs, and currently does not have any checks for the input. Please make backups of your drives before using it, and double-check what you're typing.  

## How to build

You can use the provided build.sh script. It should work with 99% of UNIX systems.
On Windows, you can probably use cl if you have Visual Studio installed (not tested).
**WARNING: This program will not work on big-endian systems**

## How to use

Use ./98tombr -h for help.

## Copyright stuff

This program is licensed under the zlib/libpng license. See LICENSE file for details.



