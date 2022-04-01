links = -lz -lGL -lGLU -lGLEW -lglfw -lrt -lm -ldl -lX11 -lXdmcp -lpthread -lxcb -lXau
buildDir = build/
libDir = lib/
includes = vendor/ vendor/glm/
flags = -std=c++2a -Wall -Wno-unused-function -Wno-format-truncation -Wno-pointer-arith\
		 -Wno-sign-compare -Wno-maybe-uninitialized -Wno-deprecated-enum-enum-conversion
Cflags = -O2 -march=native

includes := $(addprefix -I ,$(includes))
additionalDirs = vendor/tracy/ vendor/imgui/ vendor/imgui/backends/
dirs := $(dir $(shell find ./src/ -name "*.cpp" -o -name "*.c"))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
dirs := $(call uniq,$(dirs)) $(additionalDirs)
objFiles := $(patsubst %.cpp, %.o, $(addprefix $(buildDir), $(notdir $(foreach dir, $(dirs), $(wildcard $(dir)*.cpp))))) $(patsubst %.c, %.o, $(addprefix $(buildDir), $(notdir $(foreach dir, $(dirs), $(wildcard $(dir)*.c)))))

CC = g++
VPATH = %.cpp $(dirs)

debug: flags := $(flags) -g -DDEBUG
debug: $(objFiles)
	$(CC) $(flags) $(includes) $(links) $(objFiles) -o $@

release: flags := $(flags) -O3 -march=native
release: $(objFiles)
	$(CC) $(flags) $(includes) $(links) $(objFiles) -o $@

profiling: flags := $(flags) -g -O3 -march=native -DTRACY_ENABLE -fno-omit-frame-pointer
profiling: links := $(links) -lfreetype -lcapstone -lgtk-3
profiling: $(objFiles)
	$(CC) $(flags) $(includes) $(links) $(objFiles) -o $@ -rdynamic

small: flags := $(flags) -Os -march=native
small: $(objFiles)
	$(CC) $(flags) $(includes) $(links) $(objFiles) -o $@


precompile.cc: flags := $(flags) -E
precompile.cc: $(objFiles)

lib: $(libDir)liboxide.a

$(buildDir)%.o: %.cpp
	$(CC) $(flags) $(includes) -c $< -o $@

$(buildDir)%.o: %.c
	gcc $(Cflags) $(includes) -c $< -o $@

$(libDir)liboxide.a: $(objFiles)
	ar rvs $(libDir)liboxide.a $(objFiles)

clean:
	rm -f build/* || true

# vim: set noexpandtab:
