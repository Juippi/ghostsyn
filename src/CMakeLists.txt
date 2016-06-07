cmake_minimum_required(VERSION 3.2)

include(FindPkgConfig)

pkg_check_modules(JACK jack)
pkg_check_modules(JSONCPP jsoncpp)

set(CMAKE_CXX_FLAGS "-std=c++11 -O2 -ffast-math -Wall -Wextra")
include_directories(synth)

set(ghostsyn_srcs
  synth/ghostsyn.cpp
  synth/compressor.cpp
  synth/dcfilter.cpp
  synth/delay.cpp
  synth/controller.cpp
  synth/instrument.cpp
  synth/voice.cpp
  )

#add_executable(ghostsyn
#  standalone/main.cpp
#  ${ghostsyn_srcs}
#  )

#target_include_directories(ghostsyn PRIVATE
#  ${JACK_INCLUDE_DIRS}
#  ${JSONCPP_INCLUDE_DIRS}
#  )

#target_link_libraries(ghostsyn
#  ${JACK_LIBRARIES}
#  ${JSONCPP_LIBRARIES}
#  )

#enable_testing()

add_executable(test_ghostsyn
  synth/test_controller.cpp
  synth/controller.cpp
  )

target_link_libraries(test_ghostsyn
  boost_unit_test_framework
  )

#add_test(NAME ghostsyn
#  COMMAND test_ghostsyn)

add_library(ghostsyn_lv2 SHARED
  lv2/lv2_plugin.cpp
  ${ghostsyn_srcs}
  )

target_include_directories(ghostsyn_lv2 PRIVATE
  ${JSONCPP_INCLUDE_DIRS}
  )

install(TARGETS ghostsyn_lv2
  DESTINATION lib/lv2/ghostsyn.lv2)

install(FILES lv2/lv2_ghostsyn.ttl lv2/manifest.ttl
  DESTINATION lib/lv2/ghostsyn.lv2)
