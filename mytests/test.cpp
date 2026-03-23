#include <iostream>
#include <vector>
using namespace std;

int main() {
    const int N = 1 << 12;

    vector<int> next(N);

    for (int i = 0; i < N; i++)
        next[i] = (i + 1) % N;

    int idx = 0;

    for (int i = 0; i < N; i++)
        idx = next[idx];

    cout << idx << endl;
}