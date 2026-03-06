# Flappy Bird ML Training

Trains a PPO agent to play Flappy Bird, then exports to ONNX for C++ inference.

## Setup

```sh
cd games/flappybird/training
pip install -r requirements.txt
```

## Train

```sh
python train.py
```

Trains for ~500K timesteps (~2-5 min). Outputs `../model.onnx`.

## Retrain

Delete `../model.onnx` and run `python train.py` again. Adjust `TOTAL_TIMESTEPS` in `train.py` if needed.
