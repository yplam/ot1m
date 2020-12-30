OT1M - OpenThread Development Module
========================================

OT1M 是一个开源的低功耗物联网模块，提供一个简便的方式来接入与测试 OpenThread 网络。

模块硬件基于 NRF52840，引脚跟常见的 ESP8266 模块兼容，以及引出了USB相关引脚；
OT1M 模块为设备接入物联网提供一个低功耗的选择，适合电池供电设备。

NRF52840 是一个支持多协议的低功耗无线 SOC，支持 BLE，Zigbee，Thread，在这里选用的是 Thread 协议 。

OpenThread 是由 Google 发布的 Thread 的开源实现，基于 IEEE 802.15.4， 
主要特点是实现了一个低功耗的IPv6协议，可以很容易的将原来基于IP协议的应用移植过来。

模块软件基于 Zephyr RTOS，实现了一个 AT 指令服务器，可以通过 AT 指令进行配置，联网，以及发送数据；
模块实现了 SNTP 客户端、COAP 客户端以及LWM2M客户端功能（开发中），分别用来同步时间，上传数据以及管理设备。

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
