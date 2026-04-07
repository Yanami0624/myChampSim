#!/bin/bash

# ================= 配置区（只改这里） =================
CPP_FILE="$HOME/ChampSim/mytests/test.cpp"       # 你的 cpp 程序
CHAMPSIM_TRACER_DIR="$HOME/ChampSim/tracer/pin"  # tracer 目录
PYTHON_SCRIPT="$HOME/ChampSim/draw.py"   # 绘图脚本路径（重要）
# ====================================================

# ===== 固定路径（不用改）=====
PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"
CHAMPSIM_DIR="$HOME/ChampSim"

BIN_FILE="$HOME/mytests/bin/target"
TRACER="$CHAMPSIM_TRACER_DIR/obj-intel64/champsim_tracer.so"
TRACE_FILE="$CHAMPSIM_DIR/traces/$(basename $CPP_FILE .cpp).champsim"
CHAMPSIM_BIN="$CHAMPSIM_DIR/bin/champsim"

# 参数
SKIP_INS=0
TRACE_NUM_INS=1000000

# ================= 自动准备 =================
mkdir -p "$(dirname $BIN_FILE)"
mkdir -p "$CHAMPSIM_DIR/traces"
mkdir -p "$CHAMPSIM_DIR/csvs"  # 自动创建 csv 文件夹

export PIN_ROOT

# ================= 0️⃣ 编译 champsim_tracer =================
echo -e "\n=== [0/4] Compiling champsim_tracer ==="
pushd "$CHAMPSIM_TRACER_DIR" > /dev/null
make clean
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ compile failed"
    exit 1
fi
popd > /dev/null

# ================= 1️⃣ 编译 cpp =================
echo -e "\n=== [1/4] Compiling CPP ==="
g++ -O2 "$CPP_FILE" -o "$BIN_FILE"

if [ $? -ne 0 ]; then
    echo "❌ Compile failed"
    exit 1
fi

# ================= 2️⃣ 用 Pin 生成 trace =================
echo -e "\n=== [2/4] Generating trace ==="
$PIN_ROOT/pin \
    -t "$TRACER" \
    -o "$TRACE_FILE" \
    -s "$SKIP_INS" -t "$TRACE_NUM_INS" \
    -- "$BIN_FILE"

if [ ! -f "$TRACE_FILE" ]; then
    echo "❌ Trace generation failed"
    exit 1
fi
echo "✔ Trace generated: $TRACE_FILE"

# ================= 3️⃣ 用 ChampSim 分析 =================
echo -e "\n=== [3/4] Running ChampSim ==="
cd "$CHAMPSIM_DIR"
$CHAMPSIM_BIN "$TRACE_FILE"

if [ $? -ne 0 ]; then
    echo "❌ ChampSim run failed"
    exit 1
fi

# ================= 4️⃣ 自动运行绘图 =================
echo -e "\n=== [4/4] Generating plots ==="
if [ -f "$PYTHON_SCRIPT" ]; then
    python3 "$PYTHON_SCRIPT"
    echo "✅ All done! Figure saved as figure_all.png"
else
    echo "⚠️ Draw script not found: $PYTHON_SCRIPT"
fi