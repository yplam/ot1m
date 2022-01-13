# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(BOARD ot1m_display_nrf52840)

#if (NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE ZDebug)
#endif()
# Point to the build type-specific config file
# set(CONF_FILE "prj.conf overlay-ot.conf overlay-dtls.conf")
set(CONF_FILE "prj.conf overlay-ot.conf")

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

include_directories(${CMAKE_SOURCE_DIR}/build/zephyr/include/generated)

file(STRINGS
        ${CMAKE_SOURCE_DIR}/build/zephyr/include/generated/autoconf.h
        LIST_OF_CONFIGS
        REGEX "^#define CONFIG_"
        ENCODING "UTF-8"
        )
foreach (CONFIG ${LIST_OF_CONFIGS})
    string(REPLACE " " ";" CONFIG_ITEM_LIST ${CONFIG})
    list(GET CONFIG_ITEM_LIST 1 CONFIG_ITEM_NAME)
    list(GET CONFIG_ITEM_LIST 2 CONFIG_ITEM_VALUE)
#    message(${CONFIG_ITEM_NAME})
#    message(${CONFIG_ITEM_VALUE})
    add_definitions('-D${CONFIG_ITEM_NAME}=${CONFIG_ITEM_VALUE}')
    set(${CONFIG_ITEM_NAME} ${CONFIG_ITEM_VALUE})
endforeach()

include_directories(${ZEPHYR_BASE}/subsys/net/ip ../../../modules/lib/openthread/include)


project(lwm2m_display)
target_sources(app PRIVATE src/main.c src/bat.c src/lwm2m_display.c src/app_display.c)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/src)