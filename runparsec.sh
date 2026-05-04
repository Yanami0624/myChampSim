#!/bin/bash
set -e

# ================= 配置区 =================
TRACE_NAME="blackscholes"

PARSEC_DIR="$HOME/parsec-benchmark"
CHAMPSIM_DIR="$HOME/myChampSim"
PIN_ROOT="$HOME/pin-3.22-98547-g7a303a835-gcc-linux"

TARGET_BIN="${PARSEC_DIR}/pkgs/apps/${TRACE_NAME}/inst/amd64-linux.gcc/bin/${TRACE_NAME}"
RUN_DIR="${PARSEC_DIR}/pkgs/apps/${TRACE_NAME}/run"

# ✅ 终极正确参数（你本地真实存在的文件）
PROGRAM_ARGS="1 ${RUN_DIR}/in_4.txt ${RUN_DIR}/output.txt"

SKIP_INS=0
TRACE_NUM_INS=50000000
# ==========================================

CHAMPSIM_TRACER_DIR="${CHAMPSIM_DIR}/tracer/pin"
TRACER="${CHAMPSIM_TRACER_DIR}/obj-intel64/champsim_tracer.so"
TRACE_FILE="${CHAMPSIM_DIR}/traces/${TRACE_NAME}.champsim"
CHAMPSIM_BIN="${CHAMPSIM_DIR}/bin/champsim"

mkdir -p "${CHAMPSIM_DIR}/traces"

echo -e "\n=== [0/3] 编译 tracer ==="
cd "${CHAMPSIM_TRACER_DIR}"
# make clean
# make -j$(nproc)
echo "✅ tracer 编译成功"

echo -e "\n=== [1/3] 生成 trace: ${TRACE_NAME} ==="
${PIN_ROOT}/pin \
    -t "${TRACER}" \
    -o "${TRACE_FILE}" \
    -s ${SKIP_INS} \
    -t ${TRACE_NUM_INS} \
    -- "${TARGET_BIN}" ${PROGRAM_ARGS}

if [ ! -f "${TRACE_FILE}" ]; then
    echo "❌ trace 生成失败"
    exit 1
fi
echo "✅ trace 生成完成: ${TRACE_FILE}"

echo -e "\n=== [2/3] 运行 ChampSim ==="
${CHAMPSIM_BIN} "${TRACE_FILE}"

echo -e "\n🎉 全部成功！"