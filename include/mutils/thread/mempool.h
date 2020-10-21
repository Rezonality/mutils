
#pragma once

#include <concurrentqueue/concurrentqueue.h>
#include <cstdint>
#include <cassert>
#include <type_traits>

#include <ctti/type_id.hpp>

#include <gsl-lite/gsl-lite.hpp>

namespace MUtils
{

struct IListItem;

struct IMemoryPool
{
    virtual void Free(void* pEv) = 0;
    
    IListItem* m_pRoot = nullptr;
    IListItem* m_pLast = nullptr;

};

#define DECLARE_POOL_ITEM(className)                 \
    static ctti::type_id_t TypeID()                  \
    {                                                \
        return ctti::type_id<className>();           \
    }                                                \
    virtual ctti::type_id_t GetType() const override \
    {                                                \
        return TypeID();                             \
    }

struct IListItem
{
    IListItem(IMemoryPool* pPool)
        : m_pPool(pPool)
    {

    }
    // An item knows its owner pool so it can free itself
    IMemoryPool* m_pPool = nullptr;

    // Optionally sew this item into a list in the pool
    IListItem* m_pNext = nullptr;
    IListItem* m_pPrevious = nullptr;
};

IListItem* list_disconnect(gsl::not_null<IListItem*> pEvent);
IListItem* list_root(gsl::not_null<IListItem*> pEvent);
void list_insert_after(IListItem* pPos, gsl::not_null<IListItem*> pInsert);
void list_insert_before(IListItem* pPos, gsl::not_null<IListItem*> pInsert);
IListItem* list_disconnect(gsl::not_null<IListItem*> pEvent);
IListItem* list_disconnect_range(IListItem* pBegin, IListItem* pEnd);
IListItem* list_end(gsl::not_null<IListItem*> pEvent);

class PoolItem : public IListItem
{
public:
    PoolItem(IMemoryPool* pPool, uint64_t uniqueId)
        : IListItem(pPool)
        , m_id(uniqueId)
    {
    }

    virtual ~PoolItem() {}

    virtual void Free()
    {
        // When freed, disonnect from the chain we are in
        list_disconnect(gsl::not_null<IListItem*>(this));

        // And return to the pool
        m_pPool->Free(this);
    }

    virtual ctti::type_id_t GetType() const = 0;
    virtual void Init() = 0;

    uint64_t m_id = (uint64_t)-1;
};

// Thread safe memory pool
template <class T>
class TSMemoryPool : public IMemoryPool
{
    static_assert(std::is_base_of<PoolItem, T>::value, "T is not derived from PoolItem");

public:
    TSMemoryPool(uint32_t initialSize)
    {
        for (uint32_t i = 0; i < initialSize; i++)
        {
            m_freeItems.enqueue(Alloc());
        }
    }

    ~TSMemoryPool()
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

// Memory pool, not thread safe
template <class T>
class MemoryPool : public IMemoryPool
{
    static_assert(std::is_base_of<PoolItem, T>::value, "T is not derived from PoolItem");

public:
    MemoryPool(uint32_t initialSize)
    {
        for (uint32_t i = 0; i < initialSize; i++)
        {
            m_freeItems.emplace_back(Alloc());
        }
    }

    ~MemoryPool()
    {
        Clear();
    }

    void Clear()
    {
        // Final delete of free items
        for (auto& pVictim : m_freeItems)
        {
            delete pVictim;
        }
    }

    T* Alloc()
    {
        T* pRet = nullptr;
        if (m_freeItems.empty())
        {
            pRet = new T(this, m_nextId++);
        }
        else
        {
            pRet = m_freeItems.back();
            m_freeItems.pop_back();

            pRet->m_id = m_nextId++;
        }
        pRet->Init();
        return pRet;
    }

    void Free(void* pVal)
    {
        // store the free item for later
        auto pTyped = (T*)pVal;
        m_freeItems.push_back(pTyped);
    }

private:
    std::vector<T*> m_freeItems;
    uint64_t m_nextId = 0;
};

}; // namespace MUtils
