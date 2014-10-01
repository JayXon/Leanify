LEANIFY_SRC     := $(wildcard *.cpp formats/*.cpp)
LZMA_SRC        := formats/LZMA/Alloc.c formats/LZMA/LzFind.c formats/LZMA/LzmaDec.c formats/LZMA/LzmaEnc.c formats/LZMA/LzmaLib.c
MINIZ_SRC       := formats/miniz/miniz.c
MOZJPEG_SRC     := formats/mozjpeg/jaricom.c formats/mozjpeg/jcapimin.c formats/mozjpeg/jcarith.c formats/mozjpeg/jchuff.c formats/mozjpeg/jcmarker.c formats/mozjpeg/jcmaster.c formats/mozjpeg/jcomapi.c formats/mozjpeg/jcparam.c formats/mozjpeg/jcphuff.c formats/mozjpeg/jctrans.c formats/mozjpeg/jdapimin.c formats/mozjpeg/jdarith.c formats/mozjpeg/jdatadst.c formats/mozjpeg/jdatasrc.c formats/mozjpeg/jdcoefct.c formats/mozjpeg/jdcolor.c formats/mozjpeg/jddctmgr.c formats/mozjpeg/jdhuff.c formats/mozjpeg/jdinput.c formats/mozjpeg/jdmainct.c formats/mozjpeg/jdmarker.c formats/mozjpeg/jdmaster.c formats/mozjpeg/jdphuff.c formats/mozjpeg/jdpostct.c formats/mozjpeg/jdsample.c formats/mozjpeg/jdtrans.c formats/mozjpeg/jerror.c formats/mozjpeg/jidctflt.c formats/mozjpeg/jidctint.c formats/mozjpeg/jidctred.c formats/mozjpeg/jmemmgr.c formats/mozjpeg/jmemnobs.c formats/mozjpeg/jquant1.c formats/mozjpeg/jquant2.c formats/mozjpeg/jsimd_none.c formats/mozjpeg/jutils.c
TINYXML_SRC     := formats/tinyxml2/tinyxml2.cpp
ZOPFLI_SRC      := formats/zopfli/hash.c formats/zopfli/squeeze.c formats/zopfli/gzip_container.c formats/zopfli/katajainen.c formats/zopfli/zopfli_lib.c formats/zopfli/cache.c formats/zopfli/zlib_container.c formats/zopfli/util.c formats/zopfli/tree.c formats/zopfli/deflate.c formats/zopfli/blocksplitter.c formats/zopfli/lz77.c
ZOPFLIPNG_SRC   := formats/zopflipng/lodepng/lodepng.cpp formats/zopflipng/lodepng/lodepng_util.cpp formats/zopflipng/zopflipng_lib.cc

CFLAGS      += -Wall -O3 -msse2 -mfpmath=sse
ifneq ($(CC),clang)
    CFLAGS  += -flto
endif

ifeq ($(shell uname -s),Darwin)
    LDFLAGS += -liconv
else
    LDFLAGS += -s
endif

.PHONY:     leanify clean

leanify:    lzma.a miniz.o mozjpeg.a tinyxml2.o zopfli.a zopflipng.a
	$(CXX) $(CFLAGS) --std=c++0x $(LEANIFY_SRC) $^ $(LDFLAGS) -o $@

miniz.o:    $(MINIZ_SRC)
	$(CC) $(CFLAGS) -Wno-strict-aliasing -c $?

tinyxml2.o: $(TINYXML_SRC)
	$(CXX) $(CFLAGS) -c $?

lzma.a:     $(LZMA_SRC)
	$(CC) $(CFLAGS) -D _7ZIP_ST -Wno-unused-but-set-variable -Wno-self-assign -Wno-unknown-warning-option -c $?
	ar rcs $@ $(patsubst formats/LZMA/%.c,%.o,$^)

mozjpeg.a:  $(MOZJPEG_SRC)
	$(CC) $(CFLAGS) -Wno-attributes -c $?
	ar rcs $@ $(patsubst formats/mozjpeg/%.c,%.o,$^)

zopfli.a:   $(ZOPFLI_SRC)
	$(CC) $(filter-out -flto -O3,$(CFLAGS)) -O2 -c $?
	ar rcs $@ $(patsubst formats/zopfli/%.c,%.o,$^)

zopflipng.a:$(ZOPFLIPNG_SRC)
	$(CXX) $(CFLAGS) -c $?
	ar rcs $@ lodepng.o lodepng_util.o zopflipng_lib.o

clean:
	rm -f *.o *.a leanify
