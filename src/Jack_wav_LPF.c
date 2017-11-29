/*
 ============================================================================
 Name        : Jack_initialization.c
 Author      : Krishnan
 Copyright   : Your copyright notice
 Description : Low Pass Filter
 ============================================================================
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <jack/jack.h>
#include <jack/control.h>
#include <ipp.h>
#include "wavfile.h"

jack_port_t *input_port;
jack_port_t *output_port1;
//jack_port_t *output_port2;

#define ROUND(x) ((long)(x + 0.5))

void write_to_csv(Ipp64f* dst, Ipp64f* src, int size) 
{
	FILE* pFile = fopen("Jack_wav_LPF.csv", "a");
	for (int i = 0; i < size; i++) {
		fprintf(pFile, "%d\t%.4f\t%.4f\n", i, src[i], dst[i]);
	}
	fclose(pFile);
}

int copybuffer_float_to_ipp64f(Ipp64f* dst, float* src, int size)
{	
	for(int i = 0; i < size; i++)
		dst[i] = src[i];
	return size;
}

int copybuffer_ipp64f_to_float(float* dst, Ipp64f* src, int size) 
{
	for(int i = 0; i < size; i++)
		dst[i] = (float)src[i];
	return size;
}

int filter(Ipp64f* dst, Ipp64f* src, int nframes)
{
	int tapslen = 21;
	int bufSize;
	int specSize;
	int tempbufferSize;
	
	if (ippsFIRGenGetBufferSize(tapslen, &bufSize) != ippStsNoErr)
		return -1;
	
	if (ippsFIRSRGetSize(tapslen, ipp64f, &specSize, &tempbufferSize) != ippStsNoErr)
		return -1;

	Ipp8u* pBuffer = ippsMalloc_8u(bufSize);
	Ipp64f* taps = ippsMalloc_64f(tapslen * sizeof(Ipp64f));

	float rFreq = 0.2;
	if (ippsFIRGenLowpass_64f(rFreq, taps, tapslen, ippWinBartlett, ippTrue, pBuffer) != ippStsNoErr)
		goto cleanup;
	
	IppsFIRSpec_64f* pSpec = (IppsFIRSpec_64f*) ippsMalloc_8u(specSize);
	Ipp8u* pTempBuffer = ippsMalloc_8u(tempbufferSize);

	IppAlgType algType = ippAlgDirect;
	if (ippsFIRSRInit_64f(taps, tapslen, algType, pSpec) != ippStsNoErr)
		goto cleanup;

	int numIters = nframes;
	Ipp64f* pDlySrc = NULL;
	Ipp64f* pDlyDst = NULL;

	if (ippsFIRSR_64f(src, dst, numIters, pSpec, pDlySrc, pDlyDst, pTempBuffer) == ippStsNoErr)
		write_to_csv(dst, src, nframes);

cleanup:
	ippsFree(pBuffer);
	ippsFree(taps);
	ippsFree(pSpec);
	ippsFree(pTempBuffer);
	return (float) nframes;
}

//void sjbFilter(jack_default_audio_sample_t* in, jack_default_audio_sample_t* out, jack_nframes_t nframes) {
//	int count = sizeof(jack_default_audio_sample_t)*nframes;
//
//	for(int i = 0; i < count; i++) {
//			out[i] = in[i];
//	}
//}

int process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out = jack_port_get_buffer (output_port1, nframes);

	float* audio_in = (float*)malloc(nframes * sizeof(float));
	int read_cnt = wavfile_readframes((wavfile_info_t*)arg, 1, audio_in, nframes);
	
	Ipp64f* src = ippsMalloc_64f(read_cnt);
	read_cnt = copybuffer_float_to_ipp64f(src, audio_in, read_cnt);

	Ipp64f* dst = ippsMalloc_64f(read_cnt);
	read_cnt = filter(dst, src, read_cnt);

	read_cnt = copybuffer_ipp64f_to_float(out, dst, read_cnt);
	
	return 0;
}

void
jack_shutdown (void *arg)
{
	wavfile_free((wavfile_info_t*)arg);
	exit (1);
}

int main() {

	//clock_t begin = clock();
	jack_options_t options = JackNullOption;
	jack_status_t status = (jack_status_t)0;
	jack_client_t *client;


	const char *client_name = "kris";
	const char *server_name = NULL;

	printf("Stage 0		");
	printf("Status %x\n", status);

	client = jack_client_open (client_name, options, &status, server_name);

	printf("Stage 1		");	
	printf("Status 0x%x\n", status);
	if (client == NULL) {
		fprintf(stderr, "jack_client_open() failed, ""status = 0x%2.0x\n", status);
		exit (1);
	}

	if (status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}

	if (status & JackServerFailed) {
		fprintf(stderr, "Unable to connect to JACK server\n");
		exit (1);
	}

	printf("Stage 2		");
	printf("Status %x\n", status);

	if (status & JackFailure) {
		fprintf(stderr, "Jack Failure");
		exit(1);
	}

	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf(stderr, "unique name `%s' assigned\n", client_name);
	}

	fflush(stdout);

	wavfile_info_t* audiodata = wavfile_readfile("/tmp/out.wav");
	if (audiodata == NULL)
		exit (1);

	jack_set_process_callback (client, process, audiodata);

	jack_on_shutdown (client, jack_shutdown, audiodata);

	if (jack_activate(client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	printf("Stage 3		");
	printf("Status %x\n", status);
	fflush(stdout);

	/* create two ports */

	input_port = jack_port_register (client, "input",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput,0);

	const char **out_ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);

	if (out_ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	for (int i = 0; out_ports[i] != NULL; i++) {
		printf("Output ports %d %s\n", i, out_ports[i]);
		fflush(stdout);
	}

	if (jack_connect (client, out_ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	output_port1 = jack_port_register (client, "output1",
					   JACK_DEFAULT_AUDIO_TYPE,
					   JackPortIsOutput,0);

	const char ** in_ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

	if (in_ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	for (int i = 0; in_ports[i] != NULL; i++) {
		printf("Input ports %d %s\n", i, in_ports[i]);
		fflush(stdout);
	}

	if (jack_connect (client, jack_port_name (output_port1), in_ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free(out_ports);
	free(in_ports);

	const char *port_name;

	port_name = jack_port_name(input_port);
	printf("Input %s\n", port_name);

	port_name = jack_port_name(output_port1);
	printf("Output %s\n", port_name);

	while(1) {
		sleep(1);
	}

	wavfile_free(audiodata);
	//jack_client_close(client);

	//clock_t end = clock();
	//double time_spent = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
	//printf("\nThe execution time is %.5f milliseconds\n", time_spent);

}

