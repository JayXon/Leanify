LEANIFY_SRC     := $(wildcard *.cpp formats/*.cpp)
LZMA_SRC        := ./formats/LZMA/Alloc.c ./formats/LZMA/LzFind.c ./formats/LZMA/LzmaDec.c ./formats/LZMA/LzmaEnc.c ./formats/LZMA/LzmaLib.c
MINIZ_SRC       := ./formats/miniz/miniz.c
MOZJPEG_SRC     := ./formats/mozjpeg/jaricom.c ./formats/mozjpeg/jcapimin.c ./formats/mozjpeg/jcarith.c ./formats/mozjpeg/jchuff.c ./formats/mozjpeg/jcmarker.c ./formats/mozjpeg/jcmaster.c ./formats/mozjpeg/jcomapi.c ./formats/mozjpeg/jcparam.c ./formats/mozjpeg/jcphuff.c ./formats/mozjpeg/jctrans.c ./formats/mozjpeg/jdapimin.c ./formats/mozjpeg/jdarith.c ./formats/mozjpeg/jdatadst.c ./formats/mozjpeg/jdatasrc.c ./formats/mozjpeg/jdcoefct.c ./formats/mozjpeg/jdcolor.c ./formats/mozjpeg/jddctmgr.c ./formats/mozjpeg/jdhuff.c ./formats/mozjpeg/jdinput.c ./formats/mozjpeg/jdmainct.c ./formats/mozjpeg/jdmarker.c ./formats/mozjpeg/jdmaster.c ./formats/mozjpeg/jdphuff.c ./formats/mozjpeg/jdpostct.c ./formats/mozjpeg/jdsample.c ./formats/mozjpeg/jdtrans.c ./formats/mozjpeg/jerror.c ./formats/mozjpeg/jidctflt.c ./formats/mozjpeg/jidctint.c ./formats/mozjpeg/jidctred.c ./formats/mozjpeg/jmemmgr.c ./formats/mozjpeg/jmemnobs.c ./formats/mozjpeg/jquant1.c ./formats/mozjpeg/jquant2.c ./formats/mozjpeg/jsimd_none.c ./formats/mozjpeg/jutils.c
TINYXML_SRC     := ./formats/tinyxml2/tinyxml2.cpp
ZOPFLI_SRC      := ./formats/zopfli/hash.c ./formats/zopfli/squeeze.c ./formats/zopfli/gzip_container.c ./formats/zopfli/katajainen.c ./formats/zopfli/zopfli_lib.c ./formats/zopfli/cache.c ./formats/zopfli/zlib_container.c ./formats/zopfli/util.c ./formats/zopfli/tree.c ./formats/zopfli/deflate.c ./formats/zopfli/blocksplitter.c ./formats/zopfli/lz77.c
ZOPFLIPNG_SRC   := ./formats/zopflipng/lodepng/lodepng.cpp ./formats/zopflipng/lodepng/lodepng_util.cpp ./formats/zopflipng/zopflipng_lib.cc
OBJS            := blocksplitter.o cache.o deflate.o gzip_container.o hash.o jaricom.o jcapimin.o jcarith.o jchuff.o jcmarker.o jcmaster.o jcomapi.o jcparam.o jcphuff.o jctrans.o jdapimin.o jdarith.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o jdmaster.o jdphuff.o jdpostct.o jdsample.o jdtrans.o jerror.o jidctflt.o jidctint.o jidctred.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jsimd_none.o jutils.o katajainen.o lodepng.o lodepng_util.o lz77.o miniz.o squeeze.o tinyxml2.o tree.o util.o zlib_container.o zopfli_lib.o zopflipng_lib.o Alloc.o LzFind.o LzmaDec.o LzmaEnc.o LzmaLib.o

CFLAGS      := -Wall -O3 -msse2 -mfpmath=sse

.PHONY:     leanify clean

leanify:    lzma miniz.o mozjpeg tinyxml2.o zopfli zopflipng
	$(CXX) $(CFLAGS) --std=c++0x -Wno-multichar -s $(OBJS) $(LEANIFY_SRC) -o $@

miniz.o:    $(MINIZ_SRC)
	$(CC) $(CFLAGS) -Wno-strict-aliasing -c $?

tinyxml2.o: $(TINYXML_SRC)
	$(CXX) $(CFLAGS) -c $?

lzma:       $(LZMA_SRC)
	$(CC) $(CFLAGS) -D _7ZIP_ST -Wno-unused-but-set-variable -c $?
	touch $@

mozjpeg:    $(MOZJPEG_SRC)
	$(CC) $(CFLAGS) -Wno-attributes -c $?
	touch $@

zopfli:     $(ZOPFLI_SRC)
	$(CC) $(CFLAGS:O3=O2) -c $?
	touch $@

zopflipng:  $(ZOPFLIPNG_SRC)
	$(CXX) $(CFLAGS) -c $?
	touch $@

clean:
	rm -f *.o leanify lzma mozjpeg zopfli zopflipng
