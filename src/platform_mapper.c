#include "platform_mapper.h"
#include "cJSON.h"
#include "raylib.h"
#include <stdio.h>
#define TEENY_SHA1_IMPLEMENTATION
#include "teenysha1.h"

// strcpy, but allocates memory for destination
int str_alloc_cpy(char **dest, const char *src)
{
    size_t length = strlen(src) + 1;
    *dest = (char *) malloc(strlen(src) + 1);
    if (*dest == NULL) {
        return 1;
    }
    if (memcpy_s(*dest, length, src, length)) {
        return 1;
    }
    return 0;
}

int check_rom(const char *hash, Program_data *data_out)
{
    data_out->description = NULL;
    data_out->filename = NULL;
    data_out->platform = NULL;

    char *hashes_str = LoadFileText("resources/sha1-hashes.json");
    char *programs_str = LoadFileText("resources/programs.json");

    cJSON *hashes = cJSON_Parse(hashes_str);
    cJSON *programs = cJSON_Parse(programs_str);
    UnloadFileText(hashes_str);
    UnloadFileText(programs_str);

    cJSON *item = cJSON_GetObjectItemCaseSensitive(hashes, hash);
    if (!cJSON_IsNumber(item)) {
        printf("Hash: \"%s\"\nhas no item in the hashes database\n", hash);
        goto cleanup;
    }
    int index = (int) cJSON_GetNumberValue(item);

    if (!cJSON_IsArray(programs)) {
        goto cleanup;
        perror("programs.json is not an array, check if it is valid JSON");
    }
    cJSON *program;
    if ((program = cJSON_GetArrayItem(programs, index)) == NULL) {
        perror("Mismatch between sha1-hashes.json and program.json");
        goto cleanup;
    }

    cJSON *title_name
        = cJSON_GetObjectItemCaseSensitive(program, "title");
    char *title = cJSON_GetStringValue(title_name);
    if (str_alloc_cpy(&data_out->filename, title) == 1) {
        goto cleanup;
    }

    cJSON *desc_item
        = cJSON_GetObjectItemCaseSensitive(program, "description");
    // Description might not be there
    if (cJSON_IsString(desc_item)) {
        const char *description = cJSON_GetStringValue(desc_item);
        if (str_alloc_cpy(&data_out->description, description) == 1) {
            goto cleanup;
        }
    }
    //CONSIDER: allocate something like "No Description" or "-"
    else data_out->description = NULL;

    cJSON *roms = cJSON_GetObjectItemCaseSensitive(program, "roms");
    cJSON *rom = cJSON_GetObjectItemCaseSensitive(roms, hash);

    cJSON *quirkyPlatforms
        = cJSON_GetObjectItemCaseSensitive(rom, "quirkyPlatforms");
    //prioritize using quirkyPlatforms
    if (quirkyPlatforms) {
        // getting only the first for simplicity
        cJSON *platform_item = quirkyPlatforms->child;
        if (str_alloc_cpy(&data_out->platform, platform_item->string) == 1) {
            goto cleanup;
        }
        data_out->quirks = map_platform_quirks(platform_item->string);

        cJSON *shift
            = cJSON_GetObjectItemCaseSensitive(platform_item, "shift");
        if (shift) data_out->quirks.is_shift = cJSON_IsTrue(shift);

        cJSON *memoryIncrementByX = cJSON_GetObjectItemCaseSensitive(
            platform_item, "memoryIncrementByX");
        if (memoryIncrementByX) {
            data_out->quirks.is_memory_increment_by_x 
                = cJSON_IsTrue(memoryIncrementByX);
        }

        cJSON *memoryLeaveIUnchanged = cJSON_GetObjectItemCaseSensitive(
            platform_item, "memoryLeaveIUnchanged");
        if (memoryLeaveIUnchanged) {
            data_out->quirks.is_memory_leave_i_unchanged 
                = cJSON_IsTrue(memoryLeaveIUnchanged);
        }

        cJSON *wrap = cJSON_GetObjectItemCaseSensitive(platform_item, "wrap");
        if (wrap) { data_out->quirks.is_wrap = cJSON_IsTrue(wrap); }

        cJSON *jump = cJSON_GetObjectItemCaseSensitive(platform_item, "jump");
        if (jump) { data_out->quirks.is_jump = cJSON_IsTrue(jump); }

        cJSON *vblank = cJSON_GetObjectItemCaseSensitive(
            platform_item, "vblank");
        if (vblank) { data_out->quirks.is_vblank = cJSON_IsTrue(vblank); }

        cJSON *logic = cJSON_GetObjectItemCaseSensitive(
            platform_item, "logic");
        if (logic) { data_out->quirks.is_logic = cJSON_IsTrue(logic); }
    }
    else { // regular platforms list
        cJSON *platforms = cJSON_GetObjectItemCaseSensitive(rom, "platforms");
        // get first for simplicity, as it's the preferable one
        cJSON *platform_item = cJSON_GetArrayItem(platforms, 0);
        char *platform = cJSON_GetStringValue(platform_item);
        if (str_alloc_cpy(&data_out->platform, platform) == 1) {
            goto cleanup;
        }
        data_out->quirks = map_platform_quirks(platform);
    }

    cJSON *tickrate = cJSON_GetObjectItemCaseSensitive(rom, "tickrate");
    if (cJSON_IsNumber(tickrate)) {
        // overwrite ipf in case exists
        data_out->quirks.ipf = cJSON_GetNumberValue(tickrate);
    }

    cJSON_Delete(hashes);
    cJSON_Delete(programs);

    return 0;

    cleanup:;
    cJSON_Delete(hashes);
    cJSON_Delete(programs);
    return 1;
}

// Takes the checksum and converts it into 40 hexadecimal digits formated into a
// null-terminated string array pointed by `out` array
// expects the `out` array has 40+1 available size
void file_checksum(const char *rom_name, char *out)
{
    int data_size;
    unsigned char *rom = LoadFileData(rom_name, &data_size);

    SHA1 s = { 0 };
    sha1_reset(&s);
    sha1_process_bytes(&s, rom, data_size);
    digest8_t digest;
    sha1_get_digest_bytes(&s, digest);
    for (size_t i = 0; i < sizeof(digest); i++) {
        sprintf((out + i * 2), "%02x", digest[i]);
    }

    UnloadFileData(rom);
}


// frees memory used by strings alocated by `check_rom`'s data_out
void free_program_data(Program_data *data)
{
    free((void*)data->description);
    free((void*)data->platform);
    free((void*)data->filename);
}

// maps a `plataform` name to a Quirk configuration
Quirks map_platform_quirks(const char *platform)
{
    char *platforms_text = LoadFileText("resources/platforms.json");
    cJSON *platforms = cJSON_Parse(platforms_text);
    UnloadFileText(platforms_text);

    cJSON *platform_item = NULL;
    cJSON_ArrayForEach(platform_item, platforms) {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(platform_item, "id");
        char *name = cJSON_GetStringValue(id);
        if (strcmp(name, platform) == 0) {
            break; // platform_item is now pointing to the plataform desired
        }
    }
    cJSON *quirks_item
        = cJSON_GetObjectItemCaseSensitive(platform_item, "quirks");
    cJSON *shift
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "shift");
    cJSON *memoryIncrementByX
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "memoryIncrementByX");
    cJSON *memoryLeaveIUnchanged
        = cJSON_GetObjectItemCaseSensitive(quirks_item, 
            "memoryLeaveIUnchanged");
    cJSON *wrap
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "wrap");
    cJSON *jump
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "jump");
    cJSON *vblank
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "vblank");
    cJSON *logic
        = cJSON_GetObjectItemCaseSensitive(quirks_item, "logic");

    cJSON *defaultTickrate_item
        = cJSON_GetObjectItemCaseSensitive(platform_item, "defaultTickrate");

    Quirks q = (Quirks) {
        .is_shift = cJSON_IsTrue(shift),
        .is_memory_increment_by_x = cJSON_IsTrue(memoryIncrementByX),
        .is_memory_leave_i_unchanged = cJSON_IsTrue(memoryLeaveIUnchanged),
        .is_wrap = cJSON_IsTrue(wrap),
        .is_jump = cJSON_IsTrue(jump),
        .is_vblank = cJSON_IsTrue(vblank),
        .is_logic = cJSON_IsTrue(logic),
        .is_logical_scroll = true, // assumes modern superchip
        .ipf = cJSON_GetNumberValue(defaultTickrate_item)
    };
    // freeing memory
    cJSON_Delete(platforms);

    return q;
}
