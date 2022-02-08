#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_named(app_openthread)
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_APP_OPENTHREAD app_openthread.c)

