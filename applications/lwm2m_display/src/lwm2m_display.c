//
// Created by yplam on 4/1/2022.
//

#include "lwm2m_display.h"
#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lwm2m_display, LOG_LEVEL_INF);
#include <init.h>
#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define MAX_INSTANCE_COUNT 1

/* resource state variables */
static float32_value_t sensor_value[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj display;
static struct lwm2m_engine_obj_field fields[] = {
        OBJ_FIELD_DATA(26241, RW, FLOAT),
};


static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][1];
static struct lwm2m_engine_res_inst
        res_inst[MAX_INSTANCE_COUNT][1];


static int display_post_write_cb(uint16_t obj_inst_id,
                               uint16_t res_id, uint16_t res_inst_id,
                               uint8_t *data, uint16_t data_len,
                               bool last_block, size_t total_size)
{
    LOG_INF("write data");
    return 0;
}

static struct lwm2m_engine_obj_inst *display_create(uint16_t obj_inst_id)
{
    LOG_INF("create display");
    int index = 0, i = 0, j = 0;
    if(inst[index].obj){
        return NULL;
    }
    (void)memset(res[index], 0,
                 sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* initialize instance resource data */
    INIT_OBJ_RES(26241, res[index], i,
                 res_inst[index], j, 1, false, true,
                 &sensor_value[index], sizeof(*sensor_value),
                 NULL, NULL, NULL, display_post_write_cb, NULL);
    inst[index].resources = res[index];
    inst[index].resource_count = i;
    LOG_INF("create display ok");
    return &inst[index];
}

static int lwm2m_display_init(const struct device *dev)
{
    display.obj_id = 32769;
    display.version_major = 1;
    display.version_minor = 0;
    display.is_core = false;
    display.fields = fields;
    display.field_count = ARRAY_SIZE(fields);
    display.max_instance_count = 1;
    display.create_cb = display_create;
    lwm2m_register_obj(&display);

    return 0;
}

SYS_INIT(lwm2m_display_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
