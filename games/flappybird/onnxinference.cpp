// Exceptions enabled for this file via kde_source_files_enable_exceptions()

#include "onnxinference.h"

#include <onnxruntime_cxx_api.h>

#include <cmath>

struct OnnxInference::Private {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "flappybird"};
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    bool valid = false;
};

OnnxInference::OnnxInference()
    : d(std::make_unique<Private>())
{
}

OnnxInference::~OnnxInference() = default;

bool OnnxInference::initFromData(const unsigned char *data, std::size_t size)
{
    try {
        d->sessionOptions.SetIntraOpNumThreads(1);
        d->sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        d->session = std::make_unique<Ort::Session>(d->env, data, size, d->sessionOptions);
        d->valid = true;
    } catch (const Ort::Exception &e) {
        d->valid = false;
    } catch (...) {
        d->valid = false;
    }
    return d->valid;
}

bool OnnxInference::isValid() const
{
    return d->valid;
}

float OnnxInference::predict(const std::array<float, 5> &inputs)
{
    if (!d->valid) {
        return 0.F;
    }

    try {
        const std::array<int64_t, 2> inputShape{1, 5};
        auto inputTensor = Ort::Value::CreateTensor<float>(
            d->memoryInfo,
            const_cast<float *>(inputs.data()),
            inputs.size(),
            inputShape.data(),
            inputShape.size());

        const char *inputName = "obs";
        const char *outputName = "action_logits";

        auto results = d->session->Run(
            Ort::RunOptions{nullptr},
            &inputName, &inputTensor, 1,
            &outputName, 1);

        const float *logits = results[0].GetTensorData<float>();
        // Softmax of logit[1] (flap action)
        const float maxLogit = std::max(logits[0], logits[1]);
        const float exp0 = std::exp(logits[0] - maxLogit);
        const float exp1 = std::exp(logits[1] - maxLogit);
        return exp1 / (exp0 + exp1);
    } catch (...) {
        return 0.F;
    }
}
