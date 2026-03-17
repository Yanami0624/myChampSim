#!/bin/bash

# ================= 配置区（只改这里） =================
CPP_FILE="$HOME/pin-3.22-98547-g7a303a835-gcc-linux/source/tools/MemTrace/memtrace.cpp"
TARGET_PROG="/bin/ls"   # 被分析的程序（也可以改成你自己的）
# ====================================================

# 自动推导路径
PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"
TOOL_DIR="$(dirname $CPP_FILE)"
TOOL_NAME="$(basename $CPP_FILE .cpp)"
SO_FILE="$TOOL_DIR/obj-intel64/${TOOL_NAME}.so"

TRACE_DIR="$HOME/ChampSim/traces"
TRACE_FILE="$TRACE_DIR/${TOOL_NAME}.champsim"

TRACER="$HOME/ChampSim/tracer/pin/obj-intel64/champsim_tracer.so"
CHAMPSIM_BIN="$HOME/ChampSim/bin/champsim"

# 参数
SKIP_INS=0
TRACE_NUM_INS=1000000

# ================= 准备 =================
mkdir -p "$TRACE_DIR"

export PIN_ROOT

# ================= 1️⃣ 编译 Pin工具 =================
echo "=== [1/3] Building Pintool ==="
cd "$TOOL_DIR" || exit 1

make

if [ ! -f "$SO_FILE" ]; then
    echo "❌ Pintool build failed: $SO_FILE not found"
    exit 1
fi

echo "✔ Pintool built: $SO_FILE"

# ================= 2️⃣ 运行 Pintool（仅分析） =================
echo "=== [2/3] Running Pintool ==="
$PIN_ROOT/pin -t "$SO_FILE" -- "$TARGET_PROG"

# ================= 3️⃣ （可选）生成 ChampSim trace =================
echo "=== [3/3] Generating ChampSim trace ==="
$PIN_ROOT/pin \
    -t "$TRACER" \
    -o "$TRACE_FILE" \
    -s "$SKIP_INS" -t "$TRACE_NUM_INS" \
    -- "$TARGET_PROG"

if [ ! -f "$TRACE_FILE" ]; then
    echo "❌ Trace generation failed"
    exit 1
fi

echo "✔ Trace generated: $TRACE_FILE"

# ================= 4️⃣ 跑 ChampSim =================
echo "=== [4/4] Running ChampSim ==="
$CHAMPSIM_BIN "$TRACE_FILE"

echo "✅ Done!"