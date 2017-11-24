#gcc -L/opt/intel/ipp/lib/intel64_lin -I/opt/intel/ipp/include -ljackd -lipp src/Jack_wav_LPF.c -o test

link_directories("/opt/intel/ipp/lib/intel64_lin/")
add_executable(jack_wav_lpf Jack_wav_LPF.c)
target_include_directories(jack_wav_lpf PRIVATE "/opt/intel/ipp/include")
target_link_libraries(jack_wav_lpf jack ippcore ipps)
