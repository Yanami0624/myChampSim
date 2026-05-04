// test the obj-statistic reliability
#include <iostream>
using namespace std;

int main() {
    // ===================== 严格遵守限制 =====================
    // 总对象大小：8MB (1024*1024*8) < 10MB ✔️
    // 无任何 std 容器 ✔️
    // ACCESS_TIMES = 100万 ✔️
    // ========================================================
    #define OBJ_SIZE        (8 * 1024 * 1024)  // 8MB 对象
    #define ACCESS_TIMES    100000            // 100万次访问

    // 只分配一个大堆对象（malloc → 会被你的工具追踪）
    char* obj = (char*)malloc(OBJ_SIZE);
    
    // 循环密集访问：所有地址都在 [obj, obj+OBJ_SIZE) 范围内
    cout << "start\n";
    char ele;
    for (long long i = 0; i < ACCESS_TIMES; ++i) {
        if(i % (ACCESS_TIMES / 10) == 0) cout << 'a' + (ele % 26) << ' ';
        // load
        ele = obj[rand() % OBJ_SIZE];
        obj[rand() % OBJ_SIZE] = ele;
    }
    cout << "\nend\n";

    // 释放内存
    free(obj);
    return 0;
}