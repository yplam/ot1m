#include "app_coap.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(app_coap, LOG_LEVEL_INF);

#include <net/socket.h>
#include <net/udp.h>
#include <net/coap.h>
#include "app_openthread.h"

/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds=1;

static int wait(int timeout)
{
    int ret = -EINVAL;

    if (nfds <= 0) {
        return ret;
    }

    ret = poll(fds, nfds, timeout);
    if (ret < 0) {
        LOG_ERR("poll error: %d", errno);
        return -errno;
    }

    return ret;
}

static void prepare_fds(void)
{
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    nfds=1;
}

static int app_coap_start_client(struct sockaddr_in6 * addr6) {
    int ret;
    sock = socket(addr6->sin6_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_ERR("Failed to create UDP socket %d", errno);
        return -errno;
    }

    ret = connect(sock, (struct sockaddr *)addr6, sizeof(*addr6));
    if (ret < 0) {
        LOG_ERR("Cannot connect to UDP remote : %d", errno);
        return -errno;
    }

    prepare_fds();

    return 0;
}


static int coap_send_request(enum coap_method method, uint8_t * uri_path,
        uint8_t * payload, uint8_t type)
{
    struct coap_packet request;
    uint8_t *data;
    int ret = 0;

    data = (uint8_t *)k_malloc(CONFIG_APP_MAX_COAP_MSG_LEN);
    if (!data) {
        return -ENOMEM;
    }

    ret = coap_packet_init(&request, data, CONFIG_APP_MAX_COAP_MSG_LEN,
                           1, type, 8, coap_next_token(),
                           method, coap_next_id());
    if (ret < 0) {
        LOG_ERR("Failed to init CoAP message");
        goto end;
    }

    uint8_t *cur   = uri_path;
    uint8_t *end;
    if(cur[0] == '/'){
        cur += 1;
    }

    while ((end = strchr(cur, '/')) != 0)
    {
        ret = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                        cur, end - cur);
        if (ret < 0) {
            LOG_ERR("Unable add option to request");
            goto end;
        }
        cur = end + 1;
    }
    ret = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                    cur, strlen(cur));
    if (ret < 0) {
        LOG_ERR("Unable add option to request");
        goto end;
    }

    switch (method) {
        case COAP_METHOD_GET:
        case COAP_METHOD_DELETE:
            break;

        case COAP_METHOD_PUT:
        case COAP_METHOD_POST:
            ret = coap_packet_append_payload_marker(&request);
            if (ret < 0) {
                LOG_ERR("Unable to append payload marker");
                goto end;
            }

            ret = coap_packet_append_payload(&request, (uint8_t *)payload,
                                             strlen(payload));
            if (ret < 0) {
                LOG_ERR("Not able to append payload");
                goto end;
            }

            break;
        default:
            ret = -EINVAL;
            goto end;
    }

    LOG_HEXDUMP_INF(request.data, request.offset, "Request");

    ret = send(sock, request.data, request.offset, 0);

    end:
    k_free(data);

    return ret;
}


static int coap_wait_and_recv(app_coap_send_cb cb)
{
    uint8_t *data;
    int rcvd;
    int ret;

    ret = wait(10 * MSEC_PER_SEC);
    if(ret < 0) {
        return ret;
    }

    data = (uint8_t *)k_malloc(CONFIG_APP_MAX_COAP_MSG_LEN);
    if (!data) {
        return -ENOMEM;
    }

    rcvd = recv(sock, data, CONFIG_APP_MAX_COAP_MSG_LEN, MSG_DONTWAIT);
    if (rcvd == 0) {
        ret = -EIO;
        goto end;
    }

    if (rcvd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = 0;
        } else {
            ret = -errno;
        }

        goto end;
    }

    LOG_HEXDUMP_INF(data, rcvd, "Response");
    if(cb){
        cb(data, rcvd);
    }

end:
    k_free(data);

    return ret;
}

int app_coap_send_request(struct sockaddr_in6 * addr6, enum coap_method method,
        uint8_t * uri_path, uint8_t * payload, uint8_t type, app_coap_send_cb cb ) {
    int ret;
    app_ot_set_power_state(OT_POWER_ACTIVE);
    ret = app_coap_start_client(addr6);
    if (ret < 0) {
        LOG_ERR("coap start error %d", ret);
        goto exit;
    }

    ret = coap_send_request(method, uri_path, payload, type);
    if (ret < 0) {
        LOG_ERR("coap send error %d", ret);
        goto exit;
    }
    if(type == COAP_TYPE_CON){
        ret = coap_wait_and_recv(cb);
        if (ret < 0) {
            LOG_ERR("coap recv error %d", ret);
            goto exit;
        }
    }

exit:
    app_ot_set_power_state(OT_POWER_SLEEPY);
    if(sock >= 0){
        (void)close(sock);
        sock = -1;
    }
    return ret;
}