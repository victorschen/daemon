# 
#  daemon  Makefile
#


daemon:daemon.cpp
	/opt/mtwk/usr/local/powerpc-linux/gcc-3.4.3-glibc-2.3.3/bin/powerpc-linux-g++ -lxml2 -I/usr/include/libxml2 -o daemon daemon.cpp


clean:
	rm -f *.o