
#include "at_openthread.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(at_ot, LOG_LEVEL_INF);

#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/joiner.h>
#include "../../zephyr/subsys/net/ip/icmpv6.h"
#include <net/net_ip.h>
#include <net/sntp.h>
#include <random/rand32.h>
#include "app_openthread.h"
#include "app_settings.h"
#include "app_at_modem.h"

extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_APP_AT_RESPONSE_MAX_LEN];

static enum net_verdict handle_ipv6_echo_reply(struct net_pkt *pkt,
                                               struct net_ipv6_hdr *ip_hdr,
                                               struct net_icmp_hdr *icmp_hdr);

#define SNTP_PORT 123

static struct net_icmpv6_handler ping6_handler = {
        .type = NET_ICMPV6_ECHO_REPLY,
        .code = 0,
        .handler = handle_ipv6_echo_reply,
};

/**@brief List of supported AT commands. */
enum at_openthread_type {
    AT_OPENTHREAD_EUI,
    AT_OPENTHREAD_SCAN,
    AT_OPENTHREAD_STATE,
    AT_OPENTHREAD_IPADDR,
    AT_OPENTHREAD_PING,
    AT_OPENTHREAD_SED,
    AT_OPENTHREAD_JOIN,
    AT_OPENTHREAD_SNTP,
    AT_OPENTHREAD_FAC_RESET,
    AT_OPENTHREAD_MAX
};

static int handle_at_openthread_eui(enum at_cmd_type cmd_type);
static int handle_at_openthread_scan(enum at_cmd_type cmd_type);
static int handle_at_openthread_state(enum at_cmd_type cmd_type);
static int handle_at_openthread_ipaddr(enum at_cmd_type cmd_type);
static int handle_at_openthread_ping(enum at_cmd_type cmd_type);
static int handle_at_openthread_sed(enum at_cmd_type cmd_type);
static int handle_at_openthread_join(enum at_cmd_type cmd_type);
static int handle_at_openthread_sntp(enum at_cmd_type cmd_type);
static int handle_at_openthread_factory_reset(enum at_cmd_type cmd_type);

static at_cmd_list_t m_at_cmd_openthread_list[AT_OPENTHREAD_MAX] = {
        {AT_OPENTHREAD_EUI, "AT+EUI", handle_at_openthread_eui},
        {AT_OPENTHREAD_SCAN, "AT+SCAN", handle_at_openthread_scan},
        {AT_OPENTHREAD_STATE, "AT+STATE", handle_at_openthread_state},
        {AT_OPENTHREAD_IPADDR, "AT+IPADDR", handle_at_openthread_ipaddr},
        {AT_OPENTHREAD_PING, "AT+PING", handle_at_openthread_ping},
        {AT_OPENTHREAD_SED, "AT+SED", handle_at_openthread_sed},
        {AT_OPENTHREAD_JOIN, "AT+JOIN", handle_at_openthread_join},
        {AT_OPENTHREAD_SNTP, "AT+SNTP", handle_at_openthread_sntp},
        {AT_OPENTHREAD_FAC_RESET, "AT+FRST", handle_at_openthread_factory_reset},
};

int at_openthread_parser(const char *at_cmd){
    int ret = -ENOTSUP;
    enum at_cmd_type type;

    for (int i = 0; i < AT_OPENTHREAD_MAX; i++) {
        if (app_at_modem_cmd_casecmp(at_cmd, m_at_cmd_openthread_list[i].string)) {
            ret = at_parser_params_from_str(at_cmd, NULL,
                                            &at_param_list);
            if (ret) {
                return -EINVAL;
            }
            type = at_parser_cmd_type_get(at_cmd);
            ret = m_at_cmd_openthread_list[i].handler(type);
            break;
        }
    }

    return ret;
}

static int handle_at_openthread_eui(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;

    __ASSERT(instance, "OT instance is NULL");

    char * pos = rsp_buf;
    otExtAddress extAddress;
    otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress);
    pos += sprintf(pos, "+EUI:");
    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
        pos += sprintf(pos, "%02x", extAddress.m8[i]);
    }
    pos += sprintf(pos, "\n\r");
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    return error;
}


static void handle_at_openthread_scan_result(otActiveScanResult *aResult, void *aContext) {
    char * pos = rsp_buf;
    if (aResult == 0)
    {
        return;
    }
    pos += sprintf(pos, "+SCAN:%d,%s,",
                   aResult->mIsJoinable,
                   aResult->mNetworkName.m8);
    for(int i=0; i<OT_EXT_PAN_ID_SIZE; i++){
        pos += sprintf(pos, "%02x", aResult->mExtendedPanId.m8[i]);
    }
    pos += sprintf(pos, ",%04x,", aResult->mPanId);
    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
        pos += sprintf(pos, "%02x", aResult->mExtAddress.m8[i]);
    }
    pos += sprintf(pos, ",%d,%d,%d\r\n", aResult->mChannel, aResult->mRssi, aResult->mLqi);
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
}


static int handle_at_openthread_scan(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;

    __ASSERT(instance, "OT instance is NULL");

    error = otLinkActiveScan(instance, 0, 0,
                             handle_at_openthread_scan_result, 0);
    for(int i=0; i<100; i++){
        if(otLinkIsActiveScanInProgress(instance)){
            k_msleep(100);
        }
    }
    return error;
}


static int handle_at_openthread_state(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");

    switch (otThreadGetDeviceRole(instance))
    {
        case OT_DEVICE_ROLE_DISABLED:
            sprintf(rsp_buf, "+STATE:disabled\r\n");
            break;

        case OT_DEVICE_ROLE_DETACHED:
            sprintf(rsp_buf, "+STATE:detached\r\n");
            break;

        case OT_DEVICE_ROLE_CHILD:
            sprintf(rsp_buf, "+STATE:child\r\n");
            break;

#if CONFIG_OPENTHREAD_FTD
            case OT_DEVICE_ROLE_ROUTER:
            sprintf(rsp_buf, "+STATE:router\r\n");
            break;

        case OT_DEVICE_ROLE_LEADER:
            sprintf(rsp_buf, "+STATE:leader\r\n");
            break;
#endif // OPENTHREAD_FTD
        default:
            sprintf(rsp_buf, "+STATE:invalid\r\n");
            break;
    }
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    return OT_ERROR_NONE;
}

static int handle_at_openthread_ipaddr(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;
    __ASSERT(instance, "OT instance is NULL");
    const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
    char ip_str[40];
    for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
    {
        net_addr_ntop(AF_INET6, &addr->mAddress, ip_str, 40);
        sprintf(rsp_buf, "+IPADDR:%s\r\n", ip_str);
        app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    }
    return error;
}


static enum net_verdict handle_ipv6_echo_reply(struct net_pkt *pkt,
                                               struct net_ipv6_hdr *ip_hdr,
                                               struct net_icmp_hdr *icmp_hdr)
{
    NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
                                          struct net_icmpv6_echo_req);
    struct net_icmpv6_echo_req *icmp_echo;
    uint32_t cycles;
    char ip_str[40];

    icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(pkt,
                                                               &icmp_access);
    if (icmp_echo == NULL) {
        return -NET_DROP;
    }

    net_pkt_skip(pkt, sizeof(*icmp_echo));
    if (net_pkt_read_be32(pkt, &cycles)) {
        return -NET_DROP;
    }

    cycles = k_cycle_get_32() - cycles;
    if(ip_hdr){
        net_addr_ntop(AF_INET6, &ip_hdr->src, ip_str, 40);
        sprintf(rsp_buf, "+PING:%s,%d\r\n", ip_str,ip_hdr->hop_limit);
        app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    }
    net_pkt_unref(pkt);
    return NET_OK;
}

static int handle_at_openthread_ping(enum at_cmd_type cmd_type) {
    struct net_if *iface;
    int error = 0;
    char ip_str[40];
    size_t len = 40;

    iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
    if (!iface) {
        LOG_ERR("There is no net interface for OpenThread");
        goto exit;
    }
    struct in6_addr ipv6_target;

    if (at_params_valid_count_get(&at_param_list) < 2) {
        return -EINVAL;
    }
    error = at_params_string_get(&at_param_list, 1, ip_str, &len);
    if (error < 0) {
        return error;
    }
    ip_str[len] = '\0';
    if (net_addr_pton(AF_INET6, ip_str, &ipv6_target) < 0) {
        return -EINVAL;
    }
    app_ot_set_power_state(OT_POWER_ACTIVE);
    net_icmpv6_register_handler(&ping6_handler);

    uint32_t time_stamp = htonl(k_cycle_get_32());

    error = net_icmpv6_send_echo_request(iface,
                                       &ipv6_target,
                                       sys_rand32_get(),
                                       1,
                                       &time_stamp,
                                       sizeof(time_stamp));

    k_msleep(3000);
    net_icmpv6_unregister_handler(&ping6_handler);
    app_ot_set_power_state(OT_POWER_SLEEPY);
exit:
    return error;
}


static int handle_at_openthread_sed(enum at_cmd_type cmd_type) {
    app_ot_settings ot_settings;
    uint16_t setting_len = sizeof(ot_settings);
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");
    otError  error        = OT_ERROR_NONE;
    otLinkModeConfig ot_mode = otThreadGetLinkMode(instance);
    if(ot_mode.mDeviceType){
        LOG_INF("FTD");
    }
    int ret;

    ret = app_settings_get(APP_SETTINGS_OT, (uint8_t *)&ot_settings, &setting_len);
    if(ret != 0){
        ot_settings.sed_enable = 0;
        ot_settings.poll_period_ms = CONFIG_APP_OPENTHREAD_SLEEPY_POLL_PERIOD;
        ot_settings.timeout_s = CONFIG_APP_OPENTHREAD_CHILD_TIMEOUT;
    }
    uint16_t ot_is_sed;
    uint32_t period;
    uint32_t timeout;
    switch (cmd_type) {
        case AT_CMD_TYPE_SET_COMMAND:
            if (at_params_valid_count_get(&at_param_list) < 2) {
                return -EINVAL;
            }
            error = at_params_short_get(&at_param_list, 1, &ot_is_sed);
            if (error < 0) {
                return error;
            }

            if(ot_is_sed < 0 || ot_is_sed > 1 ){
                return -EINVAL;
            }
            ot_mode.mRxOnWhenIdle = !ot_is_sed;
            error = otThreadSetLinkMode(instance, ot_mode);
            if(error != OT_ERROR_NONE){
                LOG_ERR("otThreadSetLinkMode err %d", error);
                break;
            }
            ot_settings.sed_enable = ot_is_sed;
            if(ot_is_sed){
                error = at_params_int_get(&at_param_list, 2, &period);
                if (error == 0) {
                    otLinkSetPollPeriod(instance, period);
                    ot_settings.poll_period_ms = period;
                }
                error = at_params_int_get(&at_param_list, 3, &timeout);
                if (error == 0) {
                    otThreadSetChildTimeout(instance, timeout);
                    ot_settings.timeout_s = timeout;
                }
            }
            error = app_settings_set(APP_SETTINGS_OT, (uint8_t *)&ot_settings, sizeof(ot_settings));

            break;

        case AT_CMD_TYPE_READ_COMMAND:
            sprintf(rsp_buf, "+MODE:%d,%d,%d\r\n",
                    !ot_mode.mRxOnWhenIdle, otLinkGetPollPeriod(instance), otThreadGetChildTimeout(instance));
            app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
            break;

        case AT_CMD_TYPE_TEST_COMMAND:
            sprintf(rsp_buf, "+MODE: <is_sed>,<poll_period>,<timeout>\r\n");
            app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
            break;

        default:
            break;
    }
    return error;
}


static void handle_at_openthread_join_callback(otError error, void * p_context)
{
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");
    LOG_INF("joiner callback");
    if (error == OT_ERROR_NONE)
    {
        otThreadSetEnabled(instance, true);
        sprintf(rsp_buf, "+JOIN:OK\r\n");
    }
    else
    {
        sprintf(rsp_buf, "+JION:ERROR %d\r\n", error);
    }
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
}

static int handle_at_openthread_join(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    struct openthread_context * ot_context = openthread_get_default_context();
    otError  error        = OT_ERROR_NONE;
    char pskd[32] = {0};
    size_t pskd_len = 32-1;

    __ASSERT(instance, "OT instance is NULL");
    error = otThreadSetEnabled(instance, false);
    if(error != OT_ERROR_NONE){
        return error;
    }
    if(otIp6IsEnabled(instance)){
        otIp6SetEnabled(instance, false);
    }
    otIp6SetEnabled(instance, true);
    (void)otLinkSetPanId(instance, 0xFFFF);
    if (at_params_valid_count_get(&at_param_list) <= 1) {
        return -EINVAL;
    }
    error = at_params_string_get(&at_param_list, 1, pskd, &pskd_len);
    if (error < 0) {
        return error;
    }
    pskd[pskd_len] = '\0';
    for(int i=0; i<pskd_len; i++){
        LOG_INF("%02x ", pskd[i]);
    }
    LOG_INF("start joiner with pskd %d", strlen(pskd));
    openthread_api_mutex_lock(ot_context);
    error = otJoinerStart(instance, pskd, NULL, CONFIG_APP_MANUFACTURER,
                  CONFIG_APP_MODEL_NUMBER, CONFIG_APP_FIRMWARE_VER, NULL,
                  handle_at_openthread_join_callback, instance);
    openthread_api_mutex_unlock(ot_context);
    if(error != OT_ERROR_NONE){
        sprintf(rsp_buf, "+JION:ERROR %d\r\n", error);
        app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
        return error;
    }
    k_msleep(3000);
    error = -OT_ERROR_RESPONSE_TIMEOUT;
    for(int i=0; i< 60; i++){
        if(OT_JOINER_STATE_IDLE != otJoinerGetState(instance)){
            k_msleep(1000);
        }
        else{
            error = OT_ERROR_NONE;
            break;
        }
    }
    return error;
}

static int handle_at_openthread_sntp(enum at_cmd_type cmd_type) {
    struct sntp_ctx ctx;
    struct sockaddr_in6 addr6;
    struct sntp_time sntp_time;
    int ret;
    char ip_str[40];
    size_t len = 40;

    /* ipv6 */
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(SNTP_PORT);

    if ((at_params_valid_count_get(&at_param_list) >= 2 )
        && (at_params_string_get(&at_param_list, 1, ip_str, &len)==0)
    ) {
        ip_str[len] = '\0';
        LOG_INF("GOT SNTP IP %s", log_strdup(ip_str));
    }
    else{
        strcpy(ip_str, "64:ff9b::b65c:c0b");
    }
    if(!inet_pton(AF_INET6, ip_str, &addr6.sin6_addr)){
        return -1;
    }

    ret = sntp_init(&ctx, (struct sockaddr *) &addr6,
                   sizeof(struct sockaddr_in6));
    if (ret < 0) {
        LOG_ERR("Failed to init SNTP IPv6 ctx: %d", ret);
        goto end;
    }

    LOG_INF("Sending SNTP IPv6 request...");
    app_ot_set_power_state(OT_POWER_ACTIVE);
    /* With such a timeout, this is expected to fail. */
    ret = sntp_query(&ctx, 3000, &sntp_time);
    if (ret < 0) {
        LOG_ERR("SNTP IPv6 request: %d", ret);
        goto end;
    }

    LOG_INF("status: %d", ret);
    LOG_INF("time since Epoch: high word: %u, low word: %u",
            (uint32_t)(sntp_time.seconds >> 32), (uint32_t)sntp_time.seconds);

    sprintf(rsp_buf, "+SNTP:%u,%u\r\n", (uint32_t)(sntp_time.seconds >> 32), (uint32_t)sntp_time.seconds);
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
end:
    sntp_close(&ctx);
    app_ot_set_power_state(OT_POWER_SLEEPY);
    return ret;
}

static int handle_at_openthread_factory_reset(enum at_cmd_type cmd_type) {
    otInstance * instance = openthread_get_default_instance();
    sprintf(rsp_buf, "OK\r\n");
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    (void)k_msleep(10);
    otInstanceFactoryReset(instance);
    return 0;
}