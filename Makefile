LEANIFY_OBJ     := leanify.o main.o utils.o $(patsubst %.cpp,%.o,$(wildcard formats/*.cpp))
LZMA_OBJ        := lib/LZMA/Alloc.o lib/LZMA/LzFind.o lib/LZMA/LzFindMt.o lib/LZMA/LzFindOpt.o lib/LZMA/LzmaDec.o lib/LZMA/LzmaEnc.o lib/LZMA/Threads.o
MOZJPEG_OBJ     := lib/mozjpeg/jaricom.o lib/mozjpeg/jcapimin.o lib/mozjpeg/jcarith.o lib/mozjpeg/jcext.o lib/mozjpeg/jchuff.o lib/mozjpeg/jcmarker.o lib/mozjpeg/jcmaster.o lib/mozjpeg/jcomapi.o lib/mozjpeg/jcparam.o lib/mozjpeg/jcphuff.o lib/mozjpeg/jctrans.o lib/mozjpeg/jdapimin.o lib/mozjpeg/jdarith.o lib/mozjpeg/jdatadst.o lib/mozjpeg/jdatasrc.o lib/mozjpeg/jdcoefct.o lib/mozjpeg/jdhuff.o lib/mozjpeg/jdinput.o lib/mozjpeg/jdmarker.o lib/mozjpeg/jdphuff.o lib/mozjpeg/jdtrans.o lib/mozjpeg/jerror.o lib/mozjpeg/jmemmgr.o lib/mozjpeg/jmemnobs.o lib/mozjpeg/jsimd_none.o lib/mozjpeg/jutils.o
PUGIXML_OBJ     := lib/pugixml/pugixml.o
ZOPFLI_OBJ      := lib/zopfli/hash.o lib/zopfli/squeeze.o lib/zopfli/gzip_container.o lib/zopfli/katajainen.o lib/zopfli/zopfli_lib.o lib/zopfli/cache.o lib/zopfli/zlib_container.o lib/zopfli/util.o lib/zopfli/tree.o lib/zopfli/deflate.o lib/zopfli/blocksplitter.o lib/zopfli/lz77.o
ZOPFLIPNG_OBJ   := lib/zopflipng/lodepng/lodepng.o lib/zopflipng/lodepng/lodepng_util.o lib/zopflipng/zopflipng_lib.o

CFLAGS      += -Wall -Werror -O3 -flto
CPPFLAGS    += -I./lib
CXXFLAGS    += $(CFLAGS) -std=c++17 -fno-rtti
LDFLAGS     += -flto -lpthread

ifeq ($(OS), Windows_NT)
    SYSTEM  := Windows
    LDLIBS  += -lshlwapi
else
    SYSTEM  := $(shell uname -s)
endif

ifeq ($(SYSTEM), Darwin)
    LDLIBS  += -liconv
else
    # -s is "obsolete" on mac
    LDFLAGS += -s
endif

ifeq ($(SYSTEM), Windows)
    LEANIFY_OBJ += fileio_win.o
else
    LEANIFY_OBJ += fileio_linux.o
endif

.PHONY:     leanify asan clean

leanify:    $(LEANIFY_OBJ) $(LZMA_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(LEANIFY_OBJ): CFLAGS += -Wextra -Wno-unused-parameter

$(LZMA_OBJ):    CFLAGS += -Wno-unknown-warning-option -Wno-dangling-pointer

$(ZOPFLI_OBJ):  CFLAGS += -Wno-unused-function

asan: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
asan: LDFLAGS := -fsanitize=address $(filter-out -s,$(LDFLAGS))
asan: leanify

clean:
	rm -f $(LEANIFY_OBJ) $(LZMA_OBJ) $(MOZJPEG_OBJ) $(PUGIXML_OBJ) $(ZOPFLI_OBJ) $(ZOPFLIPNG_OBJ) leanify
