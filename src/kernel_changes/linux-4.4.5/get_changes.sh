#!/bin/bash

echo "Copying modfied Kernel source files"


find . -name \* -type f ! -path "*.sh" ! -path "*.txt" -print | cut -d'/' -f2- > differences.txt
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


cat differences.txt | while read LINE
do

DST_DIR=$SCRIPT_DIR$(dirname /$LINE )
echo Copying $LINE to $DST_DIR
mkdir -p $DST_DIR
cp -v /src/linux-4.4.5-insVT/$LINE $DST_DIR
done
