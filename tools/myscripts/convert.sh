# convert * images imagemagick
# usage: ./convert.sh [source format destination format]
# ex: ./convert gif png
mogrify -format $2 *$1
