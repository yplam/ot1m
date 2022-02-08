#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_named(app_coap)
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_APP_COAP app_coap.c)
