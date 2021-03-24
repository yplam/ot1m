#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_APP_COAP coap)
add_subdirectory_ifdef(CONFIG_APP_OPENTHREAD openthread)
add_subdirectory_ifdef(CONFIG_APP_SETTINGS settings)
add_subdirectory_ifdef(CONFIG_APP_LWM2M lwm2m)
add_subdirectory_ifdef(CONFIG_APP_SHT30D sht30d)
add_subdirectory_ifdef(CONFIG_APP_BUTTON button)

