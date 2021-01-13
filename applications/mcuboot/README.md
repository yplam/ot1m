MCUBOOT
================

ot1m_evm 支持 MCUBOOT 通过 USB 下载固件, 本目录包含相关文件

```shell
cd ot_app/bootloader/mcuboot/boot/zephyr
west build -b nrf52840ot1m_nrf52840  -- -DDTC_OVERLAY_FILE="boards/nrf52840ot1m_nrf52840.overlay"
```

led.c 作用是在 bootloader 模式下简便闪烁 LED，需要这功能的话需要修改 mcuboot 的 CMakeLists.txt 加入此文件
