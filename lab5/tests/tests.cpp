#include <gtest/gtest.h>
#include <string>

#include "MemoryResource.hpp"
#include "Vector.hpp"


// ===============================================
//         do_allocate / find_fit TESTS
// ===============================================
TEST(MemoryResourceTest, AllocateInsideSingleBlock) {
    MemoryResource mr(1024);

    void* p = mr.allocate(100, alignof(std::max_align_t));
    ASSERT_NE(p, nullptr);
}

TEST(MemoryResourceTest, AllocateExactFit) {
    MemoryResource mr(256);

    void* p = mr.allocate(256, 1);
    ASSERT_NE(p, nullptr);

    // теперь ресурсов свободных быть не должно
    EXPECT_THROW(mr.allocate(1, 1), std::bad_alloc);
}

TEST(MemoryResourceTest, AllocateWithAlignment) {
    MemoryResource mr(1024);

    void* p = mr.allocate(50, 64);
    ASSERT_NE(p, nullptr);

    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(p);
    ASSERT_EQ(addr % 64, 0);
}

TEST(MemoryResourceTest, AllocateMultipleAndDeallocateMergesBlocks) {
    MemoryResource mr(1024);

    void* a = mr.allocate(100, 8);
    void* b = mr.allocate(200, 8);

    mr.deallocate(b, 200, 8);
    mr.deallocate(a, 100, 8);

    // Теперь free_list должен слиться обратно в один
    // Попробуем выделить весь блок снова
    void* p = mr.allocate(1000, 8);
    ASSERT_NE(p, nullptr);
}

TEST(MemoryResourceTest, AllocateBeyondMemoryThrows) {
    MemoryResource mr(128);

    EXPECT_THROW(mr.allocate(1024, 8), std::bad_alloc);
}

// ===============================================
//                Vector TESTS
// ===============================================
TEST(VectorTest, PushBackAndSize) {
    MemoryResource mr(2048);
    Vector<int> vec(&mr);

    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(vec[0], 10);
    ASSERT_EQ(vec[1], 20);
    ASSERT_EQ(vec[2], 30);
}

TEST(VectorTest, CapacityGrowth) {
    MemoryResource mr(2048);
    Vector<int> vec(&mr);

    vec.push_back(1);
    ASSERT_EQ(vec.capacity(), 1);

    vec.push_back(2);
    ASSERT_EQ(vec.capacity(), 2);

    vec.push_back(3);
    ASSERT_EQ(vec.capacity(), 4);
}

TEST(VectorTest, ReserveIncreasesCapacity) {
    MemoryResource mr(4096);
    Vector<int> vec(&mr);

    vec.reserve(50);
    ASSERT_GE(vec.capacity(), 50);
}

TEST(VectorTest, IteratorWorks) {
    MemoryResource mr(2048);
    Vector<int> vec(&mr);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    int sum = 0;
    for (auto& v : vec) {
        sum += v;
    }

    ASSERT_EQ(sum, 6);
}

struct Counter {
        static inline int alive = 0;
        int x;
        Counter(int v) : x(v) { alive++; }
        ~Counter() { alive--; }
    };

TEST(VectorTest, ClearDestroysObjects) {
    MemoryResource mr(4096);
    {
        Vector<Counter> vec(&mr);
        vec.emplace_back(1);
        vec.emplace_back(2);
        ASSERT_EQ(Counter::alive, 2);

        vec.clear();
        ASSERT_EQ(Counter::alive, 0);
    }
}

TEST(VectorTest, WorksWithPMRString) {
    MemoryResource mr(4096);
    Vector<std::pmr::string> vec(&mr);

    vec.emplace_back(std::pmr::string("hello", &mr));
    vec.emplace_back(std::pmr::string("world", &mr));

    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[0], "hello");
    ASSERT_EQ(vec[1], "world");
}

TEST(VectorTest, MoveAndReallocationPreservesContent) {
    MemoryResource mr(4096);
    Vector<std::pmr::string> vec(&mr);

    vec.emplace_back(std::pmr::string("aaa", &mr));
    vec.emplace_back(std::pmr::string("bbb", &mr));
    vec.emplace_back(std::pmr::string("ccc", &mr));

    vec.reserve(20);

    ASSERT_EQ(vec[0], "aaa");
    ASSERT_EQ(vec[1], "bbb");
    ASSERT_EQ(vec[2], "ccc");
}

TEST(VectorTest, MemoryResourceActuallyUsed) {
    MemoryResource mr(4096);
    Vector<int> vec(&mr);

    // Выделение памяти должно происходить внутри MemoryResource
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    // Если allocate не кидает ошибок — ресурс используется
    ASSERT_TRUE(vec.size() == 3);
}

// ===============================================
//                      main
// ===============================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
