#include "threadpool.h"
#include <iostream>
int main() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; i++) {
        auto tp = [i] {
            std::cout << "hello " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "world " << i << std::endl;
            return i * i;
        };
        auto ans = pool.enqueue(std::move(tp));
        results.emplace_back(std::move(ans));
    }

    for (auto &&result : results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;

    return 0;
}