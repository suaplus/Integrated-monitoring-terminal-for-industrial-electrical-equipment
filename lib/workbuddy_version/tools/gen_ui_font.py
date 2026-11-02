#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成界面需要的 LVGL 中文字体。

字符集不再手工维护: 直接扫描 demo1/ 与 main.c 里所有字符串字面量,
把它们出现的非 ASCII 字符全部打进字体, 因此不会再出现"改文案漏字->乱码"。

用法:
    python tools/gen_ui_font.py

依赖:
    - node + lv_font_conv  (npm install lv_font_conv)
    - 源字体 STFANGSO.TTF (华文仿宋)
"""

import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import scan_charset  # noqa: E402

SIZES = (16, 24, 32)

# 除了源码里扫到的字, 再额外兜一层常用符号, 避免运行时拼出来的字符(单位/标点)缺字
EXTRA = "%/:.·-×≥≤()℃±°↑↓←→"

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FONT_TTF = r"C:/Users/suaplus/SquareLine/assets/STFANGSO.TTF"
OUT_DIR = os.path.join(ROOT, "demo1", "fonts")
LFC = os.path.join(
    "C:/Users/suaplus/.workbuddy/binaries/node/workspace",
    "node_modules", "lv_font_conv", "lv_font_conv.js",
)
NODE = r"C:/Users/suaplus/.workbuddy/binaries/node/versions/22.22.2-6/node.exe"


def dedup(s: str) -> str:
    seen, out = set(), []
    for ch in s:
        if ch not in seen:
            seen.add(ch)
            out.append(ch)
    return "".join(out)


def main() -> int:
    if not os.path.isfile(FONT_TTF):
        print("找不到源字体: %s" % FONT_TTF)
        return 1
    if not os.path.isfile(LFC):
        print("找不到 lv_font_conv, 先执行: npm install lv_font_conv")
        return 1

    used = scan_charset.collect()
    symbols = dedup("".join(sorted(used)) + EXTRA)
    print("扫描到非 ASCII 字符: %d 个 + 兜底符号" % len(used))
    print("字形数量: %d" % len(symbols))
    print("".join(symbols))

    for size in SIZES:
        name = "ui_font_cn%d" % size
        out = os.path.join(OUT_DIR, name + ".c")
        cmd = [
            NODE, LFC,
            "--font", FONT_TTF,
            "-r", "0x20-0x7f",
            "--symbols", symbols,
            "--size", str(size),
            "--bpp", "1",
            "--format", "lvgl",
            "--no-compress", "--no-prefilter",
            "-o", out,
        ]
        print("\n>>> %s (%d px)" % (name, size))
        r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode != 0:
            print(r.stdout)
            print(r.stderr, file=sys.stderr)
            return r.returncode

        with open(out, "r", encoding="utf-8") as f:
            src = f.read()
        # 与 SquareLine 产出的字体保持一致: 引 ui.h, 带开关宏, 便于整体裁剪
        src = src.replace('#include "lvgl/lvgl.h"', '#include "../ui.h"')
        with open(out, "w", encoding="utf-8") as f:
            f.write(src)
        print("    生成 %s (%.1f KB)" % (out, os.path.getsize(out) / 1024.0))

    # 生成后立刻自检, 有缺字直接报错
    print("\n=== 覆盖自检 ===")
    bad = False
    need = set(ord(c) for c in used)
    for size in SIZES:
        p = os.path.join(OUT_DIR, "ui_font_cn%d.c" % size)
        cm = scan_charset.font_cmap(p) or set()
        miss = sorted(need - cm)
        if miss:
            bad = True
            print("[%d] 仍缺 %d 字: %s" % (size, len(miss), "".join(chr(c) for c in miss)))
        else:
            print("[%d] OK, %d 字形" % (size, len(cm)))
    return 2 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
