#pragma once

#include <math.h>
#include "types.h"
#include "raylib.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096

AudioStream init_audio();
void sample_audio_buffer(Audio_data *audio);
Audio_data init_audio_vars(AudioStream stream);
