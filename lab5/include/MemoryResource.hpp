#include <memory_resource>
#include <list>
#include <new>
#include <cassert>
#include <cstring>
#include <stdexcept>


class MemoryResource : public std::pmr::memory_resource {
public:
    explicit MemoryResource(std::size_t total_size): total_size_(total_size) {
        buffer_ = static_cast<std::byte*>(::operator new(total_size_));
        free_list_.emplace_back(0, total_size_);
    }

    ~MemoryResource() override {
        ::operator delete(buffer_);
        buffer_ = nullptr;
        free_list_.clear();
        allocated_list_.clear();
    }

    MemoryResource(const MemoryResource&) = delete;
    MemoryResource& operator=(const MemoryResource&) = delete;

private:
    struct Block {
        std::size_t offset;
        std::size_t size;

        Block(std::size_t offset, std::size_t size): offset(offset), size(size) {}
    };

    std::list<Block>::iterator find_fit(std::size_t size, std::size_t alignment) {
        for (auto it = free_list_.begin(); it != free_list_.end(); ++it) {
            std::size_t start = it->offset;
            std::size_t aligned_start = align_up(start, alignment);

            if (aligned_start < start) continue;
            std::size_t padding = aligned_start - start;
            if (padding + size <= it->size) {
                return it;
            }
        }
        return free_list_.end();
    }

    static std::size_t align_up(std::size_t x, std::size_t alignment) {
        std::size_t mod = x % alignment;
        if (mod == 0) return x;
        return x + (alignment - mod);
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto it = find_fit(bytes, alignment);
        if (it == free_list_.end()) {
            throw std::bad_alloc();
        }

        std::size_t start = it->offset;
        std::size_t aligned_start = align_up(start, alignment);
        std::size_t preceding_padding = aligned_start - start;

        std::size_t remaining_after = it->size - (preceding_padding + bytes);

        allocated_list_.push_back({aligned_start, bytes});

        std::size_t original_offset = it->offset;
        std::size_t original_size = it->size;
        auto it_erase = it++;

        free_list_.erase(it_erase);

        if (preceding_padding > 0) {
            free_list_.emplace_back(original_offset, preceding_padding);
        }
        if (remaining_after > 0) {
            std::size_t after_offset = aligned_start + bytes;
            free_list_.emplace_back(after_offset, remaining_after);
        }
        return static_cast<void*>(buffer_ + aligned_start);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        if (!p) return;
        auto bytep = static_cast<std::byte*>(p);
        std::ptrdiff_t offset_pd = bytep - buffer_;
        if (offset_pd < 0 || static_cast<std::size_t>(offset_pd) >= total_size_) {
            throw std::runtime_error("deallocate: pointer not from this resource");
        }
        std::size_t offset = static_cast<std::size_t>(offset_pd);
        free_list_.emplace_back(offset, bytes);

        for (auto it = allocated_list_.begin(); it != allocated_list_.end(); ++it) {
            if (it->offset == offset && it->size == bytes) {
                allocated_list_.erase(it);
                break;
            }
        }

        free_list_.sort([](const Block& a, const Block& b){ return a.offset < b.offset; });

        for (auto it = free_list_.begin(); it != free_list_.end(); ) {
            auto next = std::next(it);
            if (next == free_list_.end()) break;
            if (it->offset + it->size == next->offset) {
                // merge
                it->size += next->size;
                free_list_.erase(next);
            } else {
                ++it;
            }
        }
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::byte* buffer_{nullptr};
    std::size_t total_size_{0};
    std::list<Block> free_list_;
    std::list<Block> allocated_list_;
};