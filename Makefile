ALL_SRCS := $(shell find . -name '*.cpp' -o -name '*.cc' -o -name '*.c')
BIN_SRCS := ./formats/zopflipng/zopflipng_bin.cc ./formats/zopfli/zopfli_bin.c
SRCS     := $(filter-out $(BIN_SRCS), $(ALL_SRCS))
TARGET   := leanify
# no -Wall here because miniz will generate a lot of warning
CXXFLAGS := --std=c++0x -Wno-multichar -O2

default: all

all:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f leanify
