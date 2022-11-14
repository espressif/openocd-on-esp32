cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(openocd-on-esp32)
