#include "ml_runner.h"
#include "model_data.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char* TAG = "ml";

namespace {
constexpr size_t kTensorArenaSize = 64 * 1024;
static uint8_t* s_tensor_arena = nullptr;
static size_t s_tensor_arena_size = 0;
static tflite::MicroInterpreter* s_interpreter = nullptr;
static TfLiteTensor* s_input = nullptr;
static TfLiteTensor* s_output = nullptr;
static size_t s_input_elements = 0;
static size_t s_output_elements = 0;
static TfLiteType s_input_type = kTfLiteNoType;
static TfLiteType s_output_type = kTfLiteNoType;
static float s_input_scale = 1.0f;
static int32_t s_input_zero = 0;
static float s_output_scale = 1.0f;
static int32_t s_output_zero = 0;
static bool s_inited = false;

static bool register_model_ops(const tflite::Model* model,
                               tflite::MicroMutableOpResolver<64>* resolver) {
    if (!model || !model->operator_codes() || !resolver) {
        return false;
    }

    bool ok = true;
    for (uint32_t i = 0; i < model->operator_codes()->size(); ++i) {
        const auto* opcode = model->operator_codes()->Get(i);
        tflite::BuiltinOperator op = opcode->builtin_code();
        if (op == tflite::BuiltinOperator_CUSTOM) {
            op = static_cast<tflite::BuiltinOperator>(opcode->deprecated_builtin_code());
        }

        TfLiteStatus st = kTfLiteOk;
        switch (op) {
            case tflite::BuiltinOperator_ADD: st = resolver->AddAdd(); break;
            case tflite::BuiltinOperator_ADD_N: st = resolver->AddAddN(); break;
            case tflite::BuiltinOperator_ARG_MAX: st = resolver->AddArgMax(); break;
            case tflite::BuiltinOperator_ARG_MIN: st = resolver->AddArgMin(); break;
            case tflite::BuiltinOperator_AVERAGE_POOL_2D: st = resolver->AddAveragePool2D(); break;
            case tflite::BuiltinOperator_BATCH_MATMUL: st = resolver->AddBatchMatMul(); break;
            case tflite::BuiltinOperator_BATCH_TO_SPACE_ND: st = resolver->AddBatchToSpaceNd(); break;
            case tflite::BuiltinOperator_BROADCAST_ARGS: st = resolver->AddBroadcastArgs(); break;
            case tflite::BuiltinOperator_BROADCAST_TO: st = resolver->AddBroadcastTo(); break;
            case tflite::BuiltinOperator_CAST: st = resolver->AddCast(); break;
            case tflite::BuiltinOperator_CEIL: st = resolver->AddCeil(); break;
            case tflite::BuiltinOperator_CONCATENATION: st = resolver->AddConcatenation(); break;
            case tflite::BuiltinOperator_CONV_2D: st = resolver->AddConv2D(); break;
            case tflite::BuiltinOperator_DEPTHWISE_CONV_2D: st = resolver->AddDepthwiseConv2D(); break;
            case tflite::BuiltinOperator_DEQUANTIZE: st = resolver->AddDequantize(); break;
            case tflite::BuiltinOperator_DIV: st = resolver->AddDiv(); break;
            case tflite::BuiltinOperator_EQUAL: st = resolver->AddEqual(); break;
            case tflite::BuiltinOperator_EXP: st = resolver->AddExp(); break;
            case tflite::BuiltinOperator_EXPAND_DIMS: st = resolver->AddExpandDims(); break;
            case tflite::BuiltinOperator_FILL: st = resolver->AddFill(); break;
            case tflite::BuiltinOperator_FLOOR: st = resolver->AddFloor(); break;
            case tflite::BuiltinOperator_FLOOR_DIV: st = resolver->AddFloorDiv(); break;
            case tflite::BuiltinOperator_FLOOR_MOD: st = resolver->AddFloorMod(); break;
            case tflite::BuiltinOperator_FULLY_CONNECTED: st = resolver->AddFullyConnected(); break;
            case tflite::BuiltinOperator_GATHER: st = resolver->AddGather(); break;
            case tflite::BuiltinOperator_GATHER_ND: st = resolver->AddGatherNd(); break;
            case tflite::BuiltinOperator_GREATER: st = resolver->AddGreater(); break;
            case tflite::BuiltinOperator_GREATER_EQUAL: st = resolver->AddGreaterEqual(); break;
            case tflite::BuiltinOperator_HARD_SWISH: st = resolver->AddHardSwish(); break;
            case tflite::BuiltinOperator_L2_NORMALIZATION: st = resolver->AddL2Normalization(); break;
            case tflite::BuiltinOperator_L2_POOL_2D: st = resolver->AddL2Pool2D(); break;
            case tflite::BuiltinOperator_LEAKY_RELU: st = resolver->AddLeakyRelu(); break;
            case tflite::BuiltinOperator_LESS: st = resolver->AddLess(); break;
            case tflite::BuiltinOperator_LESS_EQUAL: st = resolver->AddLessEqual(); break;
            case tflite::BuiltinOperator_LOG: st = resolver->AddLog(); break;
            case tflite::BuiltinOperator_LOGICAL_AND: st = resolver->AddLogicalAnd(); break;
            case tflite::BuiltinOperator_LOGICAL_NOT: st = resolver->AddLogicalNot(); break;
            case tflite::BuiltinOperator_LOGICAL_OR: st = resolver->AddLogicalOr(); break;
            case tflite::BuiltinOperator_LOGISTIC: st = resolver->AddLogistic(); break;
            case tflite::BuiltinOperator_LOG_SOFTMAX: st = resolver->AddLogSoftmax(); break;
            case tflite::BuiltinOperator_MAXIMUM: st = resolver->AddMaximum(); break;
            case tflite::BuiltinOperator_MAX_POOL_2D: st = resolver->AddMaxPool2D(); break;
            case tflite::BuiltinOperator_MEAN: st = resolver->AddMean(); break;
            case tflite::BuiltinOperator_MINIMUM: st = resolver->AddMinimum(); break;
            case tflite::BuiltinOperator_MUL: st = resolver->AddMul(); break;
            case tflite::BuiltinOperator_NEG: st = resolver->AddNeg(); break;
            case tflite::BuiltinOperator_NOT_EQUAL: st = resolver->AddNotEqual(); break;
            case tflite::BuiltinOperator_PACK: st = resolver->AddPack(); break;
            case tflite::BuiltinOperator_PAD: st = resolver->AddPad(); break;
            case tflite::BuiltinOperator_PADV2: st = resolver->AddPadV2(); break;
            case tflite::BuiltinOperator_PRELU: st = resolver->AddPrelu(); break;
            case tflite::BuiltinOperator_QUANTIZE: st = resolver->AddQuantize(); break;
            case tflite::BuiltinOperator_REDUCE_MAX: st = resolver->AddReduceMax(); break;
            case tflite::BuiltinOperator_REDUCE_MIN: st = resolver->AddReduceMin(); break;
            case tflite::BuiltinOperator_RELU: st = resolver->AddRelu(); break;
            case tflite::BuiltinOperator_RELU6: st = resolver->AddRelu6(); break;
            case tflite::BuiltinOperator_RESHAPE: st = resolver->AddReshape(); break;
            case tflite::BuiltinOperator_RESIZE_BILINEAR: st = resolver->AddResizeBilinear(); break;
            case tflite::BuiltinOperator_RESIZE_NEAREST_NEIGHBOR: st = resolver->AddResizeNearestNeighbor(); break;
            case tflite::BuiltinOperator_REVERSE_V2: st = resolver->AddReverseV2(); break;
            case tflite::BuiltinOperator_ROUND: st = resolver->AddRound(); break;
            case tflite::BuiltinOperator_RSQRT: st = resolver->AddRsqrt(); break;
            case tflite::BuiltinOperator_SELECT_V2: st = resolver->AddSelectV2(); break;
            case tflite::BuiltinOperator_SHAPE: st = resolver->AddShape(); break;
            case tflite::BuiltinOperator_SLICE: st = resolver->AddSlice(); break;
            case tflite::BuiltinOperator_SOFTMAX: st = resolver->AddSoftmax(); break;
            case tflite::BuiltinOperator_SPACE_TO_BATCH_ND: st = resolver->AddSpaceToBatchNd(); break;
            case tflite::BuiltinOperator_SPACE_TO_DEPTH: st = resolver->AddSpaceToDepth(); break;
            case tflite::BuiltinOperator_SPLIT: st = resolver->AddSplit(); break;
            case tflite::BuiltinOperator_SPLIT_V: st = resolver->AddSplitV(); break;
            case tflite::BuiltinOperator_SQRT: st = resolver->AddSqrt(); break;
            case tflite::BuiltinOperator_SQUARE: st = resolver->AddSquare(); break;
            case tflite::BuiltinOperator_SQUARED_DIFFERENCE: st = resolver->AddSquaredDifference(); break;
            case tflite::BuiltinOperator_SQUEEZE: st = resolver->AddSqueeze(); break;
            case tflite::BuiltinOperator_STRIDED_SLICE: st = resolver->AddStridedSlice(); break;
            case tflite::BuiltinOperator_SUB: st = resolver->AddSub(); break;
            case tflite::BuiltinOperator_SUM: st = resolver->AddSum(); break;
            case tflite::BuiltinOperator_SVDF: st = resolver->AddSvdf(); break;
            case tflite::BuiltinOperator_TANH: st = resolver->AddTanh(); break;
            case tflite::BuiltinOperator_TRANSPOSE: st = resolver->AddTranspose(); break;
            case tflite::BuiltinOperator_TRANSPOSE_CONV: st = resolver->AddTransposeConv(); break;
            case tflite::BuiltinOperator_UNPACK: st = resolver->AddUnpack(); break;
            case tflite::BuiltinOperator_UNIDIRECTIONAL_SEQUENCE_LSTM:
                st = resolver->AddUnidirectionalSequenceLSTM();
                break;
            case tflite::BuiltinOperator_ZEROS_LIKE: st = resolver->AddZerosLike(); break;
            default:
                ESP_LOGE(TAG, "Unsupported op code %d", (int)op);
                ok = false;
                continue;
        }

        if (st != kTfLiteOk) {
            ESP_LOGE(TAG, "Op add failed %d", (int)op);
            ok = false;
        }
    }

    return ok;
}

static size_t tensor_element_count(const TfLiteTensor* t) {
    if (!t || !t->dims) {
        return 0;
    }
    size_t count = 1;
    for (int i = 0; i < t->dims->size; ++i) {
        if (t->dims->data[i] <= 0) {
            return 0;
        }
        count *= (size_t)t->dims->data[i];
    }
    return count;
}
} // namespace

bool ml_init(void) {
    if (s_inited) {
        return true;
    }

    if (!s_tensor_arena) {
        size_t free_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        if (esp_psram_is_initialized()) {
            ESP_LOGI(TAG, "psram size=%u bytes", (unsigned)esp_psram_get_size());
            s_tensor_arena = (uint8_t*)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
        }
        if (!s_tensor_arena) {
            s_tensor_arena = (uint8_t*)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_8BIT);
        }
        if (!s_tensor_arena) {
            ESP_LOGE(TAG, "tensor arena alloc failed, free=%u bytes", (unsigned)free_before);
            return false;
        }
        s_tensor_arena_size = kTensorArenaSize;
        ESP_LOGI(TAG, "tensor arena alloc=%u bytes, free=%u bytes",
                 (unsigned)s_tensor_arena_size,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
    }

    const tflite::Model* model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema %" PRIu32 " != supported %" PRIu32,
                 (uint32_t)model->version(), (uint32_t)TFLITE_SCHEMA_VERSION);
        return false;
    }

    static tflite::MicroMutableOpResolver<64> resolver;
    if (!register_model_ops(model, &resolver)) {
        ESP_LOGE(TAG, "Op resolver init failed");
        return false;
    }
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, s_tensor_arena, s_tensor_arena_size);
    s_interpreter = &static_interpreter;

    if (s_interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed");
        return false;
    }

    s_input = s_interpreter->input(0);
    s_output = s_interpreter->output(0);
    s_input_elements = tensor_element_count(s_input);
    s_output_elements = tensor_element_count(s_output);
    s_input_type = s_input ? s_input->type : kTfLiteNoType;
    s_output_type = s_output ? s_output->type : kTfLiteNoType;
    if (s_input) {
        s_input_scale = s_input->params.scale;
        s_input_zero = s_input->params.zero_point;
    }
    if (s_output) {
        s_output_scale = s_output->params.scale;
        s_output_zero = s_output->params.zero_point;
    }

    ESP_LOGI(TAG, "Model loaded OK, size=%u bytes", (unsigned)g_model_len);
    ESP_LOGI(TAG, "input elems=%u type=%d scale=%.6f zero=%d",
             (unsigned)s_input_elements, (int)s_input_type, s_input_scale, (int)s_input_zero);
    ESP_LOGI(TAG, "output elems=%u type=%d scale=%.6f zero=%d",
             (unsigned)s_output_elements, (int)s_output_type, s_output_scale, (int)s_output_zero);
    s_inited = true;
    return true;
}

bool ml_run_inference(const float *input, size_t input_len, ml_result_t *out)
{
    if (!s_interpreter || !s_input || !s_output || !input || !out) {
        return false;
    }
    if (input_len != s_input_elements) {
        ESP_LOGE(TAG, "input len mismatch: got=%u need=%u",
                 (unsigned)input_len, (unsigned)s_input_elements);
        return false;
    }

    if (s_input_type == kTfLiteFloat32) {
        memcpy(s_input->data.f, input, input_len * sizeof(float));
    } else if (s_input_type == kTfLiteInt8) {
        for (size_t i = 0; i < input_len; ++i) {
            int32_t q = (int32_t)lroundf(input[i] / s_input_scale) + s_input_zero;
            if (q > 127) q = 127;
            if (q < -128) q = -128;
            s_input->data.int8[i] = (int8_t)q;
        }
    } else if (s_input_type == kTfLiteUInt8) {
        for (size_t i = 0; i < input_len; ++i) {
            int32_t q = (int32_t)lroundf(input[i] / s_input_scale) + s_input_zero;
            if (q > 255) q = 255;
            if (q < 0) q = 0;
            s_input->data.uint8[i] = (uint8_t)q;
        }
    } else {
        ESP_LOGE(TAG, "unsupported input type %d", (int)s_input_type);
        return false;
    }

    if (s_interpreter->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke failed");
        return false;
    }

    if (s_output_elements == 0) {
        ESP_LOGE(TAG, "output empty");
        return false;
    }

    auto dequant = [&](int32_t v) -> float {
        return (float)(v - s_output_zero) * s_output_scale;
    };

    size_t count = s_output_elements;
    if (count > ML_MAX_CLASSES) {
        count = ML_MAX_CLASSES;
    }
    for (size_t i = 0; i < ML_MAX_CLASSES; ++i) {
        out->probs[i] = 0.0f;
    }

    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float v = 0.0f;
        if (s_output_type == kTfLiteFloat32) {
            v = s_output->data.f[i];
        } else if (s_output_type == kTfLiteInt8) {
            v = dequant(s_output->data.int8[i]);
        } else if (s_output_type == kTfLiteUInt8) {
            v = dequant(s_output->data.uint8[i]);
        } else {
            ESP_LOGE(TAG, "unsupported output type %d", (int)s_output_type);
            return false;
        }

        if (v < 0.0f) {
            v = 0.0f;
        }
        out->probs[i] = v;
        sum += v;
    }

    if (count > 0) {
        if (sum > 0.0f) {
            float inv = 1.0f / sum;
            for (size_t i = 0; i < count; ++i) {
                out->probs[i] *= inv;
            }
            sum = 1.0f;
        } else {
            float uniform = 1.0f / (float)count;
            for (size_t i = 0; i < count; ++i) {
                out->probs[i] = uniform;
            }
            sum = 1.0f;
        }
    }

    int best_idx = 0;
    float best = out->probs[0];
    float second = 0.0f;
    for (size_t i = 1; i < count; ++i) {
        float v = out->probs[i];
        if (v > best) {
            second = best;
            best = v;
            best_idx = (int)i;
        } else if (v > second) {
            second = v;
        }
    }
    out->pred = best_idx;

    static int64_t s_last_log_us = 0;
    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_log_us >= 5000000LL) {
        float margin = best - second;
        ESP_LOGI(TAG, "pred_raw=%d conf=%.3f margin=%.3f sum=%.3f",
                 out->pred, best, margin, sum);
        s_last_log_us = now_us;
    }
    return true;
}
