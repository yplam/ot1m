# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

add_subdirectory_ifdef(CONFIG_DISPLAY display)
