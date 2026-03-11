from __future__ import annotations

import argparse
from pathlib import Path
import numpy as np


def write_matrix_txt(path: Path, mat: np.ndarray) -> None:
    mat = np.asarray(mat, dtype=np.int32)
    with path.open("w", encoding="utf-8") as f:
        for row in mat:
            f.write(" ".join(str(int(v)) for v in row) + "\n")


def read_matrix_txt(path: Path, rows: int | None = None, cols: int | None = None) -> np.ndarray:
    data = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            vals = [int(x, 0) for x in s.split()]
            data.append(vals)

    arr = np.asarray(data, dtype=np.int32)
    if rows is not None and arr.shape[0] != rows:
        raise ValueError(f"{path} row count mismatch: got {arr.shape[0]}, expect {rows}")
    if cols is not None and arr.shape[1] != cols:
        raise ValueError(f"{path} col count mismatch: got {arr.shape[1]}, expect {cols}")
    return arr


def write_stream_txt(path: Path, vec: np.ndarray) -> None:
    vec = np.asarray(vec, dtype=np.int32).reshape(-1)
    with path.open("w", encoding="utf-8") as f:
        for v in vec:
            f.write(f"{int(v)}\n")


def make_input_rows(kind: str, rows: int, cols: int, seed: int,
                    low: int, high: int, const_value: int) -> np.ndarray:
    rng = np.random.default_rng(seed)

    if kind == "zeros":
        x = np.zeros((rows, cols), dtype=np.int32)
    elif kind == "const":
        x = np.full((rows, cols), const_value, dtype=np.int32)
    elif kind == "ramp":
        base = np.arange(cols, dtype=np.int32)
        x = np.stack([base + np.int32(10 * r) for r in range(rows)], axis=0)
    elif kind == "checker":
        base = np.array([1 if i % 2 == 0 else -1 for i in range(cols)], dtype=np.int32)
        x = np.stack([(r + 1) * base for r in range(rows)], axis=0).astype(np.int32)
    elif kind == "impulse":
        x = np.zeros((rows, cols), dtype=np.int32)
        for r in range(rows):
            pos = (cols // 2 + r) % cols
            x[r, pos] = np.int32(5 + r)
    elif kind == "random":
        x = rng.integers(low, high + 1, size=(rows, cols), dtype=np.int32)
    else:
        raise ValueError(f"unsupported kind: {kind}")

    return x.astype(np.int32)


def _idx(j: int, cols: int, mode: str) -> int | None:
    if mode == "periodic":
        return j % cols
    if 0 <= j < cols:
        return j
    if mode == "clamp":
        return 0 if j < 0 else cols - 1
    if mode == "zero":
        return None
    raise ValueError(f"unsupported boundary mode: {mode}")


def _get(row: np.ndarray, j: int, mode: str) -> np.int64:
    idx = _idx(j, row.shape[0], mode)
    if idx is None:
        return np.int64(0)
    return np.int64(row[idx])


def lap5(north: np.ndarray, center: np.ndarray, south: np.ndarray, j: int, mode: str) -> np.int64:
    c = _get(center, j, mode)
    w = _get(center, j - 1, mode)
    e = _get(center, j + 1, mode)
    n = _get(north, j, mode)
    s = _get(south, j, mode)
    return np.int64(4) * c - n - s - w - e


def hdiff_one_window(rows5: np.ndarray, boundary: str = "clamp") -> np.ndarray:
    rows5 = np.asarray(rows5, dtype=np.int32)
    if rows5.shape[0] != 5:
        raise ValueError(f"rows5 must be shape (5, cols), got {rows5.shape}")

    row0, row1, row2, row3, row4 = rows5
    cols = row2.shape[0]
    out = np.zeros(cols, dtype=np.int32)

    for j in range(cols):
        c = _get(row2, j, boundary)
        w = _get(row2, j - 1, boundary)
        e = _get(row2, j + 1, boundary)
        n = _get(row1, j, boundary)
        s = _get(row3, j, boundary)

        lap_ij  = lap5(row1, row2, row3, j, boundary)
        lap_ijm = lap5(row1, row2, row3, j - 1, boundary)
        lap_ijp = lap5(row1, row2, row3, j + 1, boundary)
        lap_imj = lap5(row0, row1, row2, j, boundary)
        lap_ipj = lap5(row2, row3, row4, j, boundary)

        flux1 = lap_ij - lap_ijm
        flux2 = lap_ijp - lap_ij
        flux3 = lap_ij - lap_imj
        flux4 = lap_ipj - lap_ij

        inter1 = flux1 if flux1 * (c - w) > 0 else 0
        inter2 = flux2 if flux2 * (e - c) > 0 else 0
        inter3 = flux3 if flux3 * (c - n) > 0 else 0
        inter4 = flux4 if flux4 * (s - c) > 0 else 0

        val = c - np.int64(7) * (inter2 - inter1 - inter3 + inter4)
        out[j] = np.int32(val)

    return out


def hdiff_two_windows(rows6: np.ndarray, boundary: str = "clamp") -> np.ndarray:
    rows6 = np.asarray(rows6, dtype=np.int32)
    if rows6.shape[0] != 6:
        raise ValueError(f"rows6 must be shape (6, cols), got {rows6.shape}")

    out0 = hdiff_one_window(rows6[0:5], boundary=boundary)
    out1 = hdiff_one_window(rows6[1:6], boundary=boundary)
    return np.stack([out0, out1], axis=0).astype(np.int32)


def dump_human_readable_inputs(rows6: np.ndarray, out_dir: Path, graph_id: str = "hdiff") -> None:
    """
    生成人看的 2x256 文件：
      hdiff_in0.txt = [row0; row1]
      hdiff_in1.txt = [row1; row2]
      ...
      hdiff_in4.txt = [row4; row5]
    """
    for i in range(5):
        pair = np.stack([rows6[i], rows6[i + 1]], axis=0).astype(np.int32)
        write_matrix_txt(out_dir / f"{graph_id}_in{i}.txt", pair)


def dump_aie_stream_inputs(rows6: np.ndarray, out_dir: Path, graph_id: str = "hdiff") -> None:
    """
    生成 AIE 真正读的流文件：
      hdiff_in0_stream.txt = row0 || row1
      ...
      hdiff_in4_stream.txt = row4 || row5
    """
    for i in range(5):
        pair = np.concatenate([rows6[i], rows6[i + 1]]).astype(np.int32)
        write_stream_txt(out_dir / f"{graph_id}_in{i}_stream.txt", pair)


def main() -> None:
    base_dir = Path(__file__).resolve().parent

    ap = argparse.ArgumentParser(
        description="Generate input.txt, gold_out.txt, human-readable hdiff_in*.txt, and AIE stream input files."
    )
    ap.add_argument("--data-dir", type=Path, default=base_dir)
    ap.add_argument("--rows", type=int, default=6)
    ap.add_argument("--cols", type=int, default=256)
    ap.add_argument("--kind", choices=["zeros", "const", "ramp", "checker", "impulse", "random"], default="random")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--low", type=int, default=-20)
    ap.add_argument("--high", type=int, default=20)
    ap.add_argument("--const-value", type=int, default=7)
    ap.add_argument("--boundary", choices=["clamp", "periodic", "zero"], default="clamp")
    ap.add_argument("--graph-id", default="hdiff")
    args = ap.parse_args()

    if args.rows != 6:
        raise ValueError("this validation flow is fixed to 6 input rows")

    data_dir = args.data_dir
    data_dir.mkdir(parents=True, exist_ok=True)

    rows6 = make_input_rows(
        kind=args.kind,
        rows=6,
        cols=args.cols,
        seed=args.seed,
        low=args.low,
        high=args.high,
        const_value=args.const_value,
    )

    input_txt = data_dir / "input.txt"
    gold_txt = data_dir / "gold_out.txt"

    write_matrix_txt(input_txt, rows6)
    rows6_reload = read_matrix_txt(input_txt, rows=6, cols=args.cols)

    gold2 = hdiff_two_windows(rows6_reload, boundary=args.boundary)
    write_matrix_txt(gold_txt, gold2)

    dump_human_readable_inputs(rows6_reload, data_dir, graph_id=args.graph_id)
    dump_aie_stream_inputs(rows6_reload, data_dir, graph_id=args.graph_id)

    print(f"[ok] wrote {input_txt} shape={rows6_reload.shape}")
    print(f"[ok] wrote {gold_txt} shape={gold2.shape}")
    print("[ok] wrote human-readable inputs : hdiff_in0.txt ~ hdiff_in4.txt (each is 2x256)")
    print("[ok] wrote AIE stream inputs     : hdiff_in0_stream.txt ~ hdiff_in4_stream.txt (each is 512 scalars)")
    print()
    print("Sliding windows:")
    print("  win0 = [row0 row1 row2 row3 row4] -> out0")
    print("  win1 = [row1 row2 row3 row4 row5] -> out1")
    print()
    print("Port mapping:")
    print("  in0 = [row0, row1]")
    print("  in1 = [row1, row2]")
    print("  in2 = [row2, row3]")
    print("  in3 = [row3, row4]")
    print("  in4 = [row4, row5]")


if __name__ == "__main__":
    main()