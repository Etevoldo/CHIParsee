#pragma once
#include <stddef.h>
#include "types.h"

#define SHA1_STR_SIZE 41

typedef struct {
    char *filename;
    char *description;
    char *platform;
    int tickrate;
    Quirks quirks;
} Program_data;

// Takes the database info given a hash.
// Each member in `data_out` must be `free`d to avoid leak.
int check_rom(const char *hash, Program_data *data_out);

// Strcpy, but allocates memory for destination.
int str_alloc_cpy(char **dest, const char *src);

// Takes the checksum and converts it into 40 hexadecimal digits
// formated into a null-terminated string array pointed by `out` array.
// Expects that the `out` array has 40+1 available size
void file_checksum(const char *rom_name, char *out);

// Frees memory used by strings alocated by `check_rom`'s data_out.
void free_program_data(Program_data *data);

// Maps a `plataform` name to a Quirk configuration.
Quirks map_platform_quirks(const char *platform);
