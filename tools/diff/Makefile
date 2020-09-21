CFLAGS += -ILIB/xml/include
CFLAGS += -LLIB/xml/.libs -lxml2
CFLAGS += -ILIB/lzma
CFLAGS += -LLIB/lzma -llzma

srcfile = sw_config.c sw_package.c sw_sha256.c sw_crc32.c sw_bsdiff.c

swconfig: $(srcfile)
	@make -C LIB
	@gcc -o $@ $^ $(CFLAGS)

clean:
	@rm -rf swconfig pkgsrc
	@make -C LIB clean
