#include <iostream>
using namespace std;

#define SIZE 1024
#define ACCESS (1024 * 1024)

int main() {
    int *arr = (int*)malloc(SIZE * sizeof(int));
    for(int i = 0; i < SIZE; ++i)
        arr[i] = i;
    int sum = 0;
    for(int i = 0; i < ACCESS; ++i) 
        sum += arr[random() % SIZE];
    cout << "sum: " << sum << endl;
    free(arr);
}