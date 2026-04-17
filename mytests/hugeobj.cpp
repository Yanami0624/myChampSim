// test.cpp
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <random>
#include <chrono>

using namespace std;

struct Obj {
    int id;
    size_t size;
    int* ptr;
};

int main() {
    cout << "Program start\n";

    // 关键：分配远大于 L2 缓存的大小（例如 8MB = 2048KB，远超常见 L2 256KB/512KB）
    vector<size_t> sizes = {
        1024 * 1024,   // 4MB
        1024 * 1024,   // 4MB
    };

    vector<Obj> objects;
    size_t total_bytes = 0;

    // -------------------------
    // malloc 大对象
    // -------------------------
    for (int i = 0; i < sizes.size(); i++) {
        size_t n = sizes[i];
        int* p = (int*)malloc(sizeof(int) * n);

        if (!p) {
            cerr << "malloc failed\n";
            return 1;
        }
        memset(p, 0, sizeof(int) * n);

        Obj obj;
        obj.id = i;
        obj.size = n;
        obj.ptr = p;
        objects.push_back(obj);

        size_t bytes = sizeof(int) * n;
        total_bytes += bytes;
        cout << "malloc object " << i
             << " addr=" << (void*)p
             << " size=" << bytes / 1024 << " KB\n";
    }

    cout << "\nTotal allocated = " << total_bytes / 1024 / 1024 << " MB\n";

    // -------------------------
    // 大量随机访问（真正读写，防止优化）
    // -------------------------
    cout << "\n--- Heavy random access (cause L2 miss) ---\n";

    int total_size = 0;
    for (auto& o : objects) total_size += o.size;

    volatile long long sink = 0;  // volatile 防止编译器优化掉内存访问

    // 循环次数足够多
    for (long long round = 0; round < 300; round++) {
        // 随机选一个对象
        int obj_id = rand() % objects.size();
        Obj& obj = objects[obj_id];

        // 随机索引
        size_t idx = rand() % obj.size;

        // 真正读写内存！！！
        sink += obj.ptr[idx];
    }

    cout << "Random access done, sink = " << sink << "\n";

    // -------------------------
    // free
    // -------------------------
    for (auto& obj : objects) {
        free(obj.ptr);
    }

    cout << "Program end\n";
    return 0;
}