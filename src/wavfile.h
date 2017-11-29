/* 
** Copyright (c) 2017 National Center of Audiology
*/

/*
** wavfile.h -- wavfile definitions
*/

#ifndef WAVFILE_H_
#define WAVFILE_H_

typedef struct wavfile_info wavfile_info_t;

wavfile_info_t* wavfile_readfile(const char* filepath);
long wavfile_number_of_samples_in_all_channels(wavfile_info_t* info);
void wavfile_free(wavfile_info_t* info);
int wavfile_readframes(wavfile_info_t* info, int channel, float* buffer, int size);

#endif /* WAVFILE_H_ */
