CC := g++
CFLAGS := -std=c++17 -O2 -g
LDLIBS := -lgtest -lpthread -lm -ldl

SOURCES := $(wildcard *_test.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLES := $(SOURCES:.cpp=.out)

LIB := ../lib/libabc.a ../lib/aiger.o ../lib/libcadical.a
obj_dir = ../obj
src_dir = ../src
header = $(wildcard $(src_dir)/*.h)
# Project objects, without the program entry point.
obj = $(filter-out $(obj_dir)/main.o, $(wildcard $(obj_dir)/*.o))

.PHONY: all run preBuild cleanOutput clean

# Build every test binary and run it.
all: preBuild
	$(MAKE) run

run: $(EXECUTABLES)
	for t in $(EXECUTABLES); do ./$$t || exit 1; done
	$(MAKE) cleanOutput

%.out: %.o
	$(CC) -o $@ $< $(obj) $(LIB) $(LDLIBS)

%.o: %.cpp $(header)
	$(CC) $(CFLAGS) -c $< -o $@

preBuild:
	$(MAKE) -C .. objs

cleanOutput:
	rm -f abcToolAIG.aag abcToolAIG.aig

clean:
	rm -f *.o *.out
