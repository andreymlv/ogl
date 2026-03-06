#pragma once

#include <array>
#include <memory>

class OnnxInference
{
public:
    OnnxInference();
    ~OnnxInference();

    bool initFromData(const unsigned char *data, std::size_t size);
    bool isValid() const;

    // Run inference: 5 inputs -> action logits (2 outputs: [no-flap, flap])
    // Returns probability of flap action (softmax of logit[1])
    float predict(const std::array<float, 5> &inputs);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
