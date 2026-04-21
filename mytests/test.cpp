// test.cpp
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <random>

using namespace std;

struct Obj {
    int id;
    size_t size;
    int* ptr;
};

int main() {
    cout << "Program start\n";

    // vector<size_t> sizes = {
    //     16,     // 64B
    //     64,     // 256B
    //     256,    // 1KB
    //     1024,   // 4KB
    //     4096,   // 16KB
    // };

    vector<size_t> sizes = {
        10,     // 64B
        100,     // 256B
        1000,    // 1KB
        10000,   // 4KB
        160000,   // 4KB
    };

    vector<Obj> objects;

    size_t total_bytes = 0;

    // -------------------------
    // malloc 不同大小对象
    // -------------------------
    for (int i = 0; i < sizes.size(); i++) {
        size_t n = sizes[i];

        int* p = (int*) malloc(sizeof(int) * n);

        if (!p) {
            cerr << "malloc failed\n";
            return 1;
        }

        // memset(p, 0, sizeof(int) * n);

        Obj obj;
        obj.id = i;
        obj.size = n;
        obj.ptr = p;

        objects.push_back(obj);

        size_t bytes = sizeof(int) * n;
        total_bytes += bytes;

        cout << "malloc object "
             << i
             << " addr=" << (void*)p
             << " size=" << bytes
             << " bytes\n";
    }

    cout << "\nTotal allocated bytes = "
         << total_bytes
         << "\n";

    cout << "vector size: " << objects.size() * sizeof(Obj) << endl;

    cout << "\n--- Sequential write ---\n";
    // for (auto& obj : objects) {
    //     for (size_t i = 0; i < obj.size; i++) {
    //         // obj.ptr[i] = i;
    //     }
    // }

    // cout << "\n--- Sequential read ---\n";
    // long long sum = 0;
    // for (auto& obj : objects) {
    //     for (size_t i = 0; i < obj.size; i++) {
    //         sum += obj.ptr[i];
    //     }
    // }
    // cout << "sum=" << sum << "\n";

    cout << "\n--- Random access ---\n";
    int total = 0;
    for(auto o: objects) {
        total += o.size;
    }
    auto count_oid = [&](int seed) {
        seed = seed % total;
        int r = 0;
        for(auto o: objects) {
            if(seed <= r + o.size) return o.id;
            r += o.size;
        }
        return 0;
    };

    volatile long long sink = 0;
    for (int round = 0; round < 100000; round++) {
        int obj_id = count_oid(rand());
        Obj& obj = objects[obj_id];
        size_t index = rand() % obj.size;
        sink += obj.ptr[index];
    }

    cout << "Random access finished\n";

    for (auto& obj : objects) {
        cout << "free object "
             << obj.id
             << " addr=" << (void*)obj.ptr
             << "\n";

        free(obj.ptr);
    }

    printf("sum: %lld\n", sink);
    cout << "Program end\n";

    return 0;
}