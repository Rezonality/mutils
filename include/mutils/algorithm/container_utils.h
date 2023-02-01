#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <string>

namespace MUtils
{

template<typename C, typename count, typename Op>
count accumulate_pairs(C& container, count c, Op fun)
{
    for (auto it = container.begin(); it != container.end(); it++)
    {
        for (auto it2 = it + 1; it2 != container.end(); it2++)
        {
            // Think I intended to add these?
            // Not really sure what this function was supposed to do!
            // I suspect for an advent of code....
            c = fun(c, *it, *it2);
            c = fun(c, *it2, *it);
        }
    }
    return c;
}

template<typename C, typename Op>
void container_test_pairs(C& container, Op fn)
{
    for (auto it = container.begin(); it != container.end(); it++)
    {
        for (auto it2 = it + 1; it2 != container.end(); it2++)
        {
            if (it != it2)
            {
                if (!fn(*it, *it2))
                    return;
            }
        }
    }
}

template<typename C, typename Op>
void container_test_triples(C& container, Op fn)
{
    for (auto it = container.begin(); it != container.end(); it++)
    {
        for (auto it2 = it + 1; it2 != container.end(); it2++)
        {
            if (it2 != it)
            {
                for (auto it3 = it2 + 1; it3 != container.end(); it3++)
                {
                    if ((it2 != it3) && (it != it3))
                    {
                        if (!fn(*it, *it2, *it3))
                            return;
                    }
                }
            }
        }
    }
}

template<typename T>
void container_splice(T& container, typename T::iterator itrStart, int size, std::function<void (typename T::iterator, typename T::iterator)> fn)
{
    auto itr = itrStart;
    auto itr2 = itrStart + size;
    while (itr2 < container.end())
    {
        fn(itr, itr2);
        if ((container.end() - itr2) < size)
            break;

        itr = itr + size;
        itr2 = itr2 + size;
    }
}

template<typename T>
std::map<T, T> vector_convert_to_pairs(const std::vector<T>& vals)
{
    std::map<T, T> pairs;
    auto itrFirst = vals.begin();
    while (itrFirst != vals.end())
    {
        auto itrSecond = itrFirst + 1;
        if (itrSecond != vals.end())
        {
            pairs[*itrFirst] = *itrSecond;
        }

        itrFirst = ++itrSecond;
    };
    return pairs;
}

template< typename ContainerT, typename PredicateT >
  void container_erase_if( ContainerT& items, const PredicateT& predicate ) {
    for( auto it = items.begin(); it != items.end(); ) {
      if( predicate(*it) ) it = items.erase(it);
      else ++it;
    }
  };

template <typename K, typename V>
V map_get_with_default(const  std::map <K, V> & m, const K & key, const V & defval) {
    typename std::map<K, V>::const_iterator it = m.find(key);
    if (it == m.end()) {
        return defval;
    }
    else {
        return it->second;
    }
}

template <template<class,class,class...> class C, typename K, typename V, typename F, typename... Args>
void map_apply(C<K,V,Args...>& m, K const& key, const V & defVal, F fn)
{
    typename C<K,V,Args...>::iterator it = m.find( key );
    if (it == m.end())
    {
        m[key] = defVal;
        fn(m[key]);
        return;
    }

    fn(it->second);
}

template <typename I, typename F>
void map_all(I& iterable, const F& functor)
{
    std::transform(iterable.begin(), iterable.end(), iterable.begin(), functor);
}

template <typename I, typename V, typename F>
std::set<V> filter_map_to_set(const std::map<I, V>& iterable, const F& functor )
{
    std::set<V> out;
    for (auto& entry : iterable)
    {
        if (functor(entry))
        {
            out.insert(entry.second);
        }
    }
    return out;
}

template <typename I, typename V, typename F>
std::vector<V> filter_map_to_vector(const std::map<I, V>& iterable, const F& functor )
{
    std::vector<V> out;
    for (auto& entry : iterable)
    {
        if (functor(entry))
        {
            out.push_back(entry.second);
        }
    }
    return out;
}

}
