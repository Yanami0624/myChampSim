
// test.cpp | used to genarate trace
#include <iostream>
#include <cstring>
using namespace std;

// #define N 3

#define max(a, b) (a > b ? a : b)

void func(int N) {
    int *arr = (int*) malloc (sizeof(int) * N);
    memset(arr, 0, sizeof(int) * N);
    for(int i = 0; i < N; ++i)
        arr[i] = arr[max(i - 2, 0)] + arr[max(i - 1, 0)] + i;
    for(int i = 0; i < N; ++i) cout << arr[i] << ' ';
        cout << endl;
    printf("arr addr: %p\n", arr);
    cout << endl;
    free(arr);
}

int main() {
    int nloop = 10;
    while(nloop--) {
        func(3);
    }
}