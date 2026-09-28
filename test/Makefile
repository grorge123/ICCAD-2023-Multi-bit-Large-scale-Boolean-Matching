CC := g++
CFLAGS := -std=c++17 -lgtest -lpthread

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLES := $(SOURCES:.cpp=.out)
LIB := ../lib/libabc.a ../lib/aiger.o
LINK := -lm -ldl -lreadline -ltinfo
obj_dir = ../obj
src_dir = ../src
obj = $(wildcard $(obj_dir)/*.o)
obj := $(filter-out ../obj/main.o, $(obj))
header = $(wildcard $(src_dir)/*.h)

all: $(EXECUTABLES)

%.out: %.o preBuild
	$(CC) -o $@ $(patsubst %.out, %.o, $@) $(obj) $(LIB) $(CFLAGS) $(LINK)
	./$@

$(EXECUTABLES): $(OBJECTS) preBuild
	$(CC) -o $@ $(patsubst %.out, %.o, $@) $(obj) $(LIB) $(CFLAGS) $(LINK)
	./$@
	$(MAKE) cleanOutput


%.o : %.cpp $(header)
	$(CC) -c $@ $< $(CFLAGS)

preBuild:
	make -C "../" test

cleanOutput:
	rm -f abcToolAIG.aag abcToolAIG.aig
clean:
	rm -f *.o *.out