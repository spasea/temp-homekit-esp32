# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# if(DEFINED ENV{HOMEKIT_PATH})
#   set(HOMEKIT_PATH $ENV{HOMEKIT_PATH})
# else()
  set(HOMEKIT_PATH ${CMAKE_CURRENT_LIST_DIR}/components/esp-homekit-sdk)
# endif(DEFINED ENV{HOMEKIT_PATH})

if (EXISTS ${HOMEKIT_PATH})
  message(WARNING "HOMEKIT_PATH=" ${HOMEKIT_PATH})
else()
  message(ERROR "Homekit path is not valid: HOMEKIT_PATH=" ${HOMEKIT_PATH})
endif()

set(EXTRA_COMPONENT_DIRS
${HOMEKIT_PATH}/components/homekit 
${HOMEKIT_PATH}/examples/common 
${CMAKE_CURRENT_LIST_DIR}/components/esp-idf-lib/components/sht3x
${CMAKE_CURRENT_LIST_DIR}/components/esp-idf-lib/components/i2cdev
${CMAKE_CURRENT_LIST_DIR}/components/esp-idf-lib/components/esp_idf_lib_helpers 
${HOMEKIT_PATH}/components/button)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(temp-esp32-homekit)
