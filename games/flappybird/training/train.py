#!/usr/bin/env python3
"""Train a PPO agent to play Flappy Bird, export to ONNX."""

import sys
from pathlib import Path

import gymnasium as gym
import numpy as np
import torch
from gymnasium import spaces
from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import SubprocVecEnv

# ── Constants (must match constants.h exactly) ──────────────────────────────

WORLD_W = 288.0
WORLD_H = 512.0
BIRD_W = 34.0
BIRD_H = 24.0
BIRD_X = 60.0
BIRD_START_Y = 256.0
GRAVITY = -800.0
FLAP_IMPULSE = 280.0
MAX_FALL_SPEED = -400.0
PIPE_W = 52.0
PIPE_H = 320.0
PIPE_GAP = 100.0
PIPE_SPEED = 120.0
PIPE_SPACING = 180.0
PIPE_MIN_GAP_Y = 180.0
PIPE_MAX_GAP_Y = 350.0
PIPE_COUNT = 3
BASE_H = 112.0
FLOOR_Y = BASE_H

DT = 1.0 / 60.0
MAX_STEPS = 10_000
TOTAL_TIMESTEPS = 20_000_000
N_ENVS = 16  # parallel environments


# ── AABB collision (matches physics.h) ──────────────────────────────────────

def check_aabb(ax, ay, aw, ah, bx, by, bw, bh):
    return ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by


# ── Gymnasium environment ───────────────────────────────────────────────────

class FlappyBirdEnv(gym.Env):
    metadata = {"render_modes": []}

    def __init__(self):
        super().__init__()
        self.action_space = spaces.Discrete(2)  # 0=noop, 1=flap
        self.observation_space = spaces.Box(
            low=-1.0, high=1.0, shape=(5,), dtype=np.float32
        )
        self.rng = np.random.default_rng()
        self.reset()

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)
        if seed is not None:
            self.rng = np.random.default_rng(seed)

        self.bird_y = BIRD_START_Y
        self.bird_vy = 0.0
        self.pipes = []
        for i in range(PIPE_COUNT):
            self.pipes.append({
                "x": WORLD_W + i * PIPE_SPACING,
                "gap_center_y": self.rng.uniform(PIPE_MIN_GAP_Y, PIPE_MAX_GAP_Y),
                "scored": False,
            })
        self.step_count = 0
        self.score = 0
        return self._obs(), {}

    def _obs(self):
        # Find next pipe (same logic as aiTargetY)
        bird_right = BIRD_X + BIRD_W
        closest_x = WORLD_W + PIPE_SPACING
        next_pipe = self.pipes[0]
        for p in self.pipes:
            pipe_right = p["x"] + PIPE_W
            if pipe_right > bird_right and p["x"] < closest_x:
                closest_x = p["x"]
                next_pipe = p

        dx = next_pipe["x"] - BIRD_X
        gap_top = next_pipe["gap_center_y"] + PIPE_GAP / 2.0
        gap_bottom = next_pipe["gap_center_y"] - PIPE_GAP / 2.0
        bird_center_y = self.bird_y + BIRD_H / 2.0

        return np.array([
            self.bird_y / WORLD_H,
            (self.bird_vy - MAX_FALL_SPEED) / (FLAP_IMPULSE - MAX_FALL_SPEED),
            dx / WORLD_W,
            (bird_center_y - gap_top) / WORLD_H,
            (bird_center_y - gap_bottom) / WORLD_H,
        ], dtype=np.float32)

    def step(self, action):
        # Flap
        if action == 1:
            self.bird_vy = FLAP_IMPULSE

        # Physics (matches updateBird)
        self.bird_vy += GRAVITY * DT
        self.bird_vy = max(self.bird_vy, MAX_FALL_SPEED)
        self.bird_y += self.bird_vy * DT

        # Move pipes (matches updatePipes)
        for p in self.pipes:
            p["x"] -= PIPE_SPEED * DT
            if p["x"] + PIPE_W < 0.0:
                max_x = max(pp["x"] for pp in self.pipes)
                p["x"] = max_x + PIPE_SPACING
                p["gap_center_y"] = self.rng.uniform(PIPE_MIN_GAP_Y, PIPE_MAX_GAP_Y)
                p["scored"] = False

        # Scoring (matches checkScore)
        reward = 1.0  # alive bonus per frame
        bird_center_x = BIRD_X + BIRD_W / 2.0
        for p in self.pipes:
            if not p["scored"] and bird_center_x > p["x"] + PIPE_W:
                p["scored"] = True
                self.score += 1
                reward += 10.0

        # Collision (matches checkCollisions with 3px shrink)
        bx = BIRD_X + 3.0
        by = self.bird_y + 3.0
        bw = BIRD_W - 6.0
        bh = BIRD_H - 6.0
        dead = False

        if self.bird_y <= FLOOR_Y or self.bird_y + BIRD_H >= WORLD_H:
            dead = True

        if not dead:
            for p in self.pipes:
                gap_top = p["gap_center_y"] + PIPE_GAP / 2.0
                gap_bottom = p["gap_center_y"] - PIPE_GAP / 2.0
                # Top pipe
                if check_aabb(bx, by, bw, bh, p["x"], gap_top, PIPE_W, PIPE_H):
                    dead = True
                    break
                # Bottom pipe
                if check_aabb(bx, by, bw, bh, p["x"], gap_bottom - PIPE_H, PIPE_W, PIPE_H):
                    dead = True
                    break

        self.step_count += 1
        truncated = self.step_count >= MAX_STEPS

        if dead:
            reward = -10.0

        return self._obs(), reward, dead, truncated, {}


# ── Training ────────────────────────────────────────────────────────────────

def main():
    output_path = Path(__file__).parent.parent / "model.onnx"

    if not torch.cuda.is_available():
        print("ERROR: CUDA not available. Training requires a GPU.")
        return 1
    device = "cuda"
    print(f"Using device: {device}")

    print(f"Creating {N_ENVS} parallel environments...")
    env = SubprocVecEnv([lambda: FlappyBirdEnv() for _ in range(N_ENVS)])

    print(f"Training PPO for {TOTAL_TIMESTEPS} timesteps...")
    model = PPO(
        "MlpPolicy",
        env,
        verbose=1,
        device=device,
        learning_rate=3e-4,
        n_steps=2048,
        batch_size=256,
        n_epochs=10,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.2,
        policy_kwargs=dict(net_arch=[128, 128]),
    )
    model.learn(total_timesteps=TOTAL_TIMESTEPS)

    # Evaluate
    print("Evaluating...")
    test_env = FlappyBirdEnv()
    scores = []
    for _ in range(10):
        obs, _ = test_env.reset()
        total_reward = 0
        for _ in range(MAX_STEPS):
            action, _ = model.predict(obs, deterministic=True)
            obs, reward, done, truncated, _ = test_env.step(int(action))
            total_reward += reward
            if done or truncated:
                break
        scores.append(test_env.score)
    print(f"Eval scores (10 episodes): {scores}")
    print(f"Mean score: {np.mean(scores):.1f}")

    # Export to ONNX
    print(f"Exporting to {output_path}...")
    policy = model.policy
    policy.eval()
    policy.cpu()

    class PolicyWrapper(torch.nn.Module):
        def __init__(self, policy):
            super().__init__()
            self.features_extractor = policy.features_extractor
            self.mlp_extractor = policy.mlp_extractor
            self.action_net = policy.action_net

        def forward(self, obs):
            features = self.features_extractor(obs)
            latent = self.mlp_extractor.forward_actor(features)
            return self.action_net(latent)

    wrapper = PolicyWrapper(policy)
    wrapper.eval()

    dummy_obs = torch.zeros(1, 5)
    torch.onnx.export(
        wrapper,
        dummy_obs,
        str(output_path),
        input_names=["obs"],
        output_names=["action_logits"],
        dynamic_axes={"obs": {0: "batch"}, "action_logits": {0: "batch"}},
        opset_version=11,
    )

    # Convert external data to inline (torch may export weights to model.onnx.data)
    import onnx
    onnx_model = onnx.load(str(output_path), load_external_data=True)
    onnx.save(onnx_model, str(output_path), save_as_external_data=False)

    # Remove leftover external data file
    data_path = output_path.with_suffix(".onnx.data")
    if data_path.exists():
        data_path.unlink()

    print(f"Done! Model saved to {output_path}")
    print(f"Model size: {output_path.stat().st_size} bytes")

    onnx.checker.check_model(onnx_model)
    print("ONNX model validated successfully")

    return 0


if __name__ == "__main__":
    sys.exit(main())
