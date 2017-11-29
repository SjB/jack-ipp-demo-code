#include <stdlib.h>
#include <sndfile.h>

#include "wavfile.h"

struct wavfile_info {
    long num_frames_per_channel;
    long* frame_ptr;
    int channels;
    float* frames;
};

static wavfile_info_t* wavfile_create(long frame_cnt, int channels)
{
    wavfile_info_t* info = (wavfile_info_t*)malloc(sizeof(wavfile_info_t));

    info->num_frames_per_channel = frame_cnt;
    info->channels = channels;
    info->frame_ptr = (long*)calloc(channels, sizeof(int));
    info->frames = NULL;

    return info;
}

wavfile_info_t* wavfile_readfile(const char* filepath)
{
    SF_INFO header;
    
    SNDFILE* fhdl = sf_open(filepath, SFM_READ, &header);

    if (fhdl == NULL) {
        #ifdef DEBUG
        fprintf(stderr, "Error %s\n", sf_strerror(fhdl));
        #endif
        return NULL;
    }

    wavfile_info_t* info = wavfile_create(header.frames, header.channels);
    
    long total_frames = wavfile_number_of_samples_in_all_channels(info);
    info->frames = (float*)malloc(sizeof(float)*total_frames);

    sf_count_t read_cnt = sf_read_float(fhdl, info->frames, total_frames);
    sf_close(fhdl);

    if (read_cnt != total_frames) {
        wavfile_free(info);
        return NULL;
    }

    return info;
}


long wavfile_number_of_samples_in_all_channels(wavfile_info_t* info) 
{
    return info->num_frames_per_channel * info->channels;
}

void wavfile_free(wavfile_info_t* info)
{
    free(info->frames);
    free(info);
}


// channels are index starting at 1
int wavfile_readframes(wavfile_info_t* info, int channel, float* buffer, int size) 
{
    int total_frames = wavfile_number_of_samples_in_all_channels(info);
    
    channel -= 1; // re-index to zero
    for (int i = 0; i < size; i++) {
        buffer[i] = info->frames[info->frame_ptr[channel] + channel];
        info->frame_ptr[channel] += info->channels;
        if (info->frame_ptr[channel] > total_frames)
            info->frame_ptr[channel] = 0;
    }
    return size;
}
