# Quick summary: gets all '.c' files from the current directory
# compiles them and sends them to the 'build' directory, and links
# them all to the 'main' executable.

LINKER_FLAGS := -lopengl32 -lraylib -lgdi32 -lwinmm
INCLUDE_PATH := "include"
LIBRARY_PATH := "lib"
BUILD_DIR := ./build
SRC_DIR := ./src
DEBUG := -g -DDEBUG
FLAGS := -Wall -O0

# find all *.c in src_dir
SRCS_PATH = ${shell find ${SRC_DIR} -name '*.c'}
# prefix object files with the build directory
# for their build location
OBJS_PATH = $(patsubst ${SRC_DIR}/%.c,${BUILD_DIR}/%.o,${SRCS_PATH})

# build everything in the OBJS_PATH
main: ${OBJS_PATH}
	gcc $^ ${FLAGS} ${DEBUG} -L${LIBRARY_PATH} ${LINKER_FLAGS} -o $@

# expands into
# build/<stem>.o: <stem>.c
# for each respective object file
${OBJS_PATH}: ${BUILD_DIR}/%.o: ${SRC_DIR}/%.c
	gcc $^ -c ${DEBUG} ${FLAGS} -I${INCLUDE_PATH} -o $@

.PHONY: clean
clean:
	rm -f ${BUILD_DIR}/*