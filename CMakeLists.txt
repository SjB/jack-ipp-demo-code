cmake_minimum_required (VERSION 2.8)
project (jack-wav-demo)

link_directories("/opt/intel/ipp/lib/intel64_lin/")
include_directories("/opt/intel/ipp/include")

add_subdirectory(src)
