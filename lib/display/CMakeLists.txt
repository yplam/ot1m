#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_named(app_display)
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_APP_DISPLAY epdpaint.c font8.c font24.c font1632.c font815.c font_icon.c)


