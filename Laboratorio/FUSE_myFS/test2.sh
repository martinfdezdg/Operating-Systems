#!/bin/bash

MPOINT="./mount-point"

rm -R -f temp
mkdir temp

echo "Copying fuseLib.c and myFS.h to my FS..."
cp src/fuseLib.c $MPOINT/
cp src/myFS.h $MPOINT/
echo "Copying fuseLib.c and myFS.h to temp..."
cp src/fuseLib.c temp/
cp src/myFS.h temp/

echo "Auditing virtual disk..."
./auditor-pi virtual-disk
echo "Looking for differences between copies of files..."
diff -q src/fuseLib.c $MPOINT/fuseLib.c;
diff -q src/myFS.h $MPOINT/myFS.h;

echo "Truncating 1 block (-) of fuseLib.c on my FS and on temp..."
truncate -s -4K $MPOINT/fuseLib.c
truncate -s -4K temp/fuseLib.c

echo "Auditing virtual disk..."
./auditor-pi virtual-disk
echo "Looking for differences between a file and its truncated..."
diff -q src/fuseLib.c $MPOINT/fuseLib.c;

echo "Copying MyFileSystem.c to my FS..."
cp src/MyFileSystem.c $MPOINT/

echo "Auditing virtual disk..."
./auditor-pi virtual-disk
echo "Looking for differences between copies of file..."
diff -q src/MyFileSystem.c $MPOINT/MyFileSystem.c;

echo "Truncating 1 block (+) of myFS.h on my FS and on temp..."
truncate -s +4K $MPOINT/myFS.h
truncate -s +4K temp/myFS.h

echo "Auditing virtual disk..."
./auditor-pi virtual-disk
echo "Looking for differences between a file and its truncated..."
diff -q src/myFS.h $MPOINT/myFS.h;