#include "audio.h"

AudioStream init_audio()
{
    // initializing Audio
    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(BUFFER_SIZE);

    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    SetAudioStreamPan(stream, 0.0f);
    SetAudioStreamVolume(stream, 0.5f);
    PlayAudioStream(stream);

    return stream;
}

Audio_data init_audio_vars(AudioStream stream)
{
    return (Audio_data) {
        .stream = stream,
        .buffer = { 0 },
        .sample_step = 0,
        .volume = 0.5f,   // volume between 0 to 1
        .frequency = 440  // frequency of the square have in hertz
    };
}

void sample_audio_buffer(Audio_data *audio)
{
    for (int i = 0; i < BUFFER_SIZE; i++) {
        int wavelength = SAMPLE_RATE / audio->frequency;

        // square wave
        audio->buffer[i] = sinf(
            2* PI * audio->frequency
            * audio->sample_step/SAMPLE_RATE) > 0 ? 1 : -1;

        audio->buffer[i] *= audio->volume * 0.1f;
        audio->sample_step++;
        if (audio->sample_step >= wavelength) audio->sample_step = 0;
    }
}

void init_audio_buffer(float buffer[], int *sineIndex)
{
    const int sineFrequency = 440;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        int wavelength = SAMPLE_RATE / sineFrequency;
        buffer[i] = sinf(2* PI * (*sineIndex)/wavelength) > 0 ? 1 : -1;
        (*sineIndex)++;
        if ((*sineIndex) >= wavelength) (*sineIndex) = 0;
    }
}
