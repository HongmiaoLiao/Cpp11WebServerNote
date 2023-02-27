#include <iostream>
#include "./pool/threadpool.h"
#include <ctime>

int main() {
    ThreadPool threadPool(8);
    for (int i = 0; i < 100; ++i) {
        threadPool.AddTask([i](){
           std::cout << time(nullptr) << "----" << i << std::endl;
        });
    }
    std::cout << "Hello, World!" << std::endl;
    while(true) {

    }
    return 0;
}
