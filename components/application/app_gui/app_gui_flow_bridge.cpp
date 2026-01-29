#include <stdint.h>

#include "eez-flow.h"

extern "C" void app_gui_set_flow_var_int(int32_t var_id, int32_t value)
{
    eez::flow::setGlobalVariable((uint32_t)var_id, eez::IntegerValue(value));
}

extern "C" void app_gui_set_flow_var_string(int32_t var_id, const char *value)
{
    if (!value) {
        return;
    }
    eez::flow::setGlobalVariable((uint32_t)var_id, eez::StringValue(value));
}

extern "C" int32_t app_gui_get_flow_var_int(int32_t var_id, int32_t default_value)
{
    eez::Value v = eez::flow::getGlobalVariable((uint32_t)var_id);
    if (v.type == eez::VALUE_TYPE_UNDEFINED) {
        return default_value;
    }
    int err = 0;
    int32_t out = v.toInt32(&err);
    return err ? default_value : out;
}
