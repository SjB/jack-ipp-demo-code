#gcc -L/opt/intel/ipp/lib/intel64_lin -I/opt/intel/ipp/include -ljackd -lipp src/Jack_wav_LPF.c -o test

set(HEADERS
  wavfile.h)

set(SOURCES
  Jack_wav_LPF.c
  wavfile.c)

add_executable(jack_wav_lpf ${SOURCES})
target_link_libraries(jack_wav_lpf ippcore ipps jack sndfile)
