#include "app_testing.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(app_testing, LOG_LEVEL_INF);

#include <zephyr.h>
#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/joiner.h>


static void handle_test_openthread_scan_result(otActiveScanResult *aResult, void *aContext) {
    if (!aResult)
    {
        return;
    }
    LOG_INF("scan: %d,%s,%d,%d,%d", aResult->mIsJoinable, log_strdup(aResult->mNetworkName.m8),
            aResult->mChannel, aResult->mRssi, aResult->mLqi);
}

void test_openthread(void) {
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;

    __ASSERT(instance, "OT instance is NULL");

    error = otLinkActiveScan(instance, 0, 0,
                             handle_test_openthread_scan_result, 0);
    for(int i=0; i<100; i++){
        if(otLinkIsActiveScanInProgress(instance)){
            (void)k_msleep(100);
        }
    }
}

void app_testing(void) {
    LOG_INF("Start testing...");
    test_openthread();
}