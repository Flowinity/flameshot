#!/bin/sh

#==========================================
# Unlimited uploads per day, 10GB max per account.
#==========================================

URL="https://images.flowinity.com/api/v3/gallery/site"

if [ $# -eq 0 ]; then
    echo "Usage: file.io FILE [DURATION]\n"
    echo "Example: file.io path/to/my/file 1w\n"
    exit 1
fi

FILE=$1

if [ ! -f "$FILE" ]; then
    echo "File ${FILE} not found"
    exit 1
fi

RESPONSE=$(curl -# -F "file=@${FILE}" "${URL}/")

echo "${RESPONSE}" # to terminal
