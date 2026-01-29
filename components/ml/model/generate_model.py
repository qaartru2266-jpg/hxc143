import argparse
from pathlib import Path

HEADER = r'''#pragma once
#include <cstddef>
#include <cstdint>

extern const unsigned char g_model[];
extern const size_t g_model_len;
'''

CC_TEMPLATE = r'''#include "model_data.h"

alignas(16) const unsigned char g_model[] = {{
{data}
}};

const size_t g_model_len = sizeof(g_model);
'''


def bytes_to_c_array(b: bytes, cols: int = 12) -> str:
    hexes = [f"0x{x:02x}" for x in b]
    lines = []
    for i in range(0, len(hexes), cols):
        lines.append("  " + ", ".join(hexes[i:i+cols]) + ",")
    return "\n".join(lines)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="inp", required=True, help="input .tflite")
    ap.add_argument("--out_dir", default=".", help="output directory")
    args = ap.parse_args()

    inp = Path(args.inp)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    data = inp.read_bytes()

    (out_dir / "model_data.h").write_text(HEADER, encoding="utf-8")
    (out_dir / "model_data.cc").write_text(
        CC_TEMPLATE.format(data=bytes_to_c_array(data)),
        encoding="utf-8"
    )

    print(f"[DONE] wrote {out_dir / 'model_data.cc'} ({len(data)} bytes)")
    print(f"[DONE] wrote {out_dir / 'model_data.h'}")

if __name__ == "__main__":
    main()
