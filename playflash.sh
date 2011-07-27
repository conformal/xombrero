#!/bin/sh
# $xxxterm$
#
# This is an example script for playing flash videos. It requires the
# get_flash_video package to be installed on the system. You can copy this
# file to ~/.xxxterm/, and set
#   default_script = ~/.xxxterm/playflash.sh
# in ~/.xxxterm.conf. Remember to make this file executable.
#
# You may wish to add the following line to ~/.get_flash_videosrc
# to avoid accumulating files:
#   player = mplayer -loop 0 %s 2>/dev/null; rm -f %s

cd /var/tmp && get_flash_videos -p "$1"
