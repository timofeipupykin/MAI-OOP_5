#include <memory_resource>
#include <cassert>
#include <cstring>


template <typename T>
class Vector {
public:
    using value_type = T;
    using allocator_type = std::pmr::polymorphic_allocator<T>;

    explicit Vector(std::pmr::memory_resource* mr = std::pmr::get_default_resource())
        : alloc_(mr), data_(nullptr), size_(0), capacity_(0)
    {}

    ~Vector() {
        clear();
        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }
    }

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    void push_back(const T& value) {
        if (size_ == capacity_) reserve(capacity_ ? capacity_ * 2 : 1);
        alloc_.construct(&data_[size_], value);
        ++size_;
    }

    void push_back(T&& value) {
        if (size_ == capacity_) reserve(capacity_ ? capacity_ * 2 : 1);
        alloc_.construct(&data_[size_], std::move(value));
        ++size_;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (size_ == capacity_) reserve(capacity_ ? capacity_ * 2 : 1);
        alloc_.construct(&data_[size_], std::forward<Args>(args)...);
        ++size_;
    }

    void reserve(std::size_t new_cap) {
        if (new_cap <= capacity_) return;
        T* new_data = alloc_.allocate(new_cap);
        for (std::size_t i = 0; i < size_; ++i) {
            alloc_.construct(&new_data[i], std::move_if_noexcept(data_[i]));
            alloc_.destroy(&data_[i]);
        }
        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }
        data_ = new_data;
        capacity_ = new_cap;
    }

    void clear() {
        for (std::size_t i = 0; i < size_; ++i) {
            alloc_.destroy(&data_[i]);
        }
        size_ = 0;
    }

    T& operator[](std::size_t idx) {
        assert(idx < size_);
        return data_[idx];
    }
    const T& operator[](std::size_t idx) const {
        assert(idx < size_);
        return data_[idx];
    }

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() : ptr_(nullptr) {}
        explicit iterator(T* p) : ptr_(p) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }

        iterator& operator++() { ptr_++; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const iterator& a, const iterator& b) { return a.ptr_ == b.ptr_; }
        friend bool operator!=(const iterator& a, const iterator& b) { return a.ptr_ != b.ptr_; }

    private:
        T* ptr_;
    };

    iterator begin() { return iterator(data_); }
    iterator end() { return iterator(data_ + size_); }

private:
    allocator_type alloc_;
    T* data_;
    std::size_t size_;
    std::size_t capacity_;
};