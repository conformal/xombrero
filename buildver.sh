#!/bin/sh
if [ -d .git ]; then
	git describe --tags | tr -d '\n'
fi
