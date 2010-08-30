#!/bin/sh
#xrandr --newmode 800x600 0 800 0 0 0 600 0 0 0
#xrandr --addmode default 800x600

# lower the resolution
xrandr --output default --mode 800x600
# add below your activity:
#$1
cd /home/olpc/Activities/Quake.activity/bin
./sdlquake -mem 32 -winsize 800 600
#go back to normal resolution mode
xrandr --output default --mode 1200x900
