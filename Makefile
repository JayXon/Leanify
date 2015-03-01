LEANIFY_SRC     := $(wildcard *.cpp formats/*.cpp)
LZMA_SRC        := lib/LZMA/Alloc.c lib/LZMA/LzFind.c lib/LZMA/LzmaDec.c lib/LZMA/LzmaEnc.c lib/LZMA/LzmaLib.c
MINIZ_SRC       := lib/miniz/miniz.c
MOZJPEG_SRC     := lib/mozjpeg/jaricom.c lib/mozjpeg/jcapimin.c lib/mozjpeg/jcarith.c lib/mozjpeg/jcext.c lib/mozjpeg/jchuff.c lib/mozjpeg/jcmarker.c lib/mozjpeg/jcmaster.c lib/mozjpeg/jcomapi.c lib/mozjpeg/jcparam.c lib/mozjpeg/jcphuff.c lib/mozjpeg/jctrans.c lib/mozjpeg/jdapimin.c lib/mozjpeg/jdarith.c lib/mozjpeg/jdatadst.c lib/mozjpeg/jdatasrc.c lib/mozjpeg/jdcoefct.c lib/mozjpeg/jdcolor.c lib/mozjpeg/jddctmgr.c lib/mozjpeg/jdhuff.c lib/mozjpeg/jdinput.c lib/mozjpeg/jdmainct.c lib/mozjpeg/jdmarker.c lib/mozjpeg/jdmaster.c lib/mozjpeg/jdphuff.c lib/mozjpeg/jdpostct.c lib/mozjpeg/jdsample.c lib/mozjpeg/jdtrans.c lib/mozjpeg/jerror.c lib/mozjpeg/jidctflt.c lib/mozjpeg/jidctint.c lib/mozjpeg/jidctred.c lib/mozjpeg/jmemmgr.c lib/mozjpeg/jmemnobs.c lib/mozjpeg/jsimd_none.c lib/mozjpeg/jutils.c
TINYXML_SRC     := lib/tinyxml2/tinyxml2.cpp
ZOPFLI_SRC      := lib/zopfli/hash.c lib/zopfli/squeeze.c lib/zopfli/gzip_container.c lib/zopfli/katajainen.c lib/zopfli/zopfli_lib.c lib/zopfli/cache.c lib/zopfli/zlib_container.c lib/zopfli/util.c lib/zopfli/tree.c lib/zopfli/deflate.c lib/zopfli/blocksplitter.c lib/zopfli/lz77.c
ZOPFLIPNG_SRC   := lib/zopflipng/lodepng/lodepng.cpp lib/zopflipng/lodepng/lodepng_util.cpp lib/zopflipng/zopflipng_lib.cc

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
	ar rcs $@ $(patsubst lib/LZMA/%.c,%.o,$^)

mozjpeg.a:  $(MOZJPEG_SRC)
	$(CC) $(CFLAGS) -c $?
	ar rcs $@ $(patsubst lib/mozjpeg/%.c,%.o,$^)

zopfli.a:   $(ZOPFLI_SRC)
	$(CC) $(filter-out -flto -O3,$(CFLAGS)) -O2 -c $?
	ar rcs $@ $(patsubst lib/zopfli/%.c,%.o,$^)

zopflipng.a:$(ZOPFLIPNG_SRC)
	$(CXX) $(CFLAGS) -c $?
	ar rcs $@ lodepng.o lodepng_util.o zopflipng_lib.o

clean:
	rm -f *.o *.a leanify
