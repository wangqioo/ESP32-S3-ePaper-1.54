cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "../../bsp/esp32-s3-epaper-1.54-v2")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(board_smoke_test)
