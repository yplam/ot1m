# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf52840ot1m_nrf52840)

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

include_directories(${ZEPHYR_BASE}/subsys/net/ip ../modules/lib/openthread/include)

project(thingsboard_coap_sensor)
target_sources(app PRIVATE src/button.c src/main.c)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/src)