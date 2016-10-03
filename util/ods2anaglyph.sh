#!/bin/bash

convert -crop 100%x50% $1 cropped_%d.png
composite -stereo 0:0 cropped_0.png cropped_1.png anaglyph.png
#compare -metric mae cropped_0.png cropped_1.png test.png
