ThingsBoard COAP Sensor
========================================



## 入门

参考 Zephyr 官方文档，安装 SDK：https://docs.zephyrproject.org/latest/getting_started/index.html

创建项目主目录，如 ot_app，然后 clone 此项目源码到 ot1m 目录，然后使用 west 命令编译，如：

```
mkdir ot_app
cd ot_app
git clone https://github.com/yplam/ot1m.git
west init -l ot1m
west update
cd ot1m
west build -b nrf52840ot1m_nrf52840  -- -DCONF_FILE="prj.conf overlay-at-modem.conf overlay-coap.conf"

```
