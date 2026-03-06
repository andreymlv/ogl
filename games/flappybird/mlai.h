#pragma once

#include "constants.h"
#include "onnxinference.h"
#include "physics.h"

#include <array>

inline std::array<float, 5> mlComputeInputs(float birdY, float birdVy,
                                              const std::array<PipeState, kPipeCount> &pipes)
{
    // Find next pipe (same logic as aiTargetY)
    const float birdRight = kBirdX + kBirdW;
    float closestX = kWorldW + kPipeSpacing;
    const PipeState *nextPipe = &pipes[0];
    for (const auto &p : pipes) {
        const float pipeRight = p.x + kPipeW;
        if (pipeRight > birdRight && p.x < closestX) {
            closestX = p.x;
            nextPipe = &p;
        }
    }

    const float dx = nextPipe->x - kBirdX;
    const float gapTop = nextPipe->gapCenterY + kPipeGap / 2.F;
    const float gapBottom = nextPipe->gapCenterY - kPipeGap / 2.F;
    const float birdCenterY = birdY + kBirdH / 2.F;

    return {{
        birdY / kWorldH,
        (birdVy - kMaxFallSpeed) / (kFlapImpulse - kMaxFallSpeed),
        dx / kWorldW,
        (birdCenterY - gapTop) / kWorldH,
        (birdCenterY - gapBottom) / kWorldH,
    }};
}

inline bool mlShouldFlap(OnnxInference &model, float birdY, float birdVy,
                          const std::array<PipeState, kPipeCount> &pipes)
{
    return model.predict(mlComputeInputs(birdY, birdVy, pipes)) > 0.5F;
}
