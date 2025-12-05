#include <iostream>
#include <new>
#include <cassert>
#include <cstring>

#include "MemoryResource.hpp"
#include "Vector.hpp"

struct Pair {
    int a;
    double b;

    Pair(int a, double b)
        : a(a), b(b) {}
};

int main() {
    try {
        constexpr std::size_t BUFFER = 1024 * 20;
        MemoryResource fixed_res(BUFFER);

        {
            Vector<int> vec(&fixed_res);
            for (int i = 0; i < 10; ++i) {
                vec.push_back(i * 10);
                std::cout << "push " << i * 10 << ", size=" << vec.size() << ", cap=" << vec.capacity() << "\n";
            }

            std::cout << "\n";
            for (auto it = vec.begin(); it != vec.end(); ++it) {
                std::cout << *it << "\n";
            }
            std::cout << "\n";
        }
        {
            std::pmr::memory_resource* mr = &fixed_res;
            Vector<Pair> vec_pair(mr);

            for (int i = 0; i < 5; ++i) {
                vec_pair.emplace_back(i, i * 1.5);
                std::cout << "emplace a=" << vec_pair[vec_pair.size()-1].a << ", b=" << vec_pair[vec_pair.size()-1].b << "\n";
            }

            std::cout << "\n";
            for (auto it = vec_pair.begin(); it != vec_pair.end(); ++it) {
                std::cout << "a=" << it->a << ", b=" << it->b << "\n";
            }
        }

    } catch (const std::bad_alloc&) {
        std::cerr << "bad_alloc: не удалось выделить память из MemoryResource\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "exception: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
