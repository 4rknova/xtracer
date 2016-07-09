make clean-all && ./configure && clear && make all -j8 && for f in scene/*.scn; do ./bin/xtracer -res 800x800 $f; done
