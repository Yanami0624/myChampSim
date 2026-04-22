// test.cpp
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <random>

struct Obj {
    int id;
    size_t size;
    int* ptr;
};

const int NUM_OBJ = 8;
const int KB = 256;
const int MB = KB << 10;

size_t sizes[NUM_OBJ] = {
    MB,
    MB,
    MB,
    MB,
    MB,
    MB,
    MB,
    MB
};

// size_t sizes[NUM_OBJ] = {
//     KB,
//     KB,
//     KB,
//     KB,
//     KB,
//     KB,
//     KB,
//     KB
// };

Obj objects[NUM_OBJ];
size_t total_bytes = 0;

// ----------------------------
// 线性 mapping（避免 vector scan）
// ----------------------------
int get_obj_id(size_t x) {
    x = x % total_bytes;
    size_t acc = 0;
    for (int i = 0; i < NUM_OBJ; i++) {
        if (x < acc + sizes[i]) return i;
        acc += sizes[i];
    }
    return NUM_OBJ - 1;
}

int main() {
    printf("Program start\n");

    // ----------------------------
    // malloc objects（纯净）
    // ----------------------------
    for (int i = 0; i < NUM_OBJ; i++) {
        objects[i].id = i;
        objects[i].size = sizes[i];
        objects[i].ptr = (int*)malloc(sizeof(int) * sizes[i]);

        if (!objects[i].ptr) return 1;

        total_bytes += sizeof(int) * sizes[i];

        printf("malloc object %d addr=%p size=%zu bytes\n",
               i, objects[i].ptr, sizeof(int) * sizes[i]);
    }

    printf("\nTotal bytes = %zu\n", total_bytes);

    // ----------------------------
    // deterministic RNG（避免 rand noise）
    // ----------------------------
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 100000000);

    const int ROUND = 10000;

    // printf("\n--- write phase ---\n");
    // for (int i = 0; i < ROUND; i++) {
    //     int x = dist(rng) % total_bytes;
    //     int id = get_obj_id(x);

    //     Obj &o = objects[id];
    //     size_t idx = dist(rng) % o.size;

    //     o.ptr[idx] = x;
    // }


    printf("\n--- WRITE PHASE (dirty cache lines) ---\n");

    // 🔥 关键：让写发生“局部重复”
    for (int i = 0; i < ROUND; i++) {
        uint32_t x = dist(rng);
        int id = get_obj_id(x);

        Obj &o = objects[id];

        // 🔥 核心改动：限制在 cache line 内反复写
        size_t base = (x % (o.size / 16)) * 16;  // 16-int block

        for (int k = 0; k < 16; k++) {
            o.ptr[base + k] = x + k;
        }
    }

    printf("\n--- READ+WRITE MIX PHASE ---\n");

    printf("\n--- read phase ---\n");
    long long sink = 0;

    for (int i = 0; i < ROUND; i++) {
        int x = dist(rng) % total_bytes;
        int id = get_obj_id(x);

        Obj &o = objects[id];
        size_t idx = dist(rng) % o.size;

        sink += o.ptr[idx];
    }

    printf("sink=%lld\n", sink);

    // ----------------------------
    // free
    // ----------------------------
    for (int i = 0; i < NUM_OBJ; i++) {
        printf("free object %d addr=%p\n",
               i, objects[i].ptr);
        free(objects[i].ptr);
    }

    printf("Program end\n");
    return 0;
}