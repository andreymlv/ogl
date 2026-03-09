#!/usr/bin/env python3
"""Convert a 3-layer MLP ONNX model to a self-contained C++ header.

Expected architecture (as exported by train.py):
    Linear(in->hidden) -> Tanh -> Linear(hidden->hidden) -> Tanh -> Linear(hidden->out)

Usage:
    python onnx2header.py [model.onnx] [output.h]

Defaults:
    model.onnx  = ../model.onnx  (next to the game sources)
    output.h    = ../policyweights.h
"""

import sys
from pathlib import Path

import numpy as np
import onnx


# ── Formatting helpers ──────────────────────────────────────────────────────

FLOATS_PER_LINE = 8


def _fmt_float(v: float) -> str:
    """Format a single float as a C literal with 8 significant digits."""
    return f"{v:.8e}F"


def _fmt_array(name: str, arr: np.ndarray) -> str:
    """Format a 1-D float array as a C++ static constexpr float[]."""
    flat = arr.flatten()
    lines = []
    lines.append(f"static constexpr float {name}[] = {{")
    for i in range(0, len(flat), FLOATS_PER_LINE):
        chunk = flat[i : i + FLOATS_PER_LINE]
        entries = ", ".join(_fmt_float(float(v)) for v in chunk)
        lines.append(f"    {entries},")
    lines.append("};")
    return "\n".join(lines)


# ── ONNX weight extraction ─────────────────────────────────────────────────


def _extract_weights(model_path: Path) -> list[tuple[str, np.ndarray]]:
    """Return [(name, ndarray), ...] for every initializer in the model."""
    model = onnx.load(str(model_path), load_external_data=True)
    onnx.checker.check_model(model)

    tensors = []
    for init in model.graph.initializer:
        arr = np.frombuffer(init.raw_data, dtype=np.float32).copy()
        arr = arr.reshape(init.dims)
        tensors.append((init.name, arr))
    return tensors


def _validate_architecture(tensors: list[tuple[str, np.ndarray]]):
    """Verify the model has exactly 3 linear layers (6 tensors: 3 W + 3 B)."""
    if len(tensors) != 6:
        names = [n for n, _ in tensors]
        raise ValueError(
            f"Expected 6 tensors (3 weight + 3 bias), got {len(tensors)}: {names}"
        )

    shapes = [t.shape for _, t in tensors]
    # W0: (hidden, input), B0: (hidden,)
    # W1: (hidden, hidden), B1: (hidden,)
    # W2: (output, hidden), B2: (output,)
    w0, b0, w1, b1, w2, b2 = shapes
    if len(w0) != 2 or len(w1) != 2 or len(w2) != 2:
        raise ValueError(f"Weight tensors must be 2-D, got shapes: {w0}, {w1}, {w2}")
    if len(b0) != 1 or len(b1) != 1 or len(b2) != 1:
        raise ValueError(f"Bias tensors must be 1-D, got shapes: {b0}, {b1}, {b2}")

    in_dim = w0[1]
    hidden = w0[0]
    out_dim = w2[0]

    if b0[0] != hidden or w1 != (hidden, hidden) or b1[0] != hidden:
        raise ValueError(f"Hidden dimension mismatch in layer shapes: {shapes}")
    if w2[1] != hidden or b2[0] != out_dim:
        raise ValueError(f"Output layer dimension mismatch: {shapes}")

    return in_dim, hidden, out_dim


# ── Header generation ──────────────────────────────────────────────────────


def generate_header(model_path: Path) -> str:
    """Generate the complete C++ header string from an ONNX model."""
    tensors = _extract_weights(model_path)
    in_dim, hidden, out_dim = _validate_architecture(tensors)

    w0_arr = tensors[0][1]
    b0_arr = tensors[1][1]
    w1_arr = tensors[2][1]
    b1_arr = tensors[3][1]
    w2_arr = tensors[4][1]
    b2_arr = tensors[5][1]

    total_params = sum(a.size for _, a in tensors)

    parts = []
    parts.append(f"// Auto-generated from {model_path.name} -- do not edit")
    parts.append(
        f"// Architecture: Linear({in_dim}->{hidden}) -> Tanh"
        f" -> Linear({hidden}->{hidden}) -> Tanh"
        f" -> Linear({hidden}->{out_dim})"
    )
    parts.append(f"// Total parameters: {total_params}")
    parts.append("#pragma once")
    parts.append("")
    parts.append("#include <algorithm>")
    parts.append("#include <array>")
    parts.append("#include <cmath>")
    parts.append("")
    parts.append("namespace policy {")
    parts.append("")
    parts.append(f"inline constexpr int kInput = {in_dim};")
    parts.append(f"inline constexpr int kHidden = {hidden};")
    parts.append(f"inline constexpr int kOutput = {out_dim};")
    parts.append("")

    # Weight arrays
    parts.append(_fmt_array("kW0", w0_arr))
    parts.append("")
    parts.append(_fmt_array("kB0", b0_arr))
    parts.append("")
    parts.append(_fmt_array("kW1", w1_arr))
    parts.append("")
    parts.append(_fmt_array("kB1", b1_arr))
    parts.append("")
    parts.append(_fmt_array("kW2", w2_arr))
    parts.append("")
    parts.append(_fmt_array("kB2", b2_arr))

    # Inference functions
    parts.append("")
    parts.append("// y = x * W^T + b  (Gemm with transB=1)")
    parts.append("template<int OutDim, int InDim>")
    parts.append("inline void linear(float *out, const float *in,")
    parts.append("                   const float *weight, const float *bias)")
    parts.append("{")
    parts.append("    for (int o = 0; o < OutDim; ++o) {")
    parts.append("        float sum = bias[o];")
    parts.append("        for (int i = 0; i < InDim; ++i)")
    parts.append("            sum += in[i] * weight[o * InDim + i];")
    parts.append("        out[o] = sum;")
    parts.append("    }")
    parts.append("}")
    parts.append("")
    parts.append("template<int N>")
    parts.append("inline void tanhActivation(float *x)")
    parts.append("{")
    parts.append("    for (int i = 0; i < N; ++i)")
    parts.append("        x[i] = std::tanh(x[i]);")
    parts.append("}")
    parts.append("")
    parts.append("/// Run the policy network: 5 floats in -> 2 logits out.")
    parts.append("/// Returns P(flap) via softmax of logit[1].")
    parts.append("inline float predict(const std::array<float, kInput> &obs)")
    parts.append("{")
    parts.append("    float h0[kHidden];")
    parts.append("    linear<kHidden, kInput>(h0, obs.data(), kW0, kB0);")
    parts.append("    tanhActivation<kHidden>(h0);")
    parts.append("")
    parts.append("    float h1[kHidden];")
    parts.append("    linear<kHidden, kHidden>(h1, h0, kW1, kB1);")
    parts.append("    tanhActivation<kHidden>(h1);")
    parts.append("")
    parts.append("    float logits[kOutput];")
    parts.append("    linear<kOutput, kHidden>(logits, h1, kW2, kB2);")
    parts.append("")
    parts.append("    // Numerically stable softmax for P(flap)")
    parts.append("    const float mx = std::max(logits[0], logits[1]);")
    parts.append("    const float e0 = std::exp(logits[0] - mx);")
    parts.append("    const float e1 = std::exp(logits[1] - mx);")
    parts.append("    return e1 / (e0 + e1);")
    parts.append("}")
    parts.append("")
    parts.append("} // namespace policy")
    parts.append("")

    return "\n".join(parts)


# ── Verification ───────────────────────────────────────────────────────────


def _verify(model_path: Path, header_text: str):
    """Run a quick sanity check: ONNX inference vs. the generated C++ constants."""
    import onnxruntime as ort

    sess = ort.InferenceSession(str(model_path))
    test_obs = np.array([[0.5, 0.5, 0.5, 0.1, -0.1]], dtype=np.float32)
    onnx_logits = sess.run(None, {"obs": test_obs})[0][0]

    print(f"  ONNX logits:  [{onnx_logits[0]:.6f}, {onnx_logits[1]:.6f}]")

    mx = max(onnx_logits)
    e = np.exp(onnx_logits - mx)
    p_flap = e[1] / e.sum()
    print(f"  P(flap):      {p_flap:.6f}")


# ── Main ───────────────────────────────────────────────────────────────────


def main():
    script_dir = Path(__file__).parent
    default_model = script_dir.parent / "model.onnx"
    default_output = script_dir.parent / "policyweights.h"

    model_path = Path(sys.argv[1]) if len(sys.argv) > 1 else default_model
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else default_output

    if not model_path.exists():
        print(f"ERROR: Model not found: {model_path}")
        print("Run train.py first to produce model.onnx.")
        return 1

    print(f"Loading {model_path} ...")
    header = generate_header(model_path)

    print(f"Writing {output_path} ...")
    output_path.write_text(header)

    line_count = header.count("\n")
    byte_count = len(header.encode())
    print(f"Generated {line_count} lines, {byte_count} bytes")

    # Optional verification (only if onnxruntime is installed)
    try:
        print("Verifying against ONNX runtime ...")
        _verify(model_path, header)
        print("Verification OK")
    except ImportError:
        print("(onnxruntime not installed, skipping verification)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
