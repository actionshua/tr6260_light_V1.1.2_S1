RM = rm -f
CFLAGS = -c -D_7ZIP_ST 
CXX=gcc
OBJS = \
  Alloc.o \
  LzFind.o \
  LzmaDec.o \
  LzmaEnc.o \
  LzmaLib.o \


liblzma.a: $(OBJS)
	ar cr $@ $^ 

Alloc.o: Alloc.c
	$(CXX) $(CFLAGS) Alloc.c

LzFind.o: LzFind.c
	$(CXX) $(CFLAGS) LzFind.c

LzmaDec.o: LzmaDec.c
	$(CXX) $(CFLAGS) LzmaDec.c

LzmaEnc.o: LzmaEnc.c
	$(CXX) $(CFLAGS) LzmaEnc.c

clean:
	-$(RM) liblzma.a $(OBJS)
