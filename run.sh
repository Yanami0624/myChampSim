
# run.sh
#!/bin/bash

# ================= 配置区（只改这里） =================
CHAMPSIM_DIR="$HOME/myChampSim"
CPP_FILE="$CHAMPSIM_DIR/mytests/test.cpp"   # 你的 cpp 程序（改这里）
CHAMPSIM_TRACER_DIR="$CHAMPSIM_DIR/tracer/pin"  # champsim_tracer 所在目录
# ====================================================

# ===== 固定路径（不用改）=====
PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"

BIN_FILE="$HOME/mytests/bin/target"
TRACER="$CHAMPSIM_TRACER_DIR/obj-intel64/champsim_tracer.so"
TRACE_FILE="$CHAMPSIM_DIR/traces/$(basename $CPP_FILE .cpp).champsim"
CHAMPSIM_BIN="$CHAMPSIM_DIR/bin/champsim"

# 参数
SKIP_INS=0
TRACE_NUM_INS=1000000000

# ================= 自动准备 =================
mkdir -p "$(dirname $BIN_FILE)"
mkdir -p "$CHAMPSIM_DIR/traces"

export PIN_ROOT

# ================= 0️⃣ 编译 champsim_tracer =================
echo "=== [0/3] Compiling champsim_tracer ==="
pushd "$CHAMPSIM_TRACER_DIR" > /dev/null
#make clean
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ compile failed"
    exit 1
fi
popd > /dev/null

# ================= 1️⃣ 编译 cpp =================
echo "=== [1/3] Compiling CPP ==="
g++ -O2 "$CPP_FILE" -o "$BIN_FILE"

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
    -- "$BIN_FILE"

if [ ! -f "$TRACE_FILE" ]; then
    echo "❌ Trace generation failed"
    exit 1
fi

echo "✔ Trace generated: $TRACE_FILE"

# ================= 3️⃣ 用 ChampSim 分析 =================
echo "=== [3/3] Running ChampSim ==="
$CHAMPSIM_BIN "$TRACE_FILE" --hide-heartbeat

echo "✅ Done!"