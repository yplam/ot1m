# SPDX-License-Identifier: Apache-2.0

#zephyr_library()
#zephyr_library_sources(board.c)

board_runner_args(jlink "--device=nrf52" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)