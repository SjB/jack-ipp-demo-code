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
#include "ipp.h"


jack_port_t *input_port;
jack_port_t *output_port1;
//jack_port_t *output_port2;

#define EXIT_MAIN exitLine:
#define check_sts(st) if((st) != ippStsNoErr) goto exitLine;


float filter(jack_default_audio_sample_t* in, jack_default_audio_sample_t* out, jack_nframes_t nframes) {


	FILE *pOutBin;
	int n = 419190*2;
	Ipp64f bytes[n];

	Ipp64f* original_audio_buffer = ippsMalloc_64f(nframes * sizeof(Ipp64f));
	Ipp64f* processed_audio_buffer = ippsMalloc_64f(nframes * sizeof(Ipp64f));

	pOutBin = fopen("/tmp/out.bin","rb");
	fread(bytes,sizeof(Ipp64f),n, pOutBin);
	fclose(pOutBin);

	for(int i=0;i<n;i+=2){
		original_audio_buffer[i] = (Ipp64f)bytes[i];
		printf("%f \n", original_audio_buffer[i]);
	}

	IppStatus status = ippStsNoErr;
	int tapslen = 21;
	int bufSize = 0;

	check_sts(status = ippsFIRGenGetBufferSize(tapslen, &bufSize))

	Ipp8u* pBuffer = NULL;
	pBuffer = ippsMalloc_8u(bufSize);

	float rFreq = 0.2;
	Ipp64f* taps = ippsMalloc_64f(tapslen * sizeof(Ipp64f));
	check_sts(status = ippsFIRGenLowpass_64f(rFreq, taps, tapslen, ippWinBartlett, ippTrue, pBuffer))

//	printf("Generated taps:\n");
//	for(i = 0; i < tapslen; i++){
//		printf("%f,", taps[i]);
//	}

	int specSize = 0;
	int tempbufferSize;

	check_sts(status = ippsFIRSRGetSize(tapslen, ipp64f, &specSize, &tempbufferSize))

	IppsFIRSpec_64f* pSpec = NULL;
	pSpec = (IppsFIRSpec_64f*) ippsMalloc_8u(specSize);
	Ipp8u* pTempBuffer = NULL;
	pTempBuffer = ippsMalloc_8u(tempbufferSize);

	IppAlgType algType = ippAlgDirect;
	check_sts(status = ippsFIRSRInit_64f(taps, tapslen, algType, pSpec))

	int numIters = nframes;
	Ipp64f* pDlySrc = NULL;
	Ipp64f* pDlyDst = NULL;
	check_sts(status = ippsFIRSR_64f(original_audio_buffer, processed_audio_buffer, numIters, pSpec, pDlySrc, pDlyDst, pTempBuffer))

	FILE *pFile;
	pFile = fopen("Jack_wav_LPF.csv", "a");
	for (int i = 0; i < nframes; i++) {
		fprintf(pFile, "%d\t%.4f\t%.4f\n", i, original_audio_buffer[i], processed_audio_buffer[i]);
	}
	fclose(pFile);

	for (int i = 0; i < nframes; i++) {
		out[i] = (jack_default_audio_sample_t)(processed_audio_buffer[i]);

		//printf("%f -- %f\n", in[i], out[i]);
	}
	//memcpy (out, processed_audio_buffer, count);


	//free(original_audio_buffer);
	//free(processed_audio_buffer);

EXIT_MAIN
	ippsFree(original_audio_buffer);
	ippsFree(processed_audio_buffer);
	ippsFree(pBuffer);
	ippsFree(taps);
	ippsFree(pSpec);
	ippsFree(pTempBuffer);
	//printf("Exit status %d (%s) \n", (int) status, ippGetStatusString(status));
	return (float) status;
}

//void sjbFilter(jack_default_audio_sample_t* in, jack_default_audio_sample_t* out, jack_nframes_t nframes) {
//	int count = sizeof(jack_default_audio_sample_t)*nframes;
//
//	for(int i = 0; i < count; i++) {
//			out[i] = in[i];
//	}
//}

int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;

//	printf("%d\n", nframes);

	in = jack_port_get_buffer (input_port, nframes);
	out = jack_port_get_buffer (output_port1, nframes);

	filter(in, out, nframes);

	return 0;
}

void
jack_shutdown (void *arg)
{
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

	jack_set_process_callback (client, process, 0);

	jack_on_shutdown (client, jack_shutdown, 0);

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

	for (int i = 0; out_ports[i] != '\0'; i++) {
		printf("Output ports %d %s\n", i, out_ports[i]);
		fflush(stdout);
	}

	if (jack_connect (client, out_ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}


	output_port1 = jack_port_register (client, "output1",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput,0);

//	output_port2 = jack_port_register (client, "output2",
//						  JACK_DEFAULT_AUDIO_TYPE,
//						  JackPortIsOutput,0);

	const char ** in_ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

	if (in_ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	for (int i = 0; in_ports[i] != '\0'; i++) {
		printf("Input ports %d %s\n", i, in_ports[i]);
		fflush(stdout);
	}

	if (jack_connect (client, jack_port_name (output_port1), in_ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

//	if (jack_connect (client, jack_port_name (output_port2), in_ports[1])) {
//		fprintf (stderr, "cannot connect output ports\n");
//	}
//	if (jack_connect (client, in_ports[2], out_ports[1])) {
//		fprintf(stderr, "cannot connect two physical ports\n");
//	}


	free(out_ports);
	free(in_ports);

	const char *port_name;

	port_name = jack_port_name(input_port);
	printf("Input %s\n", port_name);


	port_name = jack_port_name(output_port1);
	printf("Output %s\n", port_name);

//	port_name = jack_port_name(output_port2);
//	printf("Output %s\n", port_name);


	while(1) {
		sleep(1);
	}

	//jack_client_close(client);

	//clock_t end = clock();
	//double time_spent = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
	//printf("\nThe execution time is %.5f milliseconds\n", time_spent);

}

