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


def plot_scatter(ax, x, y, xlabel, ylabel, title):
    """散点图 / 密度图（自动适配数据量）"""
    x = np.array(x)
    y = np.array(y)

    # 过滤无效值
    mask = (
        np.isfinite(x)
        & np.isfinite(y)
        & (x > 0)
    )
    x = x[mask]
    y = y[mask]
    n_points = len(x)

    if n_points < 10000:
        # 散点图
        point_size = 20
        if n_points > 1000:
            point_size = 5
        ax.scatter(
            x, y,
            s=point_size,
            alpha=0.6,
            linewidths=0,
            c='#1f77b4'
        )
    else:
        # 密度图
        ax.hist2d(
            x, y,
            bins=50,
            cmap='Blues',
            norm='log'
        )

    ax.set_xscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, alpha=0.3)


def plot_bucket_bar(ax, x, y, xlabel, ylabel, title, labels):
    """分桶柱状图"""
    ax.bar(x, y, color='#ff7f0e', alpha=0.8)
    ax.set_xticks(x)
    ax.set_xticklabels([labels[int(i)] for i in x], rotation=30)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, alpha=0.3)


def main():
    # 加载所有CSV（严格匹配C++输出文件名）
    df_lifetime    = load_csv("size_vs_lifetime.csv")
    df_miss        = load_csv("size_vs_miss.csv")
    df_density     = load_csv("size_vs_density.csv")
    df_memratio    = load_csv("size_vs_memratio.csv")
    df_l1_mr       = load_csv("size_vs_l1_missrate.csv")
    df_l2_mr       = load_csv("size_vs_l2_missrate.csv")
    df_bucket      = load_csv("bucket_stats.csv")

    # 3行3列布局，容纳8个图 + 留白
    plt.figure(figsize=(18, 15))

    # 分桶标签（和C++ bucket[6]对应）
    bucket_labels = [
        "0-1KB",
        "1-4KB",
        "4-16KB",
        "16-64KB",
        "64-256KB",
        "256KB+"
    ]

    # ===================== 1 Size vs Lifetime =====================
    if df_lifetime is not None:
        ax = plt.subplot(3, 3, 1)
        plot_scatter(
            ax,
            df_lifetime["size"],
            df_lifetime["lifetime"],
            "Object Size (B)",
            "Lifetime (cycles)",
            "Size vs Lifetime"
        )

    # ===================== 2 Size vs Total Miss =====================
    if df_miss is not None:
        ax = plt.subplot(3, 3, 2)
        plot_scatter(
            ax,
            df_miss["size"],
            df_miss["miss"],
            "Object Size (B)",
            "Total Miss Count",
            "Size vs Total Miss"
        )

    # ===================== 3 Size vs Access Density =====================
    if df_density is not None:
        ax = plt.subplot(3, 3, 3)
        plot_scatter(
            ax,
            df_density["size"],
            df_density["density"],
            "Object Size (B)",
            "Density (access/KB)",
            "Size vs Access Density"
        )

    # ===================== 4 Size vs Memory Ratio =====================
    if df_memratio is not None:
        ax = plt.subplot(3, 3, 4)
        plot_scatter(
            ax,
            df_memratio["size"],
            df_memratio["mem_ratio"],
            "Object Size (B)",
            "Peak Memory Ratio (%)",
            "Size vs Memory Ratio"
        )

    # ===================== 5 Size vs L1 Miss Rate =====================
    if df_l1_mr is not None:
        ax = plt.subplot(3, 3, 5)
        plot_scatter(
            ax,
            df_l1_mr["size"],
            df_l1_mr["l1_miss_rate"] * 100,  # 转百分比
            "Object Size (B)",
            "L1 Miss Rate (%)",
            "Size vs L1 Miss Rate"
        )

    # ===================== 6 Size vs L2 Miss Rate =====================
    if df_l2_mr is not None:
        ax = plt.subplot(3, 3, 6)
        plot_scatter(
            ax,
            df_l2_mr["size"],
            df_l2_mr["l2_miss_rate"] * 100,  # 转百分比
            "Object Size (B)",
            "L2 Miss Rate (%)",
            "Size vs L2 Miss Rate"
        )

    # ===================== 7 Size vs Total Miss Rate (MR) =====================
    if df_miss is not None and df_density is not None:
        # 从已有数据计算总MR
        size = df_miss["size"]
        miss = df_miss["miss"]
        
        ax = plt.subplot(3, 3, 7)
        plot_scatter(
            ax,
            size,
            miss / (miss / df_density["density"] * 1024) * 100,  # 总缺失率%
            "Object Size (B)",
            "Total Miss Rate (%)",
            "Size vs Total Miss Rate (MR)"
        )

    # ===================== 8 Bucket Stats (综合分桶图) =====================
    if df_bucket is not None:
        ax = plt.subplot(3, 3, 8)
        plot_bucket_bar(
            ax,
            df_bucket["bucket"],
            df_bucket["avg_l1_mr_pct"],
            "Size Bucket",
            "Avg L1 Miss Rate (%)",
            "Bucket Stats: Avg L1 Miss Rate",
            bucket_labels
        )

    plt.tight_layout()
    plt.savefig("statistic.png", dpi=300, bbox_inches="tight")
    print("figure saved: statistic.png")


if __name__ == "__main__":
    main()