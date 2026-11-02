#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
扫描界面源码里实际用到的字符，输出字符集，并可选地与字体 cmap 校验。

用法:
    python tools/scan_charset.py           # 只打印用到的字符
    python tools/scan_charset.py --check   # 额外校验字体 cmap 覆盖情况
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# 参与扫描的源文件目录
SCAN_DIRS = [
    os.path.join(ROOT, "demo1"),
    ROOT,
]
SCAN_FILES = ["main.c"]

# 只要这些扩展名
EXTS = (".c", ".h")


def strip_comments_and_collect(path, acc):
    """读取源文件，跳过注释，收集字符串/字符字面量中的非 ASCII 字符。"""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        src = f.read()

    i, n = 0, len(src)
    while i < n:
        ch = src[i]
        # 行注释
        if src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j + 1
            continue
        # 块注释
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        # 字符串 / 字符字面量
        if ch in ('"', "'"):
            quote = ch
            i += 1
            while i < n:
                c = src[i]
                if c == "\\":
                    i += 2
                    continue
                if c == quote:
                    i += 1
                    break
                if ord(c) > 0x7F:
                    acc.add(c)
                i += 1
            continue
        i += 1


def collect():
    acc = set()
    for d in SCAN_DIRS:
        if not os.path.isdir(d):
            continue
        for name in sorted(os.listdir(d)):
            p = os.path.join(d, name)
            if os.path.isfile(p) and name.endswith(EXTS):
                strip_comments_and_collect(p, acc)
        # demo1 的子目录
        for sub in ("screens", "components"):
            sd = os.path.join(d, sub)
            if not os.path.isdir(sd):
                continue
            for name in sorted(os.listdir(sd)):
                p = os.path.join(sd, name)
                if os.path.isfile(p) and name.endswith(EXTS):
                    strip_comments_and_collect(p, acc)
    return acc


def font_cmap(path):
    """从 lv_font_conv 生成的 .c 注释里解析实际打包的 unicode 码点。"""
    import re

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        src = f.read()
    vals = re.findall(r"/\* U\+([0-9A-Fa-f]{4,6}) ", src)
    return set(int(v, 16) for v in vals)


def main():
    acc = collect()
    chars = sorted(acc)
    print("字符串字面量中的非 ASCII 字符: %d 个" % len(chars))
    print("".join(chars))

    if "--check" in sys.argv:
        need = set(ord(c) for c in chars)
        for size in (16, 24, 32):
            p = os.path.join(ROOT, "demo1", "fonts", "ui_font_cn%d.c" % size)
            if not os.path.isfile(p):
                print("[%d] 字体文件不存在" % size)
                continue
            cm = font_cmap(p)
            if cm is None:
                print("[%d] 未解析到 unicode_list" % size)
                continue
            miss = sorted(need - cm)
            print("[%d] cmap %d 个字形, 缺失 %d 个%s" % (
                size, len(cm), len(miss),
                ("".join(chr(c) for c in miss) if miss else ""),
            ))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
