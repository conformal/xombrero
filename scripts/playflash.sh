#!/bin/sh
# This is an example script for playing flash videos. It requires the
# get_flash_video package to be installed on the system. You can copy this
# file to ~/.xombrero/, and set
#   default_script = ~/.xombrero/playflash.sh
# in ~/.xombrero.conf. Remember to make this file executable.
#
# You may wish to add the following line to ~/.get_flash_videosrc
# to avoid accumulating files:
#   player = mplayer -loop 0 %s 2>/dev/null; rm -f %s

cd /var/tmp && get_flash_videos --player="/usr/local/bin/mplayer -really-quiet %s 2> /dev/null" -q -p -y $1
