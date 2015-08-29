cmake_minimum_required(VERSION 3.2)

include(FindPkgConfig)

pkg_check_modules(JACK jack)
pkg_check_modules(JSONCPP jsoncpp)

add_executable(ghostsyn
  main.cpp
  ghostsyn.cpp
  compressor.cpp
  dcfilter.cpp
  delay.cpp
  )

target_compile_options(ghostsyn PRIVATE
  -std=c++11
  -O2
  -ffast-math
  )

target_include_directories(ghostsyn PRIVATE
  ${JACK_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  )

target_link_libraries(ghostsyn
  ${JACK_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  )