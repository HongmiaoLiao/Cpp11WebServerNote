#include <iostream>

using namespace std;

int main() {

    size_t i = 100;
    size_t j = (i - 1) / 2;
    
    while (j >= 0) {
        if (i == 0) {
            break;
        }
        cout << i << " " << j << endl;
        i = j;
        j = (i - 1) / 2;
    }

    return 0;
}