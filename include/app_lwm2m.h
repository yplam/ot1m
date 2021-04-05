#ifndef OT_AT_APP_LWM2M_H
#define OT_AT_APP_LWM2M_H
#include <zephyr.h>
#include <net/net_ip.h>
#include <net/lwm2m.h>

typedef struct {
    int8_t server_addr[NET_IPV6_ADDR_LEN];
    uint8_t server_is_bootstrap;
    uint8_t enable_psk;
    int8_t psk_id[32];
    uint8_t psk_key[32];
    uint8_t psk_key_length;
} app_lwm2m_settings;

enum lwm2m_rd_client_event app_lwm2m_get_client_event(void);
int app_lwm2m_client_start(app_lwm2m_settings * lwm2m_settings);
void app_lwm2m_client_stop(void);
#endif //OT_AT_APP_LWM2M_H
