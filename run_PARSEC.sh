# run_parsec.sh
#!/bin/bash

# ================= 配置区（只改这里） =================

TRACE_NAME="blackscholes"
INPUT_NAME="in_16.txt"

TARGET_BIN="$HOME/ChampSim/mytests/parsec-benchmark/pkgs/apps/$TRACE_NAME/inst/amd64-linux.gcc-serial/bin/$TRACE_NAME"

INPUT_FILE="/home/yanami0624/ChampSim/mytests/parsec-benchmark/pkgs/apps/$TRACE_NAME/run/$INPUT_NAME"

OUTPUT_FILE="$HOME/ChampSim/mytests/parsec-benchmark/pkgs/apps/$TRACE_NAME/run/output.txt"

PROGRAM_ARGS="1 $INPUT_FILE $OUTPUT_FILE"

CHAMPSIM_TRACER_DIR="$HOME/ChampSim/tracer/pin"
# ====================================================

PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"
CHAMPSIM_DIR="$HOME/ChampSim"

TRACER="$CHAMPSIM_TRACER_DIR/obj-intel64/champsim_tracer.so"

TRACE_FILE="$CHAMPSIM_DIR/traces/${TRACE_NAME}.champsim"

CHAMPSIM_BIN="$CHAMPSIM_DIR/bin/champsim"

SKIP_INS=0
TRACE_NUM_INS=100000000

mkdir -p "$CHAMPSIM_DIR/traces"

export PIN_ROOT

# ================= 0️⃣ 编译 tracer =================

echo "=== [0/3] Compiling champsim_tracer ==="

pushd "$CHAMPSIM_TRACER_DIR" > /dev/null

make clean
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ tracer compile failed"
    exit 1
fi

popd > /dev/null

# ================= 1️⃣ 生成 trace =================

echo "=== [1/3] Generating trace ==="

$PIN_ROOT/pin \
    -t "$TRACER" \
    -o "$TRACE_FILE" \
    -s "$SKIP_INS" \
    -t "$TRACE_NUM_INS" \
    -- "$TARGET_BIN" $PROGRAM_ARGS

if [ ! -f "$TRACE_FILE" ]; then
    echo "❌ Trace generation failed"
    exit 1
fi

echo "✔ Trace generated: $TRACE_FILE"

# ================= 2️⃣ 运行 ChampSim =================

echo "=== [2/3] Running ChampSim ==="

$CHAMPSIM_BIN "$TRACE_FILE"

echo "✅ Done!"