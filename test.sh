make clean-all && ./configure && clear && make all -j8 && for f in scene/basic/* ; do ./bin/xtracer $f; done
