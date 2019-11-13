#pragma once
#include <vector>
#include <string>
#include <cassert>

namespace MUtils
{

template <typename T, typename Container = std::vector<T>, typename Iterator = Container::iterator>
class RingIterator
{
public:
    using difference_type = size_t;
    using value_type = T;
    using pointer = T * ;
    using reference = T & ;
    using iterator_category = std::bidirectional_iterator_tag;

private:
    Container& data;
    Iterator   cursor;
    Iterator   begin;
    Iterator   end;

public:
    RingIterator(Container& v) : data(v), cursor(v.begin()), begin(v.begin()), end(v.end()) {}
    RingIterator(Container& v, const Iterator& i) : data(v), cursor(i), begin(v.begin()), end(v.end()) {}
    RingIterator(Container& v, const Iterator& i, const Iterator& j) : data(v), cursor(i), begin(i), end(j) {}
    RingIterator(Container& v, size_t i) : data(v), cursor(v.begin() + i % v.size()), begin(v.begin()), end(v.end()) {}
    RingIterator(const RingIterator& r)
        : data(r.data)
    {
        *this = r;
    }


    const RingIterator& operator = (const RingIterator& rhs) 
    {
        assert(&rhs.data == &data);
        cursor = rhs.cursor;
        begin = rhs.begin;
        end = rhs.end;
        return *this;
    }
    
    bool operator == (const RingIterator& x) const { return cursor == x.cursor; }
    bool operator != (const RingIterator& x) const { return !(*this == x); }
    reference operator*() const { return *cursor; }
    RingIterator& operator++() { ++cursor; if (cursor == end) cursor = begin; return *this; }
    RingIterator& operator+(size_t count) { while (count-- > 0) { (*this)++; } return *this; }
    RingIterator& operator-(size_t count) { while (count-- > 0) { (*this)--; } return *this; }
    RingIterator operator++(int) { RingIterator ring = *this; ++*this; return ring; }
    RingIterator& operator--() { if (cursor == begin) cursor = end; --cursor; return *this; }
    RingIterator operator--(int) { RingIterator ring = *this; --*this; return ring; }
    RingIterator insert(const T& x) { return RingIterator(data, data.insert(cursor, x)); }
    RingIterator erase() { return RingIterator(data, data.erase(cursor)); }
    RingIterator insertAfter(const T& x) 
    {
        if (cursor != end)
        {
            return RingIterator(data, data.insert(++cursor, x));
        }
        else
        {
            return RingIterator(data, data.insert(cursor, x));
        }
    }
};

//using ringItr = RingIterator<char, std::string>;
//using ringItrVec = RingIterator<std::string, std::vector<std::string>>;
//using ringItrInt = RingIterator<int, std::vector<int>>;
//
//
//
} // MUtils

