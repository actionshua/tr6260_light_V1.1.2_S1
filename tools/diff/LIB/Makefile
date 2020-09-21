all:
	rm -rf xml
	tar -zxvf libxml2-2.7.1.tar.gz
	mv libxml2-2.7.1 xml
	cd xml;./configure;make
	cd lzma;make
clean:
	rm -rf xml
	cd lzma;make clean
