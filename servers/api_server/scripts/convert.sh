#! /bin/bash

WIDTH=$1
HEIGHT=$2
FILENAME="$3"
OUT_DIR="$4"

# if width > height, set width to height
if [ "$WIDTH" -ge "$HEIGHT" ]; then
    WIDTH=$((HEIGHT-1))
fi

echo "Width: $WIDTH"
echo "Height: $HEIGHT"

echo "Clearing output directory"
rm -f "$OUT_DIR"/*

echo "Converting PS -> JPEG"

gs -sDEVICE=jpeg \
-dNOPAUSE \
-dBATCH \
-dSAFER \
-dFIXEDMEDIA \
-dFIXEDRESOLUTION \
-dFitPage \
-dDEVICEWIDTHPOINTS="$WIDTH" \
-dDEVICEHEIGHTPOINTS="$HEIGHT" \
-sOutputFile="$OUT_DIR/%d.jpeg" \
-dAutoRotatePages=/None \
-f "$FILENAME"