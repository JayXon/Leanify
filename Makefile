LEANIFY_SRC     := leanify.cpp main.cpp utils.cpp $(wildcard formats/*.cpp)
LZMA_OBJ        := lib/LZMA/Alloc.o lib/LZMA/LzFind.o lib/LZMA/LzmaDec.o lib/LZMA/LzmaEnc.o
MOZJPEG_OBJ     := lib/mozjpeg/jaricom.o lib/mozjpeg/jcapimin.o lib/mozjpeg/jcarith.o lib/mozjpeg/jcext.o lib/mozjpeg/jchuff.o lib/mozjpeg/jcmarker.o lib/mozjpeg/jcmaster.o lib/mozjpeg/jcomapi.o lib/mozjpeg/jcparam.o lib/mozjpeg/jcphuff.o lib/mozjpeg/jctrans.o lib/mozjpeg/jdapimin.o lib/mozjpeg/jdarith.o lib/mozjpeg/jdatadst.o lib/mozjpeg/jdatasrc.o lib/mozjpeg/jdcoefct.o lib/mozjpeg/jdhuff.o lib/mozjpeg/jdinput.o lib/mozjpeg/jdmarker.o lib/mozjpeg/jdphuff.o lib/mozjpeg/jdtrans.o lib/mozjpeg/jerror.o lib/mozjpeg/jmemmgr.o lib/mozjpeg/jmemnobs.o lib/mozjpeg/jsimd_none.o lib/mozjpeg/jutils.o
PUGIXML_OBJ     := lib/pugixml/pugixml.o
ZOPFLI_OBJ      := lib/zopfli/hash.o lib/zopfli/squeeze.o lib/zopfli/gzip_container.o lib/zopfli/katajainen.o lib/zopfli/zopfli_lib.o lib/zopfli/cache.o lib/zopfli/zlib_container.o lib/zopfli/util.o lib/zopfli/tree.o lib/zopfli/deflate.o lib/zopfli/blocksplitter.o lib/zopfli/lz77.o
ZOPFLIPNG_OBJ   := lib/zopflipng/lodepng/lodepng.o lib/zopflipng/lodepng/lodepng_util.o lib/zopflipng/zopflipng_lib.o

CFLAGS      += -Wall -O3 -msse2 -mfpmath=sse -fno-exceptions -flto
CPPFLAGS    += -I./lib
CXXFLAGS    += $(CFLAGS) -std=c++14 -fno-rtti
LDFLAGS     += -flto

ifeq ($(OS), Windows_NT)
    SYSTEM  := Windows
else
    SYSTEM  := $(shell uname -s)
endif

# Gold linker only supports Linux
ifeq ($(SYSTEM), Linux)
    LDFLAGS += -fuse-ld=gold
endif

ifeq ($(SYSTEM), Darwin)
    LDLIBS  += -liconv
else
    # -s is "obsolete" on mac
    LDFLAGS += -s
endif

# Multithread in LZMA SDK is only supported on Windows
ifeq ($(SYSTEM), Windows)
    LEANIFY_SRC += fileio_win.cpp
    LZMA_OBJ    += lib/LZMA/LzFindMt.o lib/LZMA/Threads.o
else
    LEANIFY_SRC += fileio_linux.cpp
    LZMA_CFLAGS := -D _7ZIP_ST
endif

.PHONY:     leanify clean

leanify:    $(LEANIFY_SRC) $(LZMA_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(LZMA_OBJ):    CFLAGS += $(LZMA_CFLAGS)

$(ZOPFLI_OBJ):  CFLAGS += -Wno-unused-function

clean:
	rm -f $(LZMA_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ) leanify
