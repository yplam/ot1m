#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_named(app_lwm2m)
zephyr_include_directories(.)
include_directories(${ZEPHYR_BASE}/subsys/net/lib)
zephyr_library_sources_ifdef(CONFIG_APP_LWM2M app_lwm2m.c)
