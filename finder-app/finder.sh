#!/bin/sh
# Finder.sh script to find stuff
# Author: Spencer Manning

if [ $# -ne 2 ]
    then
        echo "Incorrect arguments specified"
        exit 1
fi

filesdir=$1
searchstr=$2

if [ -d "$filesdir" ]
    then
        echo "$filesdir is a directory"
    else
        echo "$filesdir is not a directory"
        exit 1
fi

# Find number of files in directory
filecnt=$(find "$filesdir"/* | wc -l)

# Find all matches
matchstr=$(grep -r "$filesdir" -e "$searchstr" | wc -l)

echo "The number of files are ${filecnt} and the number of matching lines are $matchstr"
