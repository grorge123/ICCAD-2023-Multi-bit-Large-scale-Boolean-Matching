CC := g++
exe := main
CFLAGS := -std=c++17 -O3 -Wall -Wextra -g
SANITIZE := -fsanitize=undefined -fsanitize=address
DEFINE := -DINF -DDBG
LINK := -lm -ldl
LIB := lib/libabc.a lib/aiger.o lib/libcadical.a

src_dir = ./src
obj_dir = ./obj

src = $(wildcard $(src_dir)/*.cpp)
header = $(wildcard $(src_dir)/*.h)
obj = $(patsubst $(src_dir)/%.cpp, $(obj_dir)/%.o, $(src))

.PHONY: all objs clean run test

all: $(exe)

$(exe): $(obj) $(LIB)
	$(CC) $(DEFINE) $(CFLAGS) -o $(exe) $(obj) $(LIB) $(LINK)

# Object files only (used by test/Makefile).
objs: $(obj)

# Order-only prerequisite: the directory must exist, but its timestamp
# must not trigger a rebuild of every object on each invocation.
$(obj_dir)/%.o: $(src_dir)/%.cpp $(header) | $(obj_dir)
	$(CC) $(DEFINE) $(CFLAGS) -c $< -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

lib/libabc.a:
	$(MAKE) -j -C "lib/abc/" libabc.a ABC_USE_NO_PTHREADS=1 ABC_USE_NO_READLINE=1
	mv lib/abc/libabc.a lib/

lib/aiger.o:
	gcc -c lib/aiger/aiger.c -o lib/aiger.o

lib/libcadical.a:
	cd lib/cadical && ./configure -q && $(MAKE) -j
	mv lib/cadical/build/libcadical.a lib/

test: objs $(LIB)
	$(MAKE) -C test

clean:
	rm -rf $(obj_dir) $(exe)

run: all
	./main input.txt output.txt
	rm -f *.aig *.aag stdoutOutput.txt *.cnf
	rm -rf optimizeAIG
