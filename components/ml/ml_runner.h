#pragma once
#include <stdbool.h>

#include "ml_window.h"

bool ml_init(void);
bool ml_run_inference(const float *input, size_t input_len, ml_result_t *out);
