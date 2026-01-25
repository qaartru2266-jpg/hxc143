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
