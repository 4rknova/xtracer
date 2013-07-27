mkdir png

for i in $1.ppm; 
	do j=${i/ppm/png}; 
	echo $j;
	convert $i -depth 8 $j; 
	mv $j ./png/; 
done

ffmpeg -i ./png/%05d.png -vcodec libx264 -preset slow  -b:v 20000k -maxrate 20000k -bufsize 5000k -threads 0 -f mp4 output.mp4
ffmpeg -i output.mp4 -f mpeg output.mpg