#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

CSV_DIR = "csvs"

def load_csv(name):
    path = os.path.join(CSV_DIR, name)
    if not os.path.exists(path):
        print("Missing:", path)
        return None
    return pd.read_csv(path)


def plot_line(ax, x, y, xlabel, ylabel, title):

    x = np.array(x)
    y = np.array(y)

    mask = (
        np.isfinite(x)
        & np.isfinite(y)
        & (x > 0)
        & (y > 0)
    )

    x = x[mask]
    y = y[mask]

    # 按 size 排序（否则线会乱跳）
    idx = np.argsort(x)

    x = x[idx]
    y = y[idx]

    ax.plot(
        x,
        y,
        linewidth=1,
        marker="o",
        markersize=3,
        alpha=0.8
    )

    ax.set_xscale("log")
    ax.set_yscale("log")

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)

    ax.grid(True)

def plot_scatter(ax, x, y, xlabel, ylabel, title):
    x = np.array(x)
    y = np.array(y)

    # 过滤无效值
    mask = (
        np.isfinite(x)
        & np.isfinite(y)
        & (x > 0)
        & (y > 0)
    )
    x = x[mask]
    y = y[mask]
    n_points = len(x)

    # ==============================================
    # 自动判断：点少用散点，点多用密度图
    # ==============================================
    if n_points < 10000:
        # 少量数据：散点图，清晰好看
        point_size = 50
        if n_points > 100:
            point_size = 25
        if n_points > 500:
            point_size = 10
        if n_points > 1000:
            point_size = 5
        ax.scatter(
            x, y,
            s=point_size,          # 小点
            alpha=0.6,     # 适度透明
            linewidths=0,
            c='#1f77b4'
        )
    else:
        # 大量数据：2D 密度图，永不糊图
        h = ax.hist2d(
            x, y,
            bins=60,       # 密度精度，可微调
            cmap='Blues',  # 蓝色系清爽
            norm='log'     # 对数密度，解决极端聚集
        )
        # 添加颜色条（可选）
        # plt.colorbar(h[3], ax=ax)

    # 对数坐标 + 标签
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, alpha=0.3)

def main():

    lifetime = load_csv("size_vs_lifetime.csv")
    miss = load_csv("size_vs_miss.csv")
    density = load_csv("size_vs_density.csv")
    memratio = load_csv("size_vs_memratio.csv")
    bucket = load_csv("bucket_stats.csv")

    plt.figure(figsize=(18, 10))

    # =========================================================
    # 1 Size vs Memory Ratio
    # =========================================================

    if memratio is not None:

        ax = plt.subplot(2, 3, 1)

        plot_scatter(
            ax,
            memratio["size"],
            memratio["mem_ratio"],
            "Object Size",
            "Memory Ratio (%)",
            "Size vs Memory Ratio"
        )


    # =========================================================
    # 2 Size vs Miss
    # =========================================================

    if miss is not None:

        ax = plt.subplot(2, 3, 2)

        plot_scatter(
            ax,
            miss["size"],
            miss["miss"],
            "Object Size",
            "Total Miss",
            "Size vs Miss"
        )


    # =========================================================
    # 3 Size vs Lifetime
    # =========================================================

    if lifetime is not None:

        ax = plt.subplot(2, 3, 3)

        plot_scatter(
            ax,
            lifetime["size"],
            lifetime["lifetime"],
            "Object Size",
            "Lifetime",
            "Size vs Lifetime"
        )


    # =========================================================
    # 4 Size vs Density
    # =========================================================

    if density is not None:

        ax = plt.subplot(2, 3, 4)

        plot_scatter(
            ax,
            density["size"],
            density["density"],
            "Object Size",
            "Access Density",
            "Size vs Access Density"
        )


    # =========================================================
    # 5 Size vs Miss Rate
    # =========================================================

    if miss is not None:

        ax = plt.subplot(2, 3, 5)

        access = miss["miss"]  # 只是保持结构一致

        plot_scatter(
            ax,
            miss["size"],
            miss["miss"],
            "Object Size",
            "Miss",
            "Size vs Miss (Line)"
        )


    # =========================================================
    # 6 Bucket Avg Miss
    # =========================================================

    if bucket is not None:

        ax = plt.subplot(2, 3, 6)

        labels = [
            "0-1KB",
            "1-4KB",
            "4-16KB",
            "16-64KB",
            "64-256KB",
            "256KB+"
        ]

        x = bucket["bucket"]

        ax.bar(
            x,
            bucket["avg_miss"]
        )

        ax.set_xticks(x)

        ax.set_xticklabels(
            [labels[int(i)] for i in x]
        )

        ax.set_xlabel("Size Bucket")

        ax.set_ylabel("Average Miss")

        ax.set_title("Bucketed Miss")

        ax.grid(True)


    plt.tight_layout()

    plt.savefig(
        "figure_all.png",
        dpi=300,
        bbox_inches="tight"
    )

    print("Saved: figure_all.png")


if __name__ == "__main__":
    main()