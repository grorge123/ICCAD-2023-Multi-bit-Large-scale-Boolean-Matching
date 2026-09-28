CC := g++
exe := main
#obj := main.o statistic.o evaluate.o utility.o
CFLAGS := -std=c++17 -O3 -Wall -Wextra -g
SANITIZE := -fsanitize=undefined -fsanitize=address
DEFINE := -DINF -DDBG
LINK := -lm -ldl -lreadline -ltinfo
LIB := lib/libabc.a lib/aiger.o lib/libcadical.a
#INCLUDES :=

src_dir = ./src
obj_dir = ./obj

src = $(wildcard $(src_dir)/*.cpp)
header = $(wildcard $(src_dir)/*.h)
obj = $(patsubst $(src_dir)/%.cpp, $(obj_dir)/%.o, $(src))

all:$(obj) $(LIB) $(header)
	$(CC) $(DEFINE) $(CFLAGS) -o $(exe) $(obj) $(LIB) $(LINK)

initDir:
	mkdir -p $(obj_dir)

$(obj_dir)/%.o: $(src_dir)/%.cpp $(header) initDir
	$(CC) $(DEFINE) $(CFLAGS) -c $< -o $@

lib/libabc.a:
	$(MAKE) -j -C "lib/abc/" libabc.a ABC_USE_NO_PTHREADS=1 ABC_USE_NO_READLINE=1
	mv lib/abc/libabc.a lib/


lib/aiger.o:
	gcc -c lib/aiger/aiger.c -o lib/aiger.o

lib/libcadical.a:
	cd lib/cadical && ./configure -q && make -j
	mv lib/cadical/build/libcadical.a lib/

test:$(obj) $(LIB) $(header)
	echo $(header)

clean:
	rm -rf $(obj) $(exe)
run: all
	./main input.txt output.txt
	rm -f *.aig *.aag stdoutOutput.txt *.cnf
	rm -rf optimizeAIG

