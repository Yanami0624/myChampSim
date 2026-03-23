#!/bin/bash

# ================= 配置区（只改这里） =================
CPP_FILE="$HOME/ChampSim/mytests/mibench-master/mibench-master/telecomm/FFT/main.c"   # 改成你的 C 文件
CHAMPSIM_TRACER_DIR="$HOME/ChampSim/tracer/pin"  # champsim_tracer 所在目录
# ====================================================

# ===== 固定路径（不用改）=====
PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"
CHAMPSIM_DIR="$HOME/ChampSim"

# 二进制输出路径
BIN_FILE="$HOME/ChampSim/mytests/bin/$(basename $CPP_FILE .c)"
mkdir -p "$(dirname $BIN_FILE)"

# trace 输出路径
TRACE_FILE="$CHAMPSIM_DIR/traces/$(basename $CPP_FILE .c).champsim"
mkdir -p "$CHAMPSIM_DIR/traces"

CHAMPSIM_BIN="$CHAMPSIM_DIR/bin/champsim"
TRACER="$CHAMPSIM_TRACER_DIR/obj-intel64/champsim_tracer.so"

# 参数
SKIP_INS=0
TRACE_NUM_INS=1000000

export PIN_ROOT

# ================= 0️⃣ 编译 champsim_tracer =================
echo "=== [0/3] Compiling champsim_tracer ==="
pushd "$CHAMPSIM_TRACER_DIR" > /dev/null
make clean
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ compile failed"
    exit 1
fi
popd > /dev/null

# ================= 1️⃣ 编译 C 文件 =================
echo "=== [1/3] Compiling C file ==="
gcc -O2 "$CPP_FILE" -o "$BIN_FILE"

if [ $? -ne 0 ]; then
    echo "❌ Compile failed"
    exit 1
fi

# ================= 2️⃣ 用 Pin 生成 trace =================
echo "=== [2/3] Generating trace ==="
$PIN_ROOT/pin \
    -t "$TRACER" \
    -o "$TRACE_FILE" \
    -s "$SKIP_INS" -t "$TRACE_NUM_INS" \
    -- "$BIN_FILE" 4 4096 > \
       "$HOME/ChampSim/mytests/mibench-master/mibench-master/telecomm/FFT/output_small.txt"

if [ ! -f "$TRACE_FILE" ]; then
    echo "❌ Trace generation failed"
    exit 1
fi

echo "✔ Trace generated: $TRACE_FILE"

# ================= 3️⃣ 用 ChampSim 分析 =================
echo "=== [3/3] Running ChampSim ==="
$CHAMPSIM_BIN "$TRACE_FILE"

echo "✅ Done!"