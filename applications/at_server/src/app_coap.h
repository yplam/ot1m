#ifndef OT1M_APP_COAP_H
#define OT1M_APP_COAP_H
#include <net/net_ip.h>
#include <net/coap.h>

typedef void (*app_coap_send_cb)(const char *data, size_t data_len);

int app_coap_send_request(struct sockaddr_in6 * addr6, enum coap_method method,
                          uint8_t * uri_path, uint8_t * payload, uint8_t type, app_coap_send_cb cb );

#endif //OT1M_APP_COAP_H
