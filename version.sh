#!/bin/sh
#
# Get version from header.
#
HEADER=version.h
SCRIPT=version.sh
if [ ! -f "$HEADER" ]; then
	echo "$SCRIPT: error: $HEADER does not exist" 1>&2
	exit 1
fi
PAT_PREFIX='^#define[[:space:]]+XOMBRERO_'
PAT_SUFFIX='[[:space:]]+[0-9]+$'
MAJOR=$(egrep "${PAT_PREFIX}MAJOR${PAT_SUFFIX}" $HEADER | awk '{print $3}')
MINOR=$(egrep "${PAT_PREFIX}MINOR${PAT_SUFFIX}" $HEADER | awk '{print $3}')
PATCH=$(egrep "${PAT_PREFIX}PATCH${PAT_SUFFIX}" $HEADER | awk '{print $3}')
if [ -z "$MAJOR" -o -z "$MINOR" -o -z "$PATCH" ]; then
	echo "$SCRIPT: error: unable to get version from $HEADER" 1>&2
	exit 1
fi
echo $MAJOR.$MINOR.$PATCH | tr -d '\n'
