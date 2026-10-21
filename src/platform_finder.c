#include "platform_finder.h"
#include "cJSON.h"
#include "raylib.h"
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
    }
    cJSON *title_item;
    if ((title_item = cJSON_GetArrayItem(programs, index)) == NULL) {
        goto cleanup;
    }

    cJSON *desc
        = cJSON_GetObjectItemCaseSensitive(title_item, "description");
    char *description = cJSON_GetStringValue(desc);

    if (str_alloc_cpy(&data_out->description, description) == 1) {
        goto cleanup;
    }

    cJSON *roms = cJSON_GetObjectItemCaseSensitive(title_item, "roms");
    cJSON *rom = cJSON_GetObjectItemCaseSensitive(roms, hash);

    cJSON *rom_filename
        = cJSON_GetObjectItemCaseSensitive(rom, "file");

    char *filename = cJSON_GetStringValue(rom_filename);
    if (str_alloc_cpy(&data_out->filename, filename) == 1) {
        goto cleanup;
    }

    cJSON *platforms = cJSON_GetObjectItemCaseSensitive(rom, "platforms");
    // getting only the first for simplicity
    cJSON *platform_item = cJSON_GetArrayItem(platforms, 0);
    char *platform = cJSON_GetStringValue(platform_item);
    if (str_alloc_cpy(&data_out->platform, platform) == 1) {
        goto cleanup;
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