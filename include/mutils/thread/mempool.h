
#pragma once

#include <cstdint>
#include <type_traits>
#include <concurrentqueue/concurrentqueue.h>

#include <ctti/type_id.hpp>

namespace MUtils
{

struct IMemoryPool
{
    virtual void Free(void* pEv) = 0;
};

#define DECLARE_POOL_ITEM(className)                  \
    static ctti::type_id_t TypeID()                         \
    {                                                       \
        return ctti::type_id<className>();                  \
    }                                                       \
    virtual ctti::type_id_t GetType() const override        \
    {                                                       \
        return TypeID();                                    \
    }


class PoolItem
{
public:
    PoolItem(IMemoryPool* pPool, uint64_t uniqueId)
        : m_pPool(pPool)
        , m_id(uniqueId)
    {
    }

    virtual ~PoolItem() {}


    virtual void Free()
    {
        m_pPool->Free(this);
    }

    virtual ctti::type_id_t GetType() const = 0;
    virtual void Init() = 0;

    IMemoryPool* m_pPool = nullptr;
    uint64_t m_id = (uint64_t)-1;
};

template <class T>
class MemoryPool : public IMemoryPool
{
    static_assert(std::is_base_of<PoolItem, T>::value, "T is not derived from PoolItem");
public:
    MemoryPool(uint32_t initialSize)
    {
        for (uint32_t i = 0; i < initialSize; i++)
        {
            Alloc()->Free();
        }
    }

    ~MemoryPool()
    {
        Clear();
    }

    void Clear()
    {
        // Final delete of free items
        T* pVictim = nullptr;
        while (m_freeItems.try_dequeue(pVictim))
        {
            delete pVictim;
        }
    }

    T* Alloc()
    {
        T* pRet = nullptr;

        if (!m_freeItems.try_dequeue(pRet))
        {
            pRet = new T(this, m_nextId++);
        }
        else
        {
            pRet->m_id = m_nextId++;
        }
        pRet->Init();
        return pRet;
    }

    void Free(void* pVal)
    {
        // store the free item for later
        auto pTyped = (T*)pVal;
        m_freeItems.enqueue(pTyped);
    }

private:
    moodycamel::ConcurrentQueue<T*> m_freeItems;
    uint64_t m_nextId = 0;
};

}; // namespace MUtils
