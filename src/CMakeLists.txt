cmake_minimum_required(VERSION 3.2)

include(FindPkgConfig)

pkg_check_modules(JACK jack)
pkg_check_modules(JSONCPP jsoncpp)

set(CMAKE_CXX_FLAGS "-std=c++11 -O2 -ffast-math -Wall -Wextra")  

add_executable(ghostsyn
  main.cpp
  ghostsyn.cpp
  compressor.cpp
  dcfilter.cpp
  delay.cpp
  controller.cpp
  instrument.cpp
  voice.cpp
  )

target_include_directories(ghostsyn PRIVATE
  ${JACK_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  )

target_link_libraries(ghostsyn
  ${JACK_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  )

enable_testing()

add_executable(test_ghostsyn
  test_controller.cpp
  controller.cpp
  )

target_link_libraries(test_ghostsyn
  boost_unit_test_framework
  )

add_test(NAME ghostsyn
  COMMAND test_ghostsyn)

add_library(ghostsyn_lv2 SHARED
  lv2/lv2_plugin.cpp
  )

install(TARGETS ghostsyn_lv2
  DESTINATION lib/lv2/ghostsyn.lv2)

install(FILES lv2/lv2_ghostsyn.ttl lv2/manifest.ttl
  DESTINATION lib/lv2/ghostsyn.lv2)
