# Detect Windows or Linux
ifeq '$(findstring ;,$(PATH))' ';' # semicolons on path
	LINKER_FLAGS := -lopengl32 -lraylib -lgdi32 -lwinmm
else
	LINKER_FLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif
# ...Realistic is detects if it's Windows or non-Windows, but should work

# Quick summary: gets all '.c' files from the current directory
# compiles them and sends them to the 'build' directory, and links
# them all to the 'main' executable.
CC := gcc
INCLUDE_PATH := "include"
LIBRARY_PATH := "lib"
BUILD_DIR := ./build
SRC_DIR := ./src
DEBUG := -g -DDEBUG
FLAGS := -O0 -std=c99 -pedantic
WARNS := -Wall -Wextra -Wno-unused-function -Wno-unused-parameter

# find all *.c in src_dir
SRCS = ${shell find ${SRC_DIR} -name "*.c"}
# prefix object files with the build directory
# for their build location
OBJS = $(patsubst ${SRC_DIR}/%.c,${BUILD_DIR}/%.o,${SRCS})

DEPS := $(patsubst %.o,%.d,${OBJS})

CPPFLAGS := -I${INCLUDE_PATH} -MMD -MP

# build everything in the OBJS_PATH
chiparsee: ${OBJS}
	${CC} $^ ${DEBUG} ${FLAGS} ${WARNS} -L${LIBRARY_PATH} ${LINKER_FLAGS} -o $@

# expands into
# build/<stem>.o: <stem>.c
# for each respective object file
${BUILD_DIR}/%.o: ${SRC_DIR}/%.c
	${CC} $< -c ${DEBUG} ${FLAGS} ${WARNS} ${CPPFLAGS} -o $@


.PHONY: clean
clean:
	rm -f ${BUILD_DIR}/*

-include ${DEPS}
