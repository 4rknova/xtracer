#!/bin/sh

CURDIR="$(cd "$(dirname "$0")"; pwd)"
BINDIR=$CURDIR/bin
SRCDIR=$CURDIR/src
EXTENSION="fig"

echo "Generating postscript files from gnuplot figures.."

# check if the output directory exists
if [ ! -d "$BINDIR" ]; then
	echo "Creating directory $BINDIR .."
	mkdir "$BINDIR"
fi

if ls "$SRCDIR"/*.$EXTENSION > /dev/null 2>&1; then
	for i in "$SRCDIR"/*.$EXTENSION
	do
	    echo "Plotting ${i%%} .."
		BASENAME="$(basename "${i%%.*}")"
		FIGURE=`cat "$SRCDIR/$BASENAME.$EXTENSION"`
		printf "set terminal postscript \nset output '$BINDIR/$BASENAME' \ncd \"$SRCDIR\" \n$FIGURE" | gnuplot
	done
else
	echo "No figures to process.."
fi
