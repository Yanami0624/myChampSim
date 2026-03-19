#include <iostream>
using namespace std;

#define max(a, b) (a > b ? a : b)

int main() {
    int *arr = (int*) malloc (sizeof(int) * 20);
    for(int i = 0; i < 20; ++i)
        arr[i] = arr[max(i - 2, 0)] + arr[max(i - 1, 0)] + i;
    cout << endl;
    for(int i = 0; i < 20; ++i) cout << arr[i] << ' ';
    cout << endl;
    free(arr);
}