#include <mutils/thread/mempool.h>

namespace MUtils
{

IListItem* list_root(gsl::not_null<IListItem*> pEvent)
{
    auto pCheck = pEvent;
    while (pEvent && pEvent->m_pPrevious)
    {
        pEvent = gsl::not_null<IListItem*>(pEvent->m_pPrevious);
        assert(pEvent != pCheck);
    }
    return pEvent;
}

void list_insert_after(IListItem* pPos, gsl::not_null<IListItem*> pInsert)
{
    // If new, make it the begin/end of the pool
    auto pPool = pInsert->m_pPool;
    if (pPos == nullptr)
    {
        if (pPool)
        {
            // Insert after NULL means insert before first
            if (pPool->m_pRoot)
            {
                list_insert_before(pPool->m_pRoot, pInsert);
                return;
            }

            // Or start again
            assert(pPool->m_pLast == nullptr);
            pPool->m_pRoot = pInsert;
            pPool->m_pLast = pInsert;
            return;
        }
        else
        {
            assert(!"No pool?");
        }
    }

    // Making a new end
    if (pPool && pPool->m_pLast == pPos)
    {
        pPool->m_pLast = pInsert;
    }

    auto pAfter = pPos->m_pNext; // After insertion point
    pPos->m_pNext = pInsert; // Point insertion point to new node
    pInsert->m_pPrevious = pPos; // Point new node back at insertion point
    if (pAfter) // If something after us, point it back to us
    {
        pAfter->m_pPrevious = pInsert;
    }
    pInsert->m_pNext = pAfter; // Point us at the thing after

    if (pPool)
    {
        assert(pPool->m_pRoot->m_pPrevious == nullptr);
        assert(pPool->m_pLast->m_pNext == nullptr);
    }

#ifdef _DEBUG
    //timeline_validate(pInsert);
#endif
}

void list_insert_before(IListItem* pPos, gsl::not_null<IListItem*> pInsert)
{
    auto pPool = pInsert->m_pPool;
    if (pPos == nullptr)
    {
        if (pPool)
        {
            assert(pPool->m_pRoot == nullptr);
            assert(pPool->m_pLast == nullptr);
            pPool->m_pRoot = pInsert;
            pPool->m_pLast = pInsert;
            return;
        }
        else
        {
            assert(!"No pool?");
            return;
        }
    }

    assert(pPos);
    assert(pInsert);

    auto pBefore = pPos->m_pPrevious;
    pPos->m_pPrevious = pInsert;
    pInsert->m_pPrevious = pBefore;
    if (pBefore)
    {
        pBefore->m_pNext = pInsert;
    }
    pInsert->m_pNext = pPos;

#ifdef _DEBUG
    //timeline_validate(pInsert);
#endif
    if (pPool && pPool->m_pRoot == pPos)
    {
        pPool->m_pRoot = pInsert;
        assert(pPool->m_pRoot->m_pPrevious == nullptr);
        assert(pPool->m_pLast->m_pNext == nullptr);
    }
}

IListItem* list_disconnect(gsl::not_null<IListItem*> pEvent)
{
    // TODO: This is wrong, pool items if orphaned couldn't be freed.
    // So how to deal with disconnected chains that are later modified?
    // Perhaps that is not allowed?  But the chain must be freeable.
    auto pPool = pEvent->m_pPool;
    if (pPool)
    {
        if (pEvent == pPool->m_pRoot)
        {
            if (pEvent->m_pNext)
            {
                pPool->m_pRoot = pEvent->m_pNext;
            }
            else
            {
                pPool->m_pRoot = nullptr;
            }
        }

        if (pEvent == pPool->m_pLast)
        {
            if (pEvent->m_pPrevious)
            {
                pPool->m_pLast = pEvent->m_pPrevious;
            }
            else
            {
                pPool->m_pLast = nullptr;
            }
        }
    }

    auto pNext = pEvent->m_pNext;
    if (pEvent->m_pPrevious)
    {
        pEvent->m_pPrevious->m_pNext = pEvent->m_pNext;
    }

    if (pNext)
    {
        pNext->m_pPrevious = pEvent->m_pPrevious;
    }

    pEvent->m_pPrevious = nullptr;
    pEvent->m_pNext = nullptr;

#ifdef _DEBUG
    //timeline_validate(pEvent);
#endif

    if (pPool)
    {
        if (pPool->m_pRoot)
        {
            pPool->m_pRoot->m_pPrevious = nullptr;
        }

        if (pPool->m_pLast)
        {
            pPool->m_pLast->m_pNext = nullptr;
        }
    }

    return pNext;
}

IListItem* list_disconnect_range(IListItem* pBegin, IListItem* pEnd)
{
    // Find the end if null
    if (pEnd == nullptr)
    {
        assert(pBegin != nullptr);
        if (pBegin == nullptr)
        {
            return nullptr;
        }
        pEnd = pBegin;
        while (pEnd && pEnd->m_pNext)
        {
            pEnd = pEnd->m_pNext;
        }
    }

    // find the beginning if null
    if (pBegin == nullptr)
    {
        assert(pEnd != nullptr);
        if (pEnd == nullptr)
        {
            return nullptr;
        }
        pBegin = pEnd;
        while (pBegin && pBegin->m_pPrevious)
        {
            pBegin = pBegin->m_pPrevious;
        }
    }

    // Must have a valid chain
    if (pBegin == nullptr || pEnd == nullptr)
    {
        return nullptr;
    }

    auto pPool = pBegin->m_pPool;
    assert(pPool = pEnd->m_pPool);

    // Fix up root
    if (pBegin == pPool->m_pRoot)
    {
        if (pBegin->m_pNext)
        {
            pPool->m_pRoot = pBegin->m_pNext;
        }
        else
        {
            pPool->m_pRoot = nullptr;
        }
        assert(pPool->m_pRoot == nullptr || pPool->m_pRoot->m_pPrevious == nullptr);
    }

    if (pEnd == pPool->m_pLast)
    {
        if (pEnd->m_pPrevious)
        {
            pPool->m_pLast = pEnd->m_pPrevious;
        }
        else
        {
            pPool->m_pLast = nullptr;
        }
        assert(pPool->m_pLast == nullptr || pPool->m_pLast->m_pNext == nullptr);
    }

    if (pBegin == nullptr || pEnd == nullptr)
        return nullptr;

    // Disconnect from the chain
    auto pNext = pEnd->m_pNext;
    assert(pBegin != pEnd);

    if (pBegin->m_pPrevious)
    {
        pBegin->m_pPrevious->m_pNext = pEnd->m_pNext;
    }

    if (pEnd->m_pNext)
    {
        pEnd->m_pNext->m_pPrevious = pBegin->m_pPrevious;
    }

    pBegin->m_pPrevious = nullptr;
    pEnd->m_pNext = nullptr;

    // Validate the new chain; it has been sliced out
#ifdef _DEBUG
    //timeline_validate(pBegin);
#endif

    // Return the new strand
    return pNext;
}

IListItem* list_end(gsl::not_null<IListItem*> pEvent)
{
    while (pEvent && pEvent->m_pNext)
    {
        pEvent = gsl::not_null<IListItem*>(pEvent->m_pNext);
    }
    return pEvent;
}

} // namespace MUtils
