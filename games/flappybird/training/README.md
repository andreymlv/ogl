# Flappy Bird ML Training

Trains a PPO agent to play Flappy Bird, then converts the weights to a C++ header for zero-dependency inference.

## Setup

```sh
cd games/flappybird/training
pip install -r requirements.txt
```

## Train

```sh
python train.py
```

Trains for ~20M timesteps. Outputs `../model.onnx`.

## Generate C++ header

```sh
python onnx2header.py
```

Reads `../model.onnx` and writes `../policyweights.h`. If `onnxruntime` is installed it also runs a quick verification pass.

## Retrain

Delete `../model.onnx` and run `python train.py` again. Then regenerate the header with `python onnx2header.py`. Adjust `TOTAL_TIMESTEPS` in `train.py` if needed.
