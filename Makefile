LEANIFY_SRC     := $(wildcard *.cpp formats/*.cpp)
LZMA_OBJ        := lib/LZMA/Alloc.o lib/LZMA/LzFind.o lib/LZMA/LzmaDec.o lib/LZMA/LzmaEnc.o lib/LZMA/LzmaLib.o
MINIZ_OBJ       := lib/miniz/miniz.o
MOZJPEG_OBJ     := lib/mozjpeg/jaricom.o lib/mozjpeg/jcapimin.o lib/mozjpeg/jcarith.o lib/mozjpeg/jcext.o lib/mozjpeg/jchuff.o lib/mozjpeg/jcmarker.o lib/mozjpeg/jcmaster.o lib/mozjpeg/jcomapi.o lib/mozjpeg/jcparam.o lib/mozjpeg/jcphuff.o lib/mozjpeg/jctrans.o lib/mozjpeg/jdapimin.o lib/mozjpeg/jdarith.o lib/mozjpeg/jdatadst.o lib/mozjpeg/jdatasrc.o lib/mozjpeg/jdcoefct.o lib/mozjpeg/jdcolor.o lib/mozjpeg/jddctmgr.o lib/mozjpeg/jdhuff.o lib/mozjpeg/jdinput.o lib/mozjpeg/jdmainct.o lib/mozjpeg/jdmarker.o lib/mozjpeg/jdmaster.o lib/mozjpeg/jdphuff.o lib/mozjpeg/jdpostct.o lib/mozjpeg/jdsample.o lib/mozjpeg/jdtrans.o lib/mozjpeg/jerror.o lib/mozjpeg/jidctflt.o lib/mozjpeg/jidctint.o lib/mozjpeg/jidctred.o lib/mozjpeg/jmemmgr.o lib/mozjpeg/jmemnobs.o lib/mozjpeg/jsimd_none.o lib/mozjpeg/jutils.o
PUGIXML_OBJ     := lib/pugixml/pugixml.o
ZOPFLI_OBJ      := lib/zopfli/hash.o lib/zopfli/squeeze.o lib/zopfli/gzip_container.o lib/zopfli/katajainen.o lib/zopfli/zopfli_lib.o lib/zopfli/cache.o lib/zopfli/zlib_container.o lib/zopfli/util.o lib/zopfli/tree.o lib/zopfli/deflate.o lib/zopfli/blocksplitter.o lib/zopfli/lz77.o
ZOPFLIPNG_OBJ   := lib/zopflipng/lodepng/lodepng.o lib/zopflipng/lodepng/lodepng_util.o lib/zopflipng/zopflipng_lib.o

CFLAGS      += -Wall -O3 -msse2 -mfpmath=sse -flto -fuse-linker-plugin
CXXFLAGS    += --std=c++11 $(CFLAGS)
LDFLAGS     += -s
ifeq ($(shell uname -s),Darwin)
    LDFLAGS += -liconv
endif

.PHONY:     leanify clean

leanify:    $(LEANIFY_SRC) $(LZMA_OBJ) $(MINIZ_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(MINIZ_OBJ):   CFLAGS += -Wno-strict-aliasing

$(LZMA_OBJ):    CFLAGS += -D _7ZIP_ST

$(ZOPFLI_OBJ):  CFLAGS += -Wno-unused-function

clean:
	rm -f $(LZMA_OBJ) $(MINIZ_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ) leanify
