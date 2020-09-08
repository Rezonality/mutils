#pragma once

namespace MUtils
{

// Example C++17 Allocator
template <typename T>
class stl_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T value_type;
    stl_allocator() {}
    ~stl_allocator() {}
    template <class U>
    stl_allocator(const stl_allocator<U>&) {}
    pointer allocate(size_type n)
    {
        return (pointer)malloc(n * sizeof(T));
    }
    void deallocate(pointer p, size_type n)
    {
        // Free knows how big the block is
        (void)&n;
        free(p);
    }
};

} // namespace MUtils
