#ifdef _MSC_VER
#  pragma warning( disable: 4244 4267 )  // conversion from don't care to whatever, possible loss of data 
#endif

#ifdef _WIN32
#  include <winsock2.h>
#endif

#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

#include <cctype>
#include <chrono>
#include <mutex>
#include <string.h>
#include <inttypes.h>

#if ( defined _MSC_VER && _MSVC_LANG >= 201703L ) || __cplusplus >= 201703L
#  if __has_include(<execution>)
#    include <execution>
#  else
#    define MY_LIBCPP_SUCKS
#  endif
#else
#  define MY_LIBCPP_SUCKS
#endif

#ifdef MY_LIBCPP_SUCKS
#  include "tracy_pdqsort.h"
#endif

#include "../common/TracyProtocol.hpp"
#include "../common/TracySystem.hpp"
#include "TracyFileRead.hpp"
#include "TracyFileWrite.hpp"
#include "TracyVersion.hpp"
#include "TracyWorker.hpp"

#include "tracy_flat_hash_map.hpp"

namespace tracy
{

static inline CallstackFrameId PackPointer( uint64_t ptr )
{
    assert( ( ( ptr & 0x4000000000000000 ) << 1 ) == ( ptr & 0x8000000000000000 ) );
    CallstackFrameId id;
    id.idx = ptr;
    id.sel = 0;
    return id;
}

static const uint8_t FileHeader[8] { 't', 'r', 'a', 'c', 'y', Version::Major, Version::Minor, Version::Patch };
enum { FileHeaderMagic = 5 };
static const int CurrentVersion = FileVersion( Version::Major, Version::Minor, Version::Patch );
static const int MinSupportedVersion = FileVersion( 0, 4, 0 );


static void UpdateLockCountLockable( LockMap& lockmap, size_t pos )
{
    auto& timeline = lockmap.timeline;
    bool isContended = lockmap.isContended;
    uint8_t lockingThread;
    uint8_t lockCount;
    uint64_t waitList;

    if( pos == 0 )
    {
        lockingThread = 0;
        lockCount = 0;
        waitList = 0;
    }
    else
    {
        const auto& tl = timeline[pos-1];
        lockingThread = tl.lockingThread;
        lockCount = tl.lockCount;
        waitList = tl.waitList;
    }
    const auto end = timeline.size();

    while( pos != end )
    {
        auto& tl = timeline[pos];
        const auto tbit = uint64_t( 1 ) << tl.ptr->thread;
        switch( (LockEvent::Type)tl.ptr->type )
        {
        case LockEvent::Type::Wait:
            waitList |= tbit;
            break;
        case LockEvent::Type::Obtain:
            assert( lockCount < std::numeric_limits<uint8_t>::max() );
            assert( ( waitList & tbit ) != 0 );
            waitList &= ~tbit;
            lockingThread = tl.ptr->thread;
            lockCount++;
            break;
        case LockEvent::Type::Release:
            assert( lockCount > 0 );
            lockCount--;
            break;
        default:
            break;
        }
        tl.lockingThread = lockingThread;
        tl.waitList = waitList;
        tl.lockCount = lockCount;
        if( !isContended ) isContended = lockCount != 0 && waitList != 0;
        pos++;
    }

    lockmap.isContended = isContended;
}

static void UpdateLockCountSharedLockable( LockMap& lockmap, size_t pos )
{
    auto& timeline = lockmap.timeline;
    bool isContended = lockmap.isContended;
    uint8_t lockingThread;
    uint8_t lockCount;
    uint64_t waitShared;
    uint64_t waitList;
    uint64_t sharedList;

    if( pos == 0 )
    {
        lockingThread = 0;
        lockCount = 0;
        waitShared = 0;
        waitList = 0;
        sharedList = 0;
    }
    else
    {
        const auto& tl = timeline[pos-1];
        const auto tlp = (LockEventShared*)tl.ptr;
        lockingThread = tl.lockingThread;
        lockCount = tl.lockCount;
        waitShared = tlp->waitShared;
        waitList = tl.waitList;
        sharedList = tlp->sharedList;
    }
    const auto end = timeline.size();

    // ObtainShared and ReleaseShared should assert on lockCount == 0, but
    // due to the async retrieval of data from threads that's not possible.
    while( pos != end )
    {
        auto& tl = timeline[pos];
        const auto tlp = (LockEventShared*)tl.ptr;
        const auto tbit = uint64_t( 1 ) << tlp->thread;
        switch( (LockEvent::Type)tlp->type )
        {
        case LockEvent::Type::Wait:
            waitList |= tbit;
            break;
        case LockEvent::Type::WaitShared:
            waitShared |= tbit;
            break;
        case LockEvent::Type::Obtain:
            assert( lockCount < std::numeric_limits<uint8_t>::max() );
            assert( ( waitList & tbit ) != 0 );
            waitList &= ~tbit;
            lockingThread = tlp->thread;
            lockCount++;
            break;
        case LockEvent::Type::Release:
            assert( lockCount > 0 );
            lockCount--;
            break;
        case LockEvent::Type::ObtainShared:
            assert( ( waitShared & tbit ) != 0 );
            assert( ( sharedList & tbit ) == 0 );
            waitShared &= ~tbit;
            sharedList |= tbit;
            break;
        case LockEvent::Type::ReleaseShared:
            assert( ( sharedList & tbit ) != 0 );
            sharedList &= ~tbit;
            break;
        default:
            break;
        }
        tl.lockingThread = lockingThread;
        tlp->waitShared = waitShared;
        tl.waitList = waitList;
        tlp->sharedList = sharedList;
        tl.lockCount = lockCount;
        if( !isContended ) isContended = ( lockCount != 0 && ( waitList != 0 || waitShared != 0 ) ) || ( sharedList != 0 && waitList != 0 );
        pos++;
    }

    lockmap.isContended = isContended;
}

static inline void UpdateLockCount( LockMap& lockmap, size_t pos )
{
    if( lockmap.type == LockType::Lockable )
    {
        UpdateLockCountLockable( lockmap, pos );
    }
    else
    {
        UpdateLockCountSharedLockable( lockmap, pos );
    }
}

static tracy_force_inline void WriteTimeOffset( FileWrite& f, int64_t& refTime, int64_t time )
{
    int64_t timeOffset = time - refTime;
    refTime += timeOffset;
    f.Write( &timeOffset, sizeof( timeOffset ) );
}

static tracy_force_inline int64_t ReadTimeOffset( FileRead& f, int64_t& refTime )
{
    int64_t timeOffset;
    f.Read( timeOffset );
    refTime += timeOffset;
    return refTime;
}

static tracy_force_inline void UpdateLockRange( LockMap& lockmap, const LockEvent& ev )
{
    const auto lt = ev.Time();
    auto& range = lockmap.range[ev.thread];
    if( range.start > lt ) range.start = lt;
    if( range.end < lt ) range.end = lt;
}

LoadProgress Worker::s_loadProgress;

Worker::Worker( const char* addr )
    : m_addr( addr )
    , m_hasData( false )
    , m_stream( LZ4_createStreamDecode() )
    , m_buffer( new char[TargetFrameSize*3 + 1] )
    , m_bufferOffset( 0 )
    , m_pendingStrings( 0 )
    , m_pendingThreads( 0 )
    , m_pendingExternalNames( 0 )
    , m_pendingSourceLocation( 0 )
    , m_pendingCallstackFrames( 0 )
    , m_pendingCallstackSubframes( 0 )
    , m_callstackFrameStaging( nullptr )
    , m_traceVersion( CurrentVersion )
    , m_loadTime( 0 )
{
    m_data.sourceLocationExpand.push_back( 0 );
    m_data.localThreadCompress.InitZero();
    m_data.callstackPayload.push_back( nullptr );

    memset( m_gpuCtxMap, 0, sizeof( m_gpuCtxMap ) );

#ifndef TRACY_NO_STATISTICS
    m_data.sourceLocationZonesReady = true;
#endif

    m_thread = std::thread( [this] { SetThreadName( "Tracy Worker" ); Exec(); } );
}

Worker::Worker( FileRead& f, EventType::Type eventMask )
    : m_hasData( true )
    , m_stream( nullptr )
    , m_buffer( nullptr )
{
    auto loadStart = std::chrono::high_resolution_clock::now();

    m_data.callstackPayload.push_back( nullptr );

    int fileVer = 0;

    uint8_t hdr[8];
    f.Read( hdr, sizeof( hdr ) );
    if( memcmp( FileHeader, hdr, FileHeaderMagic ) == 0 )
    {
        fileVer = FileVersion( hdr[FileHeaderMagic], hdr[FileHeaderMagic+1], hdr[FileHeaderMagic+2] );
        if( fileVer > CurrentVersion )
        {
            throw UnsupportedVersion( fileVer );
        }
        if( fileVer < MinSupportedVersion )
        {
            throw LegacyVersion( fileVer );
        }

        f.Read( m_delay );
    }
    else
    {
        throw LegacyVersion( FileVersion( 0, 2, 0 ) );
    }
    m_traceVersion = fileVer;

    if( fileVer <= FileVersion( 0, 4, 8 ) )
    {
        s_loadProgress.total.store( 8, std::memory_order_relaxed );
    }
    else if( fileVer <= FileVersion( 0, 5, 0 ) )
    {
        s_loadProgress.total.store( 9, std::memory_order_relaxed );
    }
    else if( fileVer <= FileVersion( 0, 5, 2 ) )
    {
        s_loadProgress.total.store( 10, std::memory_order_relaxed );
    }
    else
    {
        s_loadProgress.total.store( 11, std::memory_order_relaxed );
    }

    s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
    s_loadProgress.progress.store( LoadProgress::Initialization, std::memory_order_relaxed );
    f.Read( m_resolution );
    f.Read( m_timerMul );
    f.Read( m_data.lastTime );
    f.Read( m_data.frameOffset );

    if( fileVer >= FileVersion( 0, 5, 5 ) )
    {
        f.Read( m_pid );
    }
    else
    {
        m_pid = 0;
    }

    uint64_t sz;
    {
        f.Read( sz );
        assert( sz < 1024 );
        char tmp[1024];
        f.Read( tmp, sz );
        m_captureName = std::string( tmp, tmp+sz );
    }
    {
        f.Read( sz );
        assert( sz < 1024 );
        char tmp[1024];
        f.Read( tmp, sz );
        m_captureProgram = std::string( tmp, tmp+sz );
        f.Read( m_captureTime );
    }
    {
        f.Read( sz );
        assert( sz < 1024 );
        char tmp[1024];
        f.Read( tmp, sz );
        m_hostInfo = std::string( tmp, tmp+sz );
    }

    f.Read( &m_data.crashEvent, sizeof( m_data.crashEvent ) );

    f.Read( sz );
    m_data.frames.Data().reserve_exact( sz, m_slab );
    for( uint64_t i=0; i<sz; i++ )
    {
        auto ptr = m_slab.AllocInit<FrameData>();
        f.Read( &ptr->name, sizeof( ptr->name ) );
        f.Read( &ptr->continuous, sizeof( ptr->continuous ) );
        uint64_t fsz;
        f.Read( &fsz, sizeof( fsz ) );
        ptr->frames.reserve_exact( fsz, m_slab );
        if( fileVer >= FileVersion( 0, 4, 9 ) )
        {
            int64_t refTime = 0;
            if( ptr->continuous )
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    ptr->frames[j].start = ReadTimeOffset( f, refTime );
                    ptr->frames[j].end = -1;
                    f.Read( &ptr->frames[j].frameImage, sizeof( int32_t ) );
                }
            }
            else
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    ptr->frames[j].start = ReadTimeOffset( f, refTime );
                    ptr->frames[j].end = ReadTimeOffset( f, refTime );
                    f.Read( &ptr->frames[j].frameImage, sizeof( int32_t ) );
                }
            }
        }
        else if( fileVer >= FileVersion( 0, 4, 2 ) )
        {
            int64_t refTime = 0;
            if( ptr->continuous )
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    ptr->frames[j].start = ReadTimeOffset( f, refTime );
                    ptr->frames[j].end = -1;
                    ptr->frames[j].frameImage = -1;
                }
            }
            else
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    ptr->frames[j].start = ReadTimeOffset( f, refTime );
                    ptr->frames[j].end = ReadTimeOffset( f, refTime );
                    ptr->frames[j].frameImage = -1;
                }
            }
        }
        else
        {
            if( ptr->continuous )
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    f.Read( &ptr->frames[j].start, sizeof( int64_t ) );
                    ptr->frames[j].end = -1;
                    ptr->frames[j].frameImage = -1;
                }
            }
            else
            {
                for( uint64_t j=0; j<fsz; j++ )
                {
                    f.Read( &ptr->frames[j].start, sizeof( int64_t ) );
                    f.Read( &ptr->frames[j].end, sizeof( int64_t ) );
                    ptr->frames[j].frameImage = -1;
                }
            }
        }
        m_data.frames.Data()[i] = ptr;
    }
    m_data.framesBase = m_data.frames.Data()[0];
    assert( m_data.framesBase->name == 0 );

    if( fileVer < FileVersion( 0, 5, 2 ) )
    {
        m_data.baseTime = m_data.framesBase->frames[0].start;
        m_data.lastTime -= m_data.baseTime;
        if( m_data.crashEvent.time != 0 ) m_data.crashEvent.time -= m_data.baseTime;
        for( auto& fd : m_data.frames.Data() )
        {
            if( fd->continuous )
            {
                for( auto& fe : fd->frames )
                {
                    fe.start -= m_data.baseTime;
                }
            }
            else
            {
                for( auto& fe : fd->frames )
                {
                    fe.start -= m_data.baseTime;
                    fe.end -= m_data.baseTime;
                }
            }
        }
    }

    flat_hash_map<uint64_t, const char*, nohash<uint64_t>> pointerMap;

    f.Read( sz );
    m_data.stringData.reserve_exact( sz, m_slab );
    for( uint64_t i=0; i<sz; i++ )
    {
        uint64_t ptr, ssz;
        f.Read2( ptr, ssz );
        auto dst = m_slab.Alloc<char>( ssz+1 );
        f.Read( dst, ssz );
        dst[ssz] = '\0';
        m_data.stringData[i] = ( dst );
        pointerMap.emplace( ptr, dst );
    }

    f.Read( sz );
    for( uint64_t i=0; i<sz; i++ )
    {
        uint64_t id, ptr;
        f.Read2( id, ptr );
        auto it = pointerMap.find( ptr );
        if( it != pointerMap.end() )
        {
            m_data.strings.emplace( id, it->second );
        }
    }

    f.Read( sz );
    for( uint64_t i=0; i<sz; i++ )
    {
        uint64_t id, ptr;
        f.Read2( id, ptr );
        auto it = pointerMap.find( ptr );
        if( it != pointerMap.end() )
        {
            m_data.threadNames.emplace( id, it->second );
        }
    }

    if( fileVer >= FileVersion( 0, 5, 3 ) )
    {
        f.Read( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            uint64_t id, ptr, ptr2;
            f.Read2( id, ptr );
            f.Read( ptr2 );
            auto it = pointerMap.find( ptr );
            auto it2 = pointerMap.find( ptr2 );
            if( it != pointerMap.end() && it2 != pointerMap.end() )
            {
                m_data.externalNames.emplace( id, std::make_pair( it->second, it2->second ) );
            }
        }
    }

    m_data.localThreadCompress.Load( f, fileVer );
    if( fileVer >= FileVersion( 0, 5, 6 ) )
    {
        m_data.externalThreadCompress.Load( f, fileVer );
    }

    f.Read( sz );
    for( uint64_t i=0; i<sz; i++ )
    {
        uint64_t ptr;
        f.Read( ptr );
        SourceLocation srcloc;
        f.Read( srcloc );
        m_data.sourceLocation.emplace( ptr, srcloc );
    }

    f.Read( sz );
    m_data.sourceLocationExpand.reserve_exact( sz, m_slab );
    f.Read( m_data.sourceLocationExpand.data(), sizeof( uint64_t ) * sz );
    const auto sle = sz;

    f.Read( sz );
    m_data.sourceLocationPayload.reserve_exact( sz, m_slab );
    for( uint64_t i=0; i<sz; i++ )
    {
        auto srcloc = m_slab.Alloc<SourceLocation>();
        f.Read( srcloc, sizeof( *srcloc ) );
        m_data.sourceLocationPayload[i] = srcloc;
        m_data.sourceLocationPayloadMap.emplace( srcloc, int16_t( i ) );
    }

#ifndef TRACY_NO_STATISTICS
    m_data.sourceLocationZonesReady = false;
    m_data.sourceLocationZones.reserve( sle + sz );

    f.Read( sz );
    if( fileVer >= FileVersion( 0, 5, 2 ) )
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            int16_t id;
            uint64_t cnt;
            f.Read( id );
            f.Read( cnt );
            auto status = m_data.sourceLocationZones.emplace( id, SourceLocationZones() );
            assert( status.second );
            status.first->second.zones.reserve( cnt );
        }
    }
    else
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            int32_t id;
            uint64_t cnt;
            f.Read( id );
            f.Read( cnt );
            auto status = m_data.sourceLocationZones.emplace( int16_t( id ), SourceLocationZones() );
            assert( status.second );
            status.first->second.zones.reserve( cnt );
        }
    }
#else
    f.Read( sz );
    if( fileVer >= FileVersion( 0, 5, 2 ) )
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            int16_t id;
            f.Read( id );
            f.Skip( sizeof( uint64_t ) );
            m_data.sourceLocationZonesCnt.emplace( id, 0 );
        }
    }
    else
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            int32_t id;
            f.Read( id );
            f.Skip( sizeof( uint64_t ) );
            m_data.sourceLocationZonesCnt.emplace( int16_t( id ), 0 );
        }
    }
#endif

    s_loadProgress.progress.store( LoadProgress::Locks, std::memory_order_relaxed );
    f.Read( sz );
    if( eventMask & EventType::Locks )
    {
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        for( uint64_t i=0; i<sz; i++ )
        {
            s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
            auto lockmapPtr = m_slab.AllocInit<LockMap>();
            auto& lockmap = *lockmapPtr;
            uint32_t id;
            uint64_t tsz;
            f.Read( id );
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                f.Read( lockmap.srcloc );
            }
            else
            {
                int32_t srcloc;
                f.Read( srcloc );
                lockmap.srcloc = int16_t( srcloc );
            }
            f.Read( lockmap.type );
            f.Read( lockmap.valid );
            lockmap.isContended = false;
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                f.Read2( lockmap.timeAnnounce, lockmap.timeTerminate );
            }
            else if( fileVer >= FileVersion( 0, 4, 1 ) )
            {
                f.Read2( lockmap.timeAnnounce, lockmap.timeTerminate );
                lockmap.timeAnnounce -= m_data.baseTime;
                lockmap.timeTerminate -= m_data.baseTime;
                if( lockmap.timeTerminate < lockmap.timeAnnounce ) lockmap.timeTerminate = 0;
            }
            else
            {
                lockmap.timeAnnounce = lockmap.timeTerminate = 0;
            }
            f.Read( tsz );
            for( uint64_t i=0; i<tsz; i++ )
            {
                uint64_t t;
                f.Read( t );
                lockmap.threadMap.emplace( t, lockmap.threadList.size() );
                lockmap.threadList.emplace_back( t );
            }
            f.Read( tsz );
            lockmap.timeline.reserve_exact( tsz, m_slab );
            auto ptr = lockmap.timeline.data();
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                int64_t refTime = lockmap.timeAnnounce;
                if( lockmap.type == LockType::Lockable )
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEvent>();
                        lev->SetTime( ReadTimeOffset( f, refTime ) );
                        int16_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( srcloc );
                        f.Read( &lev->thread, sizeof( LockEvent::thread ) + sizeof( LockEvent::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
                else
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEventShared>();
                        lev->SetTime( ReadTimeOffset( f, refTime ) );
                        int16_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( srcloc );
                        f.Read( &lev->thread, sizeof( LockEventShared::thread ) + sizeof( LockEventShared::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
            }
            else if( fileVer >= FileVersion( 0, 4, 2 ) )
            {
                int64_t refTime = lockmap.timeAnnounce;
                if( lockmap.type == LockType::Lockable )
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEvent>();
                        lev->SetTime( ReadTimeOffset( f, refTime ) );
                        int32_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( int16_t( srcloc ) );
                        f.Read( &lev->thread, sizeof( LockEvent::thread ) + sizeof( LockEvent::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
                else
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEventShared>();
                        lev->SetTime( ReadTimeOffset( f, refTime ) );
                        int32_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( int16_t( srcloc ) );
                        f.Read( &lev->thread, sizeof( LockEventShared::thread ) + sizeof( LockEventShared::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
            }
            else
            {
                if( lockmap.type == LockType::Lockable )
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEvent>();
                        int64_t time;
                        f.Read( time );
                        lev->SetTime( time - m_data.baseTime );
                        int32_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( int16_t( srcloc ) );
                        f.Read( &lev->thread, sizeof( LockEvent::thread ) + sizeof( LockEvent::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
                else
                {
                    for( uint64_t i=0; i<tsz; i++ )
                    {
                        auto lev = m_slab.Alloc<LockEventShared>();
                        int64_t time;
                        f.Read( time );
                        lev->SetTime( time - m_data.baseTime );
                        int32_t srcloc;
                        f.Read( srcloc );
                        lev->SetSrcLoc( int16_t( srcloc ) );
                        f.Read( &lev->thread, sizeof( LockEventShared::thread ) + sizeof( LockEventShared::type ) );
                        *ptr++ = { lev };
                        UpdateLockRange( lockmap, *lev );
                    }
                }
            }
            UpdateLockCount( lockmap, 0 );
            m_data.lockMap.emplace( id, lockmapPtr );
        }
    }
    else
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            LockType type;
            uint64_t tsz;
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                f.Skip( sizeof( uint32_t ) + sizeof( LockMap::srcloc ) );
            }
            else
            {
                f.Skip( sizeof( uint32_t ) + sizeof( int32_t ) );
            }
            f.Read( type );
            f.Skip( sizeof( LockMap::valid ) );
            if( fileVer >= FileVersion( 0, 4, 1 ) )
            {
                f.Skip( sizeof( LockMap::timeAnnounce ) + sizeof( LockMap::timeTerminate ) );
            }
            f.Read( tsz );
            f.Skip( tsz * sizeof( uint64_t ) );
            f.Read( tsz );
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                f.Skip( tsz * ( sizeof( int64_t ) + sizeof( int16_t ) + sizeof( LockEvent::thread ) + sizeof( LockEvent::type ) ) );
            }
            else
            {
                f.Skip( tsz * ( sizeof( int64_t ) + sizeof( int32_t ) + sizeof( LockEvent::thread ) + sizeof( LockEvent::type ) ) );
            }
        }
    }

    s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
    s_loadProgress.progress.store( LoadProgress::Messages, std::memory_order_relaxed );
    flat_hash_map<uint64_t, MessageData*, nohash<uint64_t>> msgMap;
    f.Read( sz );
    if( eventMask & EventType::Messages )
    {
        m_data.messages.reserve_exact( sz, m_slab );
        if( fileVer >= FileVersion( 0, 5, 2 ) )
        {
            int64_t refTime = 0;
            for( uint64_t i=0; i<sz; i++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                auto msgdata = m_slab.Alloc<MessageData>();
                msgdata->time = ReadTimeOffset( f, refTime );
                f.Read( msgdata->ref );
                f.Read( msgdata->color );
                m_data.messages[i] = msgdata;
                msgMap.emplace( ptr, msgdata );
            }
        }
        else if( fileVer >= FileVersion( 0, 4, 8 ) )
        {
            int64_t refTime = -m_data.baseTime;
            for( uint64_t i=0; i<sz; i++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                auto msgdata = m_slab.Alloc<MessageData>();
                msgdata->time = ReadTimeOffset( f, refTime );
                f.Read( msgdata->ref );
                f.Read( msgdata->color );
                m_data.messages[i] = msgdata;
                msgMap.emplace( ptr, msgdata );
            }
        }
        else if( fileVer >= FileVersion( 0, 4, 2 ) )
        {
            int64_t refTime = -m_data.baseTime;
            for( uint64_t i=0; i<sz; i++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                auto msgdata = m_slab.Alloc<MessageData>();
                msgdata->time = ReadTimeOffset( f, refTime );
                f.Read( msgdata->ref );
                msgdata->color = 0xFFFFFFFF;
                m_data.messages[i] = msgdata;
                msgMap.emplace( ptr, msgdata );
            }
        }
        else
        {
            for( uint64_t i=0; i<sz; i++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                auto msgdata = m_slab.Alloc<MessageData>();
                f.Read( msgdata, sizeof( MessageData::time ) + sizeof( MessageData::ref ) );
                msgdata->time -= m_data.baseTime;
                msgdata->color = 0xFFFFFFFF;
                m_data.messages[i] = msgdata;
                msgMap.emplace( ptr, msgdata );
            }
        }
    }
    else
    {
        if( fileVer <= FileVersion( 0, 4, 7 ) )
        {
            f.Skip( sz * ( sizeof( uint64_t ) + sizeof( MessageData::time ) + sizeof( MessageData::ref ) ) );
        }
        else
        {
            f.Skip( sz * ( sizeof( uint64_t ) + sizeof( MessageData::time ) + sizeof( MessageData::ref ) + sizeof( MessageData::color ) ) );
        }
    }

    s_loadProgress.progress.store( LoadProgress::Zones, std::memory_order_relaxed );
    if( fileVer >= FileVersion( 0, 4, 7 ) )
    {
        f.Read( sz );
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        s_loadProgress.subProgress.store( 0, std::memory_order_relaxed );
    }
    f.Read( sz );
    m_data.threads.reserve_exact( sz, m_slab );
    for( uint64_t i=0; i<sz; i++ )
    {
        auto td = m_slab.AllocInit<ThreadData>();
        uint64_t tid;
        f.Read( tid );
        td->id = tid;
        f.Read( td->count );
        uint64_t tsz;
        f.Read( tsz );
        if( fileVer < FileVersion( 0, 4, 7 ) )
        {
            s_loadProgress.subTotal.store( td->count, std::memory_order_relaxed );
            s_loadProgress.subProgress.store( 0, std::memory_order_relaxed );
        }
        if( tsz != 0 )
        {
            if( fileVer <= FileVersion( 0, 4, 1 ) )
            {
                ReadTimelinePre042( f, td->timeline, CompressThread( tid ), tsz, fileVer );
            }
            else if( fileVer <= FileVersion( 0, 5, 1 ) )
            {
                int64_t refTime = 0;
                ReadTimelinePre052( f, td->timeline, CompressThread( tid ), tsz, refTime, fileVer );
            }
            else
            {
                int64_t refTime = 0;
                ReadTimeline( f, td->timeline, CompressThread( tid ), tsz, refTime );
            }
        }
        uint64_t msz;
        f.Read( msz );
        if( eventMask & EventType::Messages )
        {
            const auto ctid = CompressThread( tid );
            td->messages.reserve_exact( msz, m_slab );
            for( uint64_t j=0; j<msz; j++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                auto md = msgMap[ptr];
                td->messages[j] = md;
                md->thread = ctid;
            }
        }
        else
        {
            f.Skip( msz * sizeof( uint64_t ) );
        }
        m_data.threads[i] = td;
    }

    s_loadProgress.progress.store( LoadProgress::GpuZones, std::memory_order_relaxed );
    if( fileVer >= FileVersion( 0, 4, 7 ) )
    {
        f.Read( sz );
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        s_loadProgress.subProgress.store( 0, std::memory_order_relaxed );
    }
    f.Read( sz );
    m_data.gpuData.reserve_exact( sz, m_slab );
    for( uint64_t i=0; i<sz; i++ )
    {
        auto ctx = m_slab.AllocInit<GpuCtxData>();
        f.Read( ctx->thread );
        f.Read( ctx->accuracyBits );
        f.Read( ctx->count );
        if( fileVer < FileVersion( 0, 4, 7 ) )
        {
            s_loadProgress.subTotal.store( ctx->count, std::memory_order_relaxed );
            s_loadProgress.subProgress.store( 0, std::memory_order_relaxed );
        }
        int64_t refTime = 0;
        int64_t refGpuTime = 0;
        f.Read( ctx->period );
        uint64_t tsz;
        f.Read( tsz );
        if( tsz != 0 )
        {
            if( fileVer <= FileVersion( 0, 5, 1 ) )
            {
                ReadTimelinePre052( f, ctx->timeline, tsz, refTime, refGpuTime, fileVer );
            }
            else
            {
                ReadTimeline( f, ctx->timeline, tsz, refTime, refGpuTime );
            }
        }
        m_data.gpuData[i] = ctx;
    }

    s_loadProgress.progress.store( LoadProgress::Plots, std::memory_order_relaxed );
    f.Read( sz );
    if( eventMask & EventType::Plots )
    {
        m_data.plots.Data().reserve( sz );
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        for( uint64_t i=0; i<sz; i++ )
        {
            s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
            auto pd = m_slab.AllocInit<PlotData>();
            if( fileVer >= FileVersion( 0, 4, 5 ) )
            {
                f.Read( pd->type );
            }
            else
            {
                pd->type = PlotType::User;
            }
            f.Read( pd->name );
            f.Read( pd->min );
            f.Read( pd->max );
            uint64_t psz;
            f.Read( psz );
            pd->data.reserve_exact( psz, m_slab );
            if( fileVer >= FileVersion( 0, 5, 2 ) )
            {
                int64_t refTime = 0;
                for( uint64_t j=0; j<psz; j++ )
                {
                    pd->data[j].time = ReadTimeOffset( f, refTime );
                    f.Read( pd->data[j].val );
                }
            }
            else if( fileVer >= FileVersion( 0, 4, 2 ) )
            {
                int64_t refTime = -m_data.baseTime;
                for( uint64_t j=0; j<psz; j++ )
                {
                    pd->data[j].time = ReadTimeOffset( f, refTime );
                    f.Read( pd->data[j].val );
                }
            }
            else
            {
                f.Read( pd->data.data(), psz * sizeof( PlotItem ) );
                for( uint64_t j=0; j<psz; j++ )
                {
                    pd->data[j].time -= m_data.baseTime;
                }
            }
            m_data.plots.Data().push_back_no_space_check( pd );
        }
    }
    else
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            if( fileVer >= FileVersion( 0, 4, 5 ) )
            {
                f.Skip( sizeof( PlotData::name ) + sizeof( PlotData::min ) + sizeof( PlotData::max ) + sizeof( PlotData::type ) );
            }
            else
            {
                f.Skip( sizeof( PlotData::name ) + sizeof( PlotData::min ) + sizeof( PlotData::max ) );
            }
            uint64_t psz;
            f.Read( psz );
            f.Skip( psz * sizeof( PlotItem ) );
        }
    }

    bool reconstructMemAllocPlot = false;

    s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
    s_loadProgress.progress.store( LoadProgress::Memory, std::memory_order_relaxed );
    f.Read( sz );
    if( eventMask & EventType::Memory )
    {
        m_data.memory.data.reserve_exact( sz, m_slab );
        uint64_t activeSz, freesSz;
        f.Read2( activeSz, freesSz );
        m_data.memory.active.reserve( activeSz );
        m_data.memory.frees.reserve_exact( freesSz, m_slab );
        auto mem = m_data.memory.data.data();
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        size_t fidx = 0;
        int64_t refTime = 0;
        if( fileVer >= FileVersion( 0, 5, 2 ) )
        {
            auto& frees = m_data.memory.frees;
            auto& active = m_data.memory.active;

            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                f.Read( mem, sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) + sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) );
                int64_t timeAlloc, timeFree;
                uint16_t threadAlloc, threadFree;
                f.Read2( timeAlloc, timeFree );
                f.Read2( threadAlloc, threadFree );
                refTime += timeAlloc;
                mem->SetTimeAlloc( refTime );
                if( timeFree >= 0 )
                {
                    mem->SetTimeFree( timeFree + refTime );
                    frees[fidx++] = i;
                }
                else
                {
                    mem->SetTimeFree( timeFree );
                    active.emplace( mem->ptr, i );
                }
                mem->SetThreadAlloc( threadAlloc );
                mem->SetThreadFree( threadFree );
                mem++;
            }
        }
        else if( fileVer >= FileVersion( 0, 4, 4 ) )
        {
            auto& frees = m_data.memory.frees;
            auto& active = m_data.memory.active;

            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                f.Read( mem, sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) );
                int64_t timeAlloc, timeFree;
                f.Read2( timeAlloc, timeFree );
                f.Read( &mem->csAlloc, sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) );
                uint16_t threadAlloc, threadFree;
                f.Read2( threadAlloc, threadFree );
                refTime += timeAlloc;
                mem->SetTimeAlloc( refTime - m_data.baseTime );
                if( timeFree >= 0 )
                {
                    mem->SetTimeFree( timeFree + refTime - m_data.baseTime );
                    frees[fidx++] = i;
                }
                else
                {
                    mem->SetTimeFree( timeFree );
                    active.emplace( mem->ptr, i );
                }
                mem->SetThreadAlloc( threadAlloc );
                mem->SetThreadFree( threadFree );
                mem++;
            }
        }
        else
        {
            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                if( fileVer > FileVersion( 0, 4, 1 ) )
                {
                    f.Read( mem, sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) );
                    int64_t timeAlloc, timeFree;
                    f.Read2( timeAlloc, timeFree );
                    f.Read( &mem->csAlloc, sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) );
                    refTime += timeAlloc;
                    mem->SetTimeAlloc( refTime - m_data.baseTime );
                    if( timeFree >= 0 )
                    {
                        mem->SetTimeFree( timeFree + refTime - m_data.baseTime );
                    }
                    else
                    {
                        mem->SetTimeFree( timeFree );
                    }
                }
                else
                {
                    f.Read( mem, sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) );
                    int64_t timeAlloc, timeFree;
                    f.Read2( timeAlloc, timeFree );
                    f.Read( &mem->csAlloc, sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) );
                    mem->SetTimeAlloc( timeAlloc - m_data.baseTime );
                    if( timeFree >= 0 )
                    {
                        mem->SetTimeFree( timeFree - m_data.baseTime );
                    }
                    else
                    {
                        mem->SetTimeFree( timeFree );
                    }
                }

                uint64_t t0, t1;
                f.Read2( t0, t1 );
                mem->SetThreadAlloc( CompressThread( t0 ) );
                if( t0 == t1 )
                {
                    mem->SetThreadFree( mem->ThreadAlloc() );
                }
                else
                {
                    mem->SetThreadFree( CompressThread( t1 ) );
                }

                if( mem->TimeFree() < 0 )
                {
                    m_data.memory.active.emplace( mem->ptr, i );
                }
                else
                {
                    m_data.memory.frees[fidx++] = i;
                }

                mem++;
            }
        }

        f.Read( m_data.memory.high );
        f.Read( m_data.memory.low );
        f.Read( m_data.memory.usage );

        if( sz != 0 )
        {
            reconstructMemAllocPlot = true;
        }
    }
    else
    {
        f.Skip( 2 * sizeof( uint64_t ) );

        if( fileVer >= FileVersion( 0, 5, 2 ) )
        {
            f.Skip( sz * ( sizeof( MemEvent::ptr ) + + sizeof( MemEvent::size ) + sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) + sizeof( MemEvent::_time_thread_alloc ) + sizeof( MemEvent::_time_thread_free ) ) );
        }
        else if( fileVer >= FileVersion( 0, 4, 4 ) )
        {
            f.Skip( sz * ( sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) + sizeof( int64_t ) + sizeof( int64_t ) + sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) + sizeof( uint16_t ) + sizeof( uint16_t ) ) );
        }
        else if( fileVer > FileVersion( 0, 4, 1 ) )
        {
            f.Skip( sz * ( sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) + sizeof( int64_t ) + sizeof( int64_t ) + sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) + 2 * sizeof( uint64_t ) ) );
        }
        else
        {
            f.Skip( sz * ( sizeof( MemEvent::ptr ) + sizeof( MemEvent::size ) + sizeof( int64_t ) + sizeof( int64_t ) + sizeof( MemEvent::csAlloc ) + sizeof( MemEvent::csFree ) + 2 * sizeof( uint64_t ) ) );
        }

        f.Skip( sizeof( MemData::high ) + sizeof( MemData::low ) + sizeof( MemData::usage ) );
    }

    s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
    s_loadProgress.progress.store( LoadProgress::CallStacks, std::memory_order_relaxed );
    f.Read( sz );
    m_data.callstackPayload.reserve( sz );
    if( fileVer >= FileVersion( 0, 4, 6 ) )
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            uint8_t csz;
            f.Read( csz );

            const auto memsize = sizeof( VarArray<CallstackFrameId> ) + csz * sizeof( CallstackFrameId );
            auto mem = (char*)m_slab.AllocRaw( memsize );

            auto data = (CallstackFrameId*)mem;
            f.Read( data, csz * sizeof( CallstackFrameId ) );

            auto arr = (VarArray<CallstackFrameId>*)( mem + csz * sizeof( CallstackFrameId ) );
            new(arr) VarArray<CallstackFrameId>( csz, data );

            m_data.callstackPayload.push_back_no_space_check( arr );
        }
    }
    else
    {
        for( uint64_t i=0; i<sz; i++ )
        {
            uint8_t csz;
            f.Read( csz );

            const auto memsize = sizeof( VarArray<CallstackFrameId> ) + csz * sizeof( CallstackFrameId );
            auto mem = (char*)m_slab.AllocRaw( memsize );

            auto data = (CallstackFrameId*)mem;
            for( uint8_t j=0; j<csz; j++ )
            {
                uint64_t ptr;
                f.Read( ptr );
                data[j] = PackPointer( ptr );
            }

            auto arr = (VarArray<CallstackFrameId>*)( mem + csz * sizeof( CallstackFrameId ) );
            new(arr) VarArray<CallstackFrameId>( csz, data );

            m_data.callstackPayload.push_back_no_space_check( arr );
        }
    }

    if( fileVer >= FileVersion( 0, 4, 6 ) )
    {
        f.Read( sz );
        m_data.callstackFrameMap.reserve( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            CallstackFrameId id;
            f.Read( id );

            auto frameData = m_slab.Alloc<CallstackFrameData>();
            f.Read( frameData->size );

            frameData->data = m_slab.Alloc<CallstackFrame>( frameData->size );
            f.Read( frameData->data, sizeof( CallstackFrame ) * frameData->size );

            m_data.callstackFrameMap.emplace( id, frameData );
        }
    }
    else if( fileVer >= FileVersion( 0, 4, 3 ) )
    {
        f.Read( sz );
        m_data.callstackFrameMap.reserve( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            uint64_t ptr;
            f.Read( ptr );

            auto frameData = m_slab.Alloc<CallstackFrameData>();
            f.Read( frameData->size );

            frameData->data = m_slab.Alloc<CallstackFrame>( frameData->size );
            f.Read( frameData->data, sizeof( CallstackFrame ) * frameData->size );

            m_data.callstackFrameMap.emplace( PackPointer( ptr ), frameData );
        }
    }
    else
    {
        f.Read( sz );
        m_data.callstackFrameMap.reserve( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            uint64_t ptr;
            f.Read( ptr );

            auto frameData = m_slab.Alloc<CallstackFrameData>();
            frameData->size = 1;

            frameData->data = m_slab.Alloc<CallstackFrame>();
            f.Read( frameData->data, sizeof( CallstackFrame ) );

            m_data.callstackFrameMap.emplace( PackPointer( ptr ), frameData );
        }
    }

    if( fileVer >= FileVersion( 0, 4, 11 ) )
    {
        f.Read( sz );
        if( sz > 0 )
        {
            m_data.appInfo.reserve_exact( sz, m_slab );
            f.Read( m_data.appInfo.data(), sizeof( m_data.appInfo[0] ) * sz );
        }
    }

    if( fileVer >= FileVersion( 0, 4, 9 ) )
    {
        s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
        s_loadProgress.progress.store( LoadProgress::FrameImages, std::memory_order_relaxed );

        if( eventMask & EventType::FrameImages )
        {
            size_t tmpbufsz = 0;
            char* tmpbuf = nullptr;
            f.Read( sz );
            m_data.frameImage.reserve_exact( sz, m_slab );
            s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                auto fi = m_slab.Alloc<FrameImage>();
                f.Read2( fi->w, fi->h );
                f.Read( fi->flip );
                const auto sz = size_t( fi->w * fi->h / 2 );
                if( tmpbufsz < sz )
                {
                    tmpbufsz = sz;
                    delete[] tmpbuf;
                    tmpbuf = new char[sz];
                }
                f.Read( tmpbuf, sz );
                fi->ptr = PackFrameImage( tmpbuf, fi->w, fi->h, fi->csz );
                m_data.frameImage[i] = fi;
            }
            delete[] tmpbuf;

            const auto& frames = GetFramesBase()->frames;
            const auto fsz = uint32_t( frames.size() );
            for( uint32_t i=0; i<fsz; i++ )
            {
                const auto& f = frames[i];
                if( f.frameImage != -1 )
                {
                    m_data.frameImage[f.frameImage]->frameRef = i;
                }
            }
        }
        else
        {
            f.Read( sz );
            s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                uint16_t w, h;
                f.Read2( w, h );
                const auto fisz = w * h / 2;
                f.Skip( fisz + sizeof( FrameImage::flip ) );
            }
        }
    }

    if( fileVer >= FileVersion( 0, 5, 6 ) )
    {
        s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
        s_loadProgress.progress.store( LoadProgress::ContextSwitches, std::memory_order_relaxed );

        if( eventMask & EventType::ContextSwitches )
        {
            f.Read( sz );
            s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
            m_data.ctxSwitch.reserve( sz );
            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                uint64_t thread, csz;
                f.Read2( thread, csz );
                auto data = m_slab.AllocInit<ContextSwitch>();
                data->v.reserve_exact( csz, m_slab );
                int64_t runningTime = 0;
                int64_t refTime = 0;
                auto ptr = data->v.data();
                for( uint64_t j=0; j<csz; j++ )
                {
                    ptr->wakeup = ReadTimeOffset( f, refTime );
                    ptr->SetStart( ReadTimeOffset( f, refTime ) );
                    int64_t diff;
                    f.Read( diff );
                    if( diff > 0 ) runningTime += diff;
                    refTime += diff;
                    ptr->SetEnd( refTime );
                    uint8_t cpu;
                    int8_t reason;
                    int8_t state;
                    f.Read( cpu );
                    f.Read( reason );
                    f.Read( state );
                    ptr->SetCpu( cpu );
                    ptr->SetReason( reason );
                    ptr->SetState( state );
                    ptr++;
                }
                data->runningTime = runningTime;
                m_data.ctxSwitch.emplace( thread, data );
            }
        }
        else
        {
            f.Read( sz );
            s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
            for( uint64_t i=0; i<sz; i++ )
            {
                s_loadProgress.subProgress.store( i, std::memory_order_relaxed );
                f.Skip( sizeof( uint64_t ) );
                uint64_t csz;
                f.Read( csz );
                f.Skip( csz * ( sizeof( int64_t ) * 3 + sizeof( int8_t ) * 3 ) );
            }
        }

        s_loadProgress.subTotal.store( 0, std::memory_order_relaxed );
        s_loadProgress.progress.store( LoadProgress::ContextSwitchesPerCpu, std::memory_order_relaxed );
        f.Read( sz );
        s_loadProgress.subTotal.store( sz, std::memory_order_relaxed );
        if( eventMask & EventType::ContextSwitches )
        {
            uint64_t cnt = 0;
            for( int i=0; i<256; i++ )
            {
                int64_t refTime = 0;
                f.Read( sz );
                m_data.cpuData[i].cs.reserve_exact( sz, m_slab );
                auto ptr = m_data.cpuData[i].cs.data();
                for( uint64_t j=0; j<sz; j++ )
                {
                    ptr->SetStart( ReadTimeOffset( f, refTime ) );
                    ptr->SetEnd( ReadTimeOffset( f, refTime ) );
                    uint16_t thread;
                    f.Read( thread );
                    ptr->SetThread( thread );
                    ptr++;
                }
                cnt += sz;
                s_loadProgress.subProgress.store( cnt, std::memory_order_relaxed );
            }
        }
        else
        {
            for( int i=0; i<256; i++ )
            {
                f.Read( sz );
                f.Skip( sz * ( sizeof( int64_t ) * 2 + sizeof( uint16_t ) ) );
            }
        }

        f.Read( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            uint64_t tid, pid;
            f.Read2( tid, pid );
            m_data.tidToPid.emplace( tid, pid );
        }

        f.Read( sz );
        for( uint64_t i=0; i<sz; i++ )
        {
            uint64_t tid;
            CpuThreadData data;
            f.Read( tid );
            f.Read( data );
            m_data.cpuThreadData.emplace( tid, data );
        }
    }

    s_loadProgress.total.store( 0, std::memory_order_relaxed );
    m_loadTime = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now() - loadStart ).count();

    m_backgroundDone.store( false, std::memory_order_relaxed );
#ifndef TRACY_NO_STATISTICS
    m_threadBackground = std::thread( [this, reconstructMemAllocPlot] {
        std::function<void(const Vector<ZoneEvent*>&, uint16_t)> ProcessTimeline;
        ProcessTimeline = [this, &ProcessTimeline] ( const Vector<ZoneEvent*>& vec, uint16_t thread )
        {
            for( auto& zone : vec )
            {
                ReadTimelineUpdateStatistics( zone, thread );
                if( zone->child >= 0 )
                {
                    ProcessTimeline( GetZoneChildren( zone->child ), thread );
                }
            }
        };

        for( auto& t : m_data.threads )
        {
            if( !t->timeline.empty() )
            {
                // Don't touch thread compression cache in a thread.
                ProcessTimeline( t->timeline, m_data.localThreadCompress.DecompressMustRaw( t->id ) );
            }
        }
        for( auto& v : m_data.sourceLocationZones )
        {
            auto& zones = v.second.zones;
#ifdef MY_LIBCPP_SUCKS
            pdqsort_branchless( zones.begin(), zones.end(), []( const auto& lhs, const auto& rhs ) { return lhs.zone->Start() < rhs.zone->Start(); } );
#else
            std::sort( std::execution::par_unseq, zones.begin(), zones.end(), []( const auto& lhs, const auto& rhs ) { return lhs.zone->Start() < rhs.zone->Start(); } );
#endif
        }
        {
            std::lock_guard<std::shared_mutex> lock( m_data.lock );
            m_data.sourceLocationZonesReady = true;
        }
        if( reconstructMemAllocPlot ) ReconstructMemAllocPlot();
        m_backgroundDone.store( true, std::memory_order_relaxed );
    } );
#else
    if( reconstructMemAllocPlot )
    {
        m_threadBackground = std::thread( [this] { ReconstructMemAllocPlot(); m_backgroundDone.store( true, std::memory_order_relaxed ); } );
    }
#endif
}

Worker::~Worker()
{
    Shutdown();

    if( m_thread.joinable() ) m_thread.join();
    if( m_threadBackground.joinable() ) m_threadBackground.join();

    delete[] m_buffer;
    LZ4_freeStreamDecode( m_stream );

    delete[] m_frameImageBuffer;

    for( auto& v : m_data.threads )
    {
        v->timeline.~Vector();
        v->stack.~Vector();
        v->messages.~Vector();
        v->zoneIdStack.~Vector();
    }
    for( auto& v : m_data.gpuData )
    {
        v->timeline.~Vector();
        v->stack.~Vector();
    }
    for( auto& v : m_data.plots.Data() )
    {
        v->~PlotData();
    }
    for( auto& v : m_data.frames.Data() )
    {
        v->~FrameData();
    }
    for( auto& v : m_data.lockMap )
    {
        v.second->~LockMap();
    }
}

uint64_t Worker::GetLockCount() const
{
    uint64_t cnt = 0;
    for( auto& l : m_data.lockMap )
    {
        cnt += l.second->timeline.size();
    }
    return cnt;
}

uint64_t Worker::GetPlotCount() const
{
    uint64_t cnt = 0;
    for( auto& p : m_data.plots.Data() )
    {
        if( p->type != PlotType::Memory )
        {
            cnt += p->data.size();
        }
    }
    return cnt;
}

uint64_t Worker::GetContextSwitchCount() const
{
    uint64_t cnt = 0;
    for( auto& v : m_data.ctxSwitch )
    {
        cnt += v.second->v.size();
    }
    return cnt;
}

uint64_t Worker::GetContextSwitchPerCpuCount() const
{
    uint64_t cnt = 0;
    for( int i=0; i<256; i++ )
    {
        cnt += m_data.cpuData[i].cs.size();
    }
    return cnt;
}

uint64_t Worker::GetPidFromTid( uint64_t tid ) const
{
    auto it = m_data.tidToPid.find( tid );
    if( it == m_data.tidToPid.end() ) return 0;
    return it->second;
}

const ContextSwitch* const Worker::GetContextSwitchDataImpl( uint64_t thread )
{
    auto it = m_data.ctxSwitch.find( thread );
    if( it != m_data.ctxSwitch.end() )
    {
        m_data.ctxSwitchLast.first = thread;
        m_data.ctxSwitchLast.second = it->second;
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

size_t Worker::GetFullFrameCount( const FrameData& fd ) const
{
    const auto sz = fd.frames.size();
    assert( sz != 0 );

    if( fd.continuous )
    {
        if( IsConnected() )
        {
            return sz - 1;
        }
        else
        {
            return sz;
        }
    }
    else
    {
        const auto& last = fd.frames.back();
        if( last.end >= 0 )
        {
            return sz;
        }
        else
        {
            return sz - 1;
        }
    }
}

int64_t Worker::GetFrameTime( const FrameData& fd, size_t idx ) const
{
    if( fd.continuous )
    {
        if( idx < fd.frames.size() - 1 )
        {
            return fd.frames[idx+1].start - fd.frames[idx].start;
        }
        else
        {
            assert( m_data.lastTime != 0 );
            return m_data.lastTime - fd.frames.back().start;
        }
    }
    else
    {
        const auto& frame = fd.frames[idx];
        if( frame.end >= 0 )
        {
            return frame.end - frame.start;
        }
        else
        {
            return m_data.lastTime - fd.frames.back().start;
        }
    }
}

int64_t Worker::GetFrameBegin( const FrameData& fd, size_t idx ) const
{
    assert( idx < fd.frames.size() );
    return fd.frames[idx].start;
}

int64_t Worker::GetFrameEnd( const FrameData& fd, size_t idx ) const
{
    if( fd.continuous )
    {
        if( idx < fd.frames.size() - 1 )
        {
            return fd.frames[idx+1].start;
        }
        else
        {
            return m_data.lastTime;
        }
    }
    else
    {
        if( fd.frames[idx].end >= 0 )
        {
            return fd.frames[idx].end;
        }
        else
        {
            return m_data.lastTime;
        }
    }
}

const FrameImage* Worker::GetFrameImage( const FrameData& fd, size_t idx ) const
{
    assert( idx < fd.frames.size() );
    const auto& v = fd.frames[idx].frameImage;
    if( v < 0 ) return nullptr;
    return m_data.frameImage[v];
}

std::pair<int, int> Worker::GetFrameRange( const FrameData& fd, int64_t from, int64_t to )
{
    auto zitbegin = std::lower_bound( fd.frames.begin(), fd.frames.end(), from, [] ( const auto& lhs, const auto& rhs ) { return lhs.start < rhs; } );
    if( zitbegin == fd.frames.end() ) zitbegin--;

    const auto zitend = std::lower_bound( zitbegin, fd.frames.end(), to, [] ( const auto& lhs, const auto& rhs ) { return lhs.start < rhs; } );

    int zbegin = std::distance( fd.frames.begin(), zitbegin );
    if( zbegin > 0 && zitbegin->start != from ) --zbegin;
    const int zend = std::distance( fd.frames.begin(), zitend );

    return std::make_pair( zbegin, zend );
}

const CallstackFrameData* Worker::GetCallstackFrame( const CallstackFrameId& ptr ) const
{
    auto it = m_data.callstackFrameMap.find( ptr );
    if( it == m_data.callstackFrameMap.end() )
    {
        return nullptr;
    }
    else
    {
        return it->second;
    }
}

int64_t Worker::GetZoneEnd( const ZoneEvent& ev )
{
    auto ptr = &ev;
    for(;;)
    {
        if( ptr->end >= 0 ) return ptr->end;
        if( ptr->child < 0 ) return ptr->Start();
        ptr = GetZoneChildren( ptr->child ).back();
    }
}

int64_t Worker::GetZoneEnd( const GpuEvent& ev )
{
    auto ptr = &ev;
    for(;;)
    {
        if( ptr->gpuEnd >= 0 ) return ptr->gpuEnd;
        if( ptr->child < 0 ) return ptr->gpuStart;
        ptr = GetGpuChildren( ptr->child ).back();
    }
}

const char* Worker::GetString( uint64_t ptr ) const
{
    const auto it = m_data.strings.find( ptr );
    if( it == m_data.strings.end() || it->second == nullptr )
    {
        return "???";
    }
    else
    {
        return it->second;
    }
}

const char* Worker::GetString( const StringRef& ref ) const
{
    if( ref.isidx )
    {
        assert( ref.active );
        return m_data.stringData[ref.str];
    }
    else
    {
        if( ref.active )
        {
            return GetString( ref.str );
        }
        else
        {
            return "???";
        }
    }
}

const char* Worker::GetString( const StringIdx& idx ) const
{
    assert( idx.active );
    return m_data.stringData[idx.idx];
}

const char* Worker::GetThreadName( uint64_t id ) const
{
    const auto it = m_data.threadNames.find( id );
    if( it == m_data.threadNames.end() )
    {
        const auto eit = m_data.externalNames.find( id );
        if( eit == m_data.externalNames.end() )
        {
            return "???";
        }
        else
        {
            return eit->second.second;
        }
    }
    else
    {
        // Client should send additional information about thread name, to make this check unnecessary
        const auto txt = it->second;
        if( txt[0] >= '0' && txt[0] <= '9' && atoi( txt ) == id )
        {
            const auto eit = m_data.externalNames.find( id );
            if( eit != m_data.externalNames.end() )
            {
                return eit->second.second;
            }
        }
        return txt;
    }
}

bool Worker::IsThreadLocal( uint64_t id ) const
{
    return m_data.localThreadCompress.Exists( id );
}

const SourceLocation& Worker::GetSourceLocation( int16_t srcloc ) const
{
    if( srcloc < 0 )
    {
        return *m_data.sourceLocationPayload[-srcloc-1];
    }
    else
    {
        const auto it = m_data.sourceLocation.find( m_data.sourceLocationExpand[srcloc] );
        assert( it != m_data.sourceLocation.end() );
        return it->second;
    }
}

std::pair<const char*, const char*> Worker::GetExternalName( uint64_t id ) const
{
    const auto it = m_data.externalNames.find( id );
    if( it == m_data.externalNames.end() )
    {
        return std::make_pair( "???", "???" );
    }
    else
    {
        return it->second;
    }
}

const char* Worker::GetZoneName( const SourceLocation& srcloc ) const
{
    if( srcloc.name.active )
    {
        return GetString( srcloc.name );
    }
    else
    {
        return GetString( srcloc.function );
    }
}

const char* Worker::GetZoneName( const ZoneEvent& ev ) const
{
    auto& srcloc = GetSourceLocation( ev.SrcLoc() );
    return GetZoneName( ev, srcloc );
}

const char* Worker::GetZoneName( const ZoneEvent& ev, const SourceLocation& srcloc ) const
{
    if( ev.name.active )
    {
        return GetString( ev.name );
    }
    else if( srcloc.name.active )
    {
        return GetString( srcloc.name );
    }
    else
    {
        return GetString( srcloc.function );
    }
}

const char* Worker::GetZoneName( const GpuEvent& ev ) const
{
    auto& srcloc = GetSourceLocation( ev.srcloc );
    return GetZoneName( ev, srcloc );
}

const char* Worker::GetZoneName( const GpuEvent& ev, const SourceLocation& srcloc ) const
{
    if( srcloc.name.active )
    {
        return GetString( srcloc.name );
    }
    else
    {
        return GetString( srcloc.function );
    }
}

static bool strstr_nocase( const char* l, const char* r )
{
    const auto lsz = strlen( l );
    const auto rsz = strlen( r );
    auto ll = (char*)alloca( lsz + 1 );
    auto rl = (char*)alloca( lsz + 1 );
    for( size_t i=0; i<lsz; i++ )
    {
        ll[i] = tolower( l[i] );
    }
    ll[lsz] = '\0';
    for( size_t i=0; i<rsz; i++ )
    {
        rl[i] = tolower( r[i] );
    }
    rl[rsz] = '\0';
    return strstr( ll, rl ) != nullptr;
}

std::vector<int16_t> Worker::GetMatchingSourceLocation( const char* query, bool ignoreCase ) const
{
    std::vector<int16_t> match;

    const auto sz = m_data.sourceLocationExpand.size();
    for( size_t i=1; i<sz; i++ )
    {
        const auto it = m_data.sourceLocation.find( m_data.sourceLocationExpand[i] );
        assert( it != m_data.sourceLocation.end() );
        const auto& srcloc = it->second;
        const auto str = GetString( srcloc.name.active ? srcloc.name : srcloc.function );
        bool found = false;
        if( ignoreCase )
        {
            found = strstr_nocase( str, query );
        }
        else
        {
            found = strstr( str, query ) != nullptr;
        }
        if( found )
        {
            match.push_back( (int16_t)i );
        }
    }

    for( auto& srcloc : m_data.sourceLocationPayload )
    {
        const auto str = GetString( srcloc->name.active ? srcloc->name : srcloc->function );
        bool found = false;
        if( ignoreCase )
        {
            found = strstr_nocase( str, query );
        }
        else
        {
            found = strstr( str, query ) != nullptr;
        }
        if( found )
        {
            auto it = m_data.sourceLocationPayloadMap.find( srcloc );
            assert( it != m_data.sourceLocationPayloadMap.end() );
            match.push_back( -int16_t( it->second + 1 ) );
        }
    }

    return match;
}

#ifndef TRACY_NO_STATISTICS
const Worker::SourceLocationZones& Worker::GetZonesForSourceLocation( int16_t srcloc ) const
{
    static const SourceLocationZones empty;
    auto it = m_data.sourceLocationZones.find( srcloc );
    return it != m_data.sourceLocationZones.end() ? it->second : empty;
}
#endif

void Worker::Exec()
{
    auto ShouldExit = [this]
    {
        return m_shutdown.load( std::memory_order_relaxed );
    };

    for(;;)
    {
        if( m_shutdown.load( std::memory_order_relaxed ) ) return;
        if( m_sock.Connect( m_addr.c_str(), "8086" ) ) break;
    }

    auto lz4buf = std::make_unique<char[]>( LZ4Size );

    std::chrono::time_point<std::chrono::high_resolution_clock> t0;

    uint64_t bytes = 0;
    uint64_t decBytes = 0;

    m_sock.Send( HandshakeShibboleth, HandshakeShibbolethSize );
    uint32_t protocolVersion = ProtocolVersion;
    m_sock.Send( &protocolVersion, sizeof( protocolVersion ) );
    HandshakeStatus handshake;
    if( !m_sock.Read( &handshake, sizeof( handshake ), 10, ShouldExit ) )
    {
        m_handshake.store( HandshakeDropped, std::memory_order_relaxed );
        goto close;
    }
    m_handshake.store( handshake, std::memory_order_relaxed );
    switch( handshake )
    {
    case HandshakeWelcome:
        break;
    case HandshakeProtocolMismatch:
    case HandshakeNotAvailable:
    default:
        goto close;
    }

    m_data.framesBase = m_data.frames.Retrieve( 0, [this] ( uint64_t name ) {
        auto fd = m_slab.AllocInit<FrameData>();
        fd->name = name;
        fd->continuous = 1;
        return fd;
    }, [this] ( uint64_t name ) {
        assert( name == 0 );
        char tmp[6] = "Frame";
        HandleFrameName( name, tmp, 5 );
    } );

    {
        WelcomeMessage welcome;
        if( !m_sock.Read( &welcome, sizeof( welcome ), 10, ShouldExit ) )
        {
            m_handshake.store( HandshakeDropped, std::memory_order_relaxed );
            goto close;
        }
        m_timerMul = welcome.timerMul;
        m_data.baseTime = welcome.initBegin;
        const auto initEnd = TscTime( welcome.initEnd - m_data.baseTime );
        m_data.framesBase->frames.push_back( FrameEvent{ 0, -1, -1 } );
        m_data.framesBase->frames.push_back( FrameEvent{ initEnd, -1, -1 } );
        m_data.lastTime = initEnd;
        m_delay = TscTime( welcome.delay );
        m_resolution = TscTime( welcome.resolution );
        m_pid = welcome.pid;
        m_onDemand = welcome.onDemand;
        m_captureProgram = welcome.programName;
        m_captureTime = welcome.epoch;
        m_ignoreMemFreeFaults = welcome.onDemand || welcome.isApple;

        char dtmp[64];
        time_t date = welcome.epoch;
        auto lt = localtime( &date );
        strftime( dtmp, 64, "%F %T", lt );
        char tmp[1024];
        sprintf( tmp, "%s @ %s", welcome.programName, dtmp );
        m_captureName = tmp;

        m_hostInfo = welcome.hostInfo;

        if( welcome.onDemand != 0 )
        {
            OnDemandPayloadMessage onDemand;
            if( !m_sock.Read( &onDemand, sizeof( onDemand ), 10, ShouldExit ) )
            {
                m_handshake.store( HandshakeDropped, std::memory_order_relaxed );
                goto close;
            }
            m_data.frameOffset = onDemand.frames;
            m_data.framesBase->frames.push_back( FrameEvent{ TscTime( onDemand.currentTime - m_data.baseTime ), -1, -1 } );
        }
    }

    m_serverQuerySpaceLeft = ( m_sock.GetSendBufSize() / ServerQueryPacketSize ) - ServerQueryPacketSize;   // leave space for terminate request
    m_hasData.store( true, std::memory_order_release );

    LZ4_setStreamDecode( m_stream, nullptr, 0 );
    m_connected.store( true, std::memory_order_relaxed );

    t0 = std::chrono::high_resolution_clock::now();

    for(;;)
    {
        if( m_shutdown.load( std::memory_order_relaxed ) )
        {
            QueryTerminate();
            return;
        }

        auto buf = m_buffer + m_bufferOffset;
        lz4sz_t lz4sz;
        if( !m_sock.Read( &lz4sz, sizeof( lz4sz ), 10, ShouldExit ) ) goto close;
        if( !m_sock.Read( lz4buf.get(), lz4sz, 10, ShouldExit ) ) goto close;
        bytes += sizeof( lz4sz ) + lz4sz;

        auto sz = LZ4_decompress_safe_continue( m_stream, lz4buf.get(), buf, lz4sz, TargetFrameSize );
        assert( sz >= 0 );
        decBytes += sz;

        char* ptr = buf;
        const char* end = buf + sz;

        {
            std::lock_guard<std::shared_mutex> lock( m_data.lock );
            while( ptr < end )
            {
                auto ev = (const QueueItem*)ptr;
                if( !DispatchProcess( *ev, ptr ) )
                {
                    QueryTerminate();
                    goto close;
                }
            }

            m_bufferOffset += sz;
            if( m_bufferOffset > TargetFrameSize * 2 ) m_bufferOffset = 0;

            HandlePostponedPlots();

            while( !m_serverQueryQueue.empty() && m_serverQuerySpaceLeft > 0 )
            {
                m_serverQuerySpaceLeft--;
                const auto& query = m_serverQueryQueue.back();
                m_sock.Send( &query, ServerQueryPacketSize );
                m_serverQueryQueue.pop_back();
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        auto td = std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 ).count();
        enum { MbpsUpdateTime = 200 };
        if( td > MbpsUpdateTime )
        {
            std::lock_guard<std::shared_mutex> lock( m_mbpsData.lock );
            m_mbpsData.mbps.erase( m_mbpsData.mbps.begin() );
            m_mbpsData.mbps.emplace_back( bytes / ( td * 125.f ) );
            m_mbpsData.compRatio = float( bytes ) / decBytes;
            m_mbpsData.queue = m_serverQueryQueue.size();
            t0 = t1;
            bytes = 0;
            decBytes = 0;
        }

        if( m_terminate )
        {
            if( m_pendingStrings != 0 || m_pendingThreads != 0 || m_pendingSourceLocation != 0 || m_pendingCallstackFrames != 0 ||
                !m_pendingCustomStrings.empty() || m_data.plots.IsPending() || m_pendingCallstackPtr != 0 ||
                m_pendingExternalNames != 0 || m_pendingCallstackSubframes != 0 || !m_pendingFrameImageData.empty() )
            {
                continue;
            }
            if( !m_crashed && !m_disconnect )
            {
                bool done = true;
                for( auto& v : m_data.threads )
                {
                    if( !v->stack.empty() )
                    {
                        done = false;
                        break;
                    }
                }
                if( !done ) continue;
            }
            Query( ServerQueryTerminate, 0 );
            break;
        }
    }

close:
    m_sock.Close();
    m_connected.store( false, std::memory_order_relaxed );
}

void Worker::Query( ServerQuery type, uint64_t data )
{
    ServerQueryPacket query { type, data };
    if( m_serverQuerySpaceLeft > 0 )
    {
        m_serverQuerySpaceLeft--;
        m_sock.Send( &query, ServerQueryPacketSize );
    }
    else
    {
        m_serverQueryQueue.insert( m_serverQueryQueue.begin(), query );
    }
}

void Worker::QueryTerminate()
{
    ServerQueryPacket query { ServerQueryTerminate, 0 };
    m_sock.Send( &query, ServerQueryPacketSize );
}

bool Worker::DispatchProcess( const QueueItem& ev, char*& ptr )
{
    if( ev.hdr.idx >= (int)QueueType::StringData )
    {
        ptr += sizeof( QueueHeader ) + sizeof( QueueStringTransfer );
        if( ev.hdr.type == QueueType::FrameImageData )
        {
            uint32_t sz;
            memcpy( &sz, ptr, sizeof( sz ) );
            ptr += sizeof( sz );
            AddFrameImageData( ev.stringTransfer.ptr, ptr, sz );
            ptr += sz;
        }
        else
        {
            uint16_t sz;
            memcpy( &sz, ptr, sizeof( sz ) );
            ptr += sizeof( sz );
            switch( ev.hdr.type )
            {
            case QueueType::CustomStringData:
                AddCustomString( ev.stringTransfer.ptr, ptr, sz );
                break;
            case QueueType::StringData:
                AddString( ev.stringTransfer.ptr, ptr, sz );
                m_serverQuerySpaceLeft++;
                break;
            case QueueType::ThreadName:
                AddThreadString( ev.stringTransfer.ptr, ptr, sz );
                m_serverQuerySpaceLeft++;
                break;
            case QueueType::PlotName:
                HandlePlotName( ev.stringTransfer.ptr, ptr, sz );
                m_serverQuerySpaceLeft++;
                break;
            case QueueType::SourceLocationPayload:
                AddSourceLocationPayload( ev.stringTransfer.ptr, ptr, sz );
                break;
            case QueueType::CallstackPayload:
                AddCallstackPayload( ev.stringTransfer.ptr, ptr, sz );
                break;
            case QueueType::FrameName:
                HandleFrameName( ev.stringTransfer.ptr, ptr, sz );
                m_serverQuerySpaceLeft++;
                break;
            case QueueType::CallstackAllocPayload:
                AddCallstackAllocPayload( ev.stringTransfer.ptr, ptr, sz );
                break;
            case QueueType::ExternalName:
                AddExternalName( ev.stringTransfer.ptr, ptr, sz );
                break;
            case QueueType::ExternalThreadName:
                AddExternalThreadName( ev.stringTransfer.ptr, ptr, sz );
                break;
            default:
                assert( false );
                break;
            }
            ptr += sz;
        }
        return true;
    }
    else
    {
        ptr += QueueDataSize[ev.hdr.idx];
        return Process( ev );
    }
}

void Worker::CheckSourceLocation( uint64_t ptr )
{
    if( m_data.sourceLocation.find( ptr ) == m_data.sourceLocation.end() )
    {
        NewSourceLocation( ptr );
    }
}

void Worker::NewSourceLocation( uint64_t ptr )
{
    static const SourceLocation emptySourceLocation = {};

    m_data.sourceLocation.emplace( ptr, emptySourceLocation );
    m_pendingSourceLocation++;
    m_sourceLocationQueue.push_back( ptr );

    Query( ServerQuerySourceLocation, ptr );
}

int16_t Worker::ShrinkSourceLocation( uint64_t srcloc )
{
    auto it = m_sourceLocationShrink.find( srcloc );
    if( it != m_sourceLocationShrink.end() )
    {
        return it->second;
    }
    else
    {
        return NewShrinkedSourceLocation( srcloc );
    }
}

int16_t Worker::NewShrinkedSourceLocation( uint64_t srcloc )
{
    assert( m_data.sourceLocationExpand.size() < std::numeric_limits<int16_t>::max() );
    const auto sz = int16_t( m_data.sourceLocationExpand.size() );
    m_data.sourceLocationExpand.push_back( srcloc );
#ifndef TRACY_NO_STATISTICS
    m_data.sourceLocationZones.emplace( sz, SourceLocationZones() );
#else
    m_data.sourceLocationZonesCnt.emplace( sz, 0 );
#endif
    m_sourceLocationShrink.emplace( srcloc, sz );
    return sz;
}

void Worker::InsertMessageData( MessageData* msg, uint64_t thread )
{
    if( m_data.messages.empty() )
    {
        m_data.messages.push_back( msg );
    }
    else if( m_data.messages.back()->time < msg->time )
    {
        m_data.messages.push_back_non_empty( msg );
    }
    else
    {
        auto mit = std::lower_bound( m_data.messages.begin(), m_data.messages.end(), msg->time, [] ( const auto& lhs, const auto& rhs ) { return lhs->time < rhs; } );
        m_data.messages.insert( mit, msg );
    }

    auto vec = &NoticeThread( thread )->messages;
    if( vec->empty() )
    {
        vec->push_back( msg );
    }
    else if( vec->back()->time < msg->time )
    {
        vec->push_back_non_empty( msg );
    }
    else
    {
        auto tmit = std::lower_bound( vec->begin(), vec->end(), msg->time, [] ( const auto& lhs, const auto& rhs ) { return lhs->time < rhs; } );
        vec->insert( tmit, msg );
    }
}

ThreadData* Worker::NoticeThreadReal( uint64_t thread )
{
    auto it = m_threadMap.find( thread );
    if( it != m_threadMap.end() )
    {
        m_data.threadDataLast.first = thread;
        m_data.threadDataLast.second = it->second;
        return it->second;
    }
    else
    {
        return NewThread( thread );
    }
}

ThreadData* Worker::NewThread( uint64_t thread )
{
    CheckThreadString( thread );
    auto td = m_slab.AllocInit<ThreadData>();
    td->id = thread;
    td->count = 0;
    td->nextZoneId = 0;
    m_data.threads.push_back( td );
    m_threadMap.emplace( thread, td );
    m_data.threadDataLast.first = thread;
    m_data.threadDataLast.second = td;
    return td;
}

void Worker::NewZone( ZoneEvent* zone, uint64_t thread )
{
    m_data.zonesCnt++;

#ifndef TRACY_NO_STATISTICS
    auto it = m_data.sourceLocationZones.find( zone->SrcLoc() );
    assert( it != m_data.sourceLocationZones.end() );
    it->second.zones.push_back( ZoneThreadData { zone, CompressThread( thread ) } );
#else
    auto it = m_data.sourceLocationZonesCnt.find( zone->SrcLoc() );
    assert( it != m_data.sourceLocationZonesCnt.end() );
    it->second++;
#endif

    auto td = NoticeThread( thread );
    td->count++;
    if( td->stack.empty() )
    {
        td->stack.push_back( zone );
        td->timeline.push_back( zone );
    }
    else
    {
        auto back = td->stack.back();
        if( back->child < 0 )
        {
            back->child = int32_t( m_data.zoneChildren.size() );
            if( m_data.zoneVectorCache.empty() )
            {
                m_data.zoneChildren.push_back( Vector<ZoneEvent*>( zone ) );
            }
            else
            {
                Vector<ZoneEvent*> vze = std::move( m_data.zoneVectorCache.back_and_pop() );
                assert( !vze.empty() );
                vze.clear();
                vze.push_back_non_empty( zone );
                m_data.zoneChildren.push_back( std::move( vze ) );
            }
        }
        else
        {
            m_data.zoneChildren[back->child].push_back( zone );
        }
        td->stack.push_back_non_empty( zone );
    }

    td->zoneIdStack.push_back( td->nextZoneId );
    td->nextZoneId = 0;
}

void Worker::InsertLockEvent( LockMap& lockmap, LockEvent* lev, uint64_t thread )
{
    const auto lt = lev->Time();
    m_data.lastTime = std::max( m_data.lastTime, lt );

    NoticeThread( thread );

    auto it = lockmap.threadMap.find( thread );
    if( it == lockmap.threadMap.end() )
    {
        assert( lockmap.threadList.size() < MaxLockThreads );
        it = lockmap.threadMap.emplace( thread, lockmap.threadList.size() ).first;
        lockmap.threadList.emplace_back( thread );
    }
    lev->thread = it->second;
    assert( lev->thread == it->second );
    auto& timeline = lockmap.timeline;
    if( timeline.empty() )
    {
        timeline.push_back( { lev } );
        UpdateLockCount( lockmap, timeline.size() - 1 );
    }
    else
    {
        assert( timeline.back().ptr->Time() <= lt );
        timeline.push_back_non_empty( { lev } );
        UpdateLockCount( lockmap, timeline.size() - 1 );
    }

    auto& range = lockmap.range[it->second];
    if( range.start > lt ) range.start = lt;
    if( range.end < lt ) range.end = lt;
}

void Worker::CheckString( uint64_t ptr )
{
    if( ptr == 0 ) return;
    if( m_data.strings.find( ptr ) != m_data.strings.end() ) return;

    m_data.strings.emplace( ptr, "???" );
    m_pendingStrings++;

    Query( ServerQueryString, ptr );
}

void Worker::CheckThreadString( uint64_t id )
{
    if( m_data.threadNames.find( id ) != m_data.threadNames.end() ) return;

    m_data.threadNames.emplace( id, "???" );
    m_pendingThreads++;

    Query( ServerQueryThreadString, id );
}

void Worker::CheckExternalName( uint64_t id )
{
    if( m_data.externalNames.find( id ) != m_data.externalNames.end() ) return;

    m_data.externalNames.emplace( id, std::make_pair( "???", "???" ) );
    m_pendingExternalNames += 2;

    Query( ServerQueryExternalName, id );
}

void Worker::AddSourceLocation( const QueueSourceLocation& srcloc )
{
    assert( m_pendingSourceLocation > 0 );
    m_pendingSourceLocation--;

    const auto ptr = m_sourceLocationQueue.front();
    m_sourceLocationQueue.erase( m_sourceLocationQueue.begin() );

    auto it = m_data.sourceLocation.find( ptr );
    assert( it != m_data.sourceLocation.end() );
    CheckString( srcloc.name );
    CheckString( srcloc.file );
    CheckString( srcloc.function );
    const uint32_t color = ( srcloc.r << 16 ) | ( srcloc.g << 8 ) | srcloc.b;
    it->second = SourceLocation { srcloc.name == 0 ? StringRef() : StringRef( StringRef::Ptr, srcloc.name ), StringRef( StringRef::Ptr, srcloc.function ), StringRef( StringRef::Ptr, srcloc.file ), srcloc.line, color };
}

void Worker::AddSourceLocationPayload( uint64_t ptr, char* data, size_t sz )
{
    const auto start = data;

    assert( m_pendingSourceLocationPayload.find( ptr ) == m_pendingSourceLocationPayload.end() );

    uint32_t color, line;
    memcpy( &color, data, 4 );
    memcpy( &line, data + 4, 4 );
    data += 8;
    auto end = data;

    while( *end ) end++;
    const auto func = StoreString( data, end - data );
    end++;

    data = end;
    while( *end ) end++;
    const auto source = StoreString( data, end - data );
    end++;

    const auto nsz = sz - ( end - start );

    color = ( ( color & 0x00FF0000 ) >> 16 ) |
            ( ( color & 0x0000FF00 )       ) |
            ( ( color & 0x000000FF ) << 16 );

    SourceLocation srcloc { nsz == 0 ? StringRef() : StringRef( StringRef::Idx, StoreString( end, nsz ).idx ), StringRef( StringRef::Idx, func.idx ), StringRef( StringRef::Idx, source.idx ), line, color };
    auto it = m_data.sourceLocationPayloadMap.find( &srcloc );
    if( it == m_data.sourceLocationPayloadMap.end() )
    {
        auto slptr = m_slab.Alloc<SourceLocation>();
        memcpy( slptr, &srcloc, sizeof( srcloc ) );
        uint32_t idx = m_data.sourceLocationPayload.size();
        m_data.sourceLocationPayloadMap.emplace( slptr, idx );
        m_pendingSourceLocationPayload.emplace( ptr, -int16_t( idx + 1 ) );
        m_data.sourceLocationPayload.push_back( slptr );
#ifndef TRACY_NO_STATISTICS
        m_data.sourceLocationZones.emplace( -int16_t( idx + 1 ), SourceLocationZones() );
#else
        m_data.sourceLocationZonesCnt.emplace( -int16_t( idx + 1 ), 0 );
#endif
    }
    else
    {
        m_pendingSourceLocationPayload.emplace( ptr, -int16_t( it->second + 1 ) );
    }
}

void Worker::AddString( uint64_t ptr, char* str, size_t sz )
{
    assert( m_pendingStrings > 0 );
    m_pendingStrings--;
    auto it = m_data.strings.find( ptr );
    assert( it != m_data.strings.end() && strcmp( it->second, "???" ) == 0 );
    const auto sl = StoreString( str, sz );
    it->second = sl.ptr;
}

void Worker::AddThreadString( uint64_t id, char* str, size_t sz )
{
    assert( m_pendingThreads > 0 );
    m_pendingThreads--;
    auto it = m_data.threadNames.find( id );
    assert( it != m_data.threadNames.end() && strcmp( it->second, "???" ) == 0 );
    const auto sl = StoreString( str, sz );
    it->second = sl.ptr;
}

void Worker::AddCustomString( uint64_t ptr, char* str, size_t sz )
{
    assert( m_pendingCustomStrings.find( ptr ) == m_pendingCustomStrings.end() );
    m_pendingCustomStrings.emplace( ptr, StoreString( str, sz ) );
}

void Worker::AddExternalName( uint64_t ptr, char* str, size_t sz )
{
    assert( m_pendingExternalNames > 0 );
    m_pendingExternalNames--;
    auto it = m_data.externalNames.find( ptr );
    assert( it != m_data.externalNames.end() && strcmp( it->second.first, "???" ) == 0 );
    const auto sl = StoreString( str, sz );
    it->second.first = sl.ptr;
}

void Worker::AddExternalThreadName( uint64_t ptr, char* str, size_t sz )
{
    assert( m_pendingExternalNames > 0 );
    m_pendingExternalNames--;
    auto it = m_data.externalNames.find( ptr );
    assert( it != m_data.externalNames.end() && strcmp( it->second.second, "???" ) == 0 );
    const auto sl = StoreString( str, sz );
    it->second.second = sl.ptr;
}

static const uint8_t DxtcIndexTable[256] = {
    85,     87,     86,     84,     93,     95,     94,     92,     89,     91,     90,     88,     81,     83,     82,     80,
    117,    119,    118,    116,    125,    127,    126,    124,    121,    123,    122,    120,    113,    115,    114,    112,
    101,    103,    102,    100,    109,    111,    110,    108,    105,    107,    106,    104,    97,     99,     98,     96,
    69,     71,     70,     68,     77,     79,     78,     76,     73,     75,     74,     72,     65,     67,     66,     64,
    213,    215,    214,    212,    221,    223,    222,    220,    217,    219,    218,    216,    209,    211,    210,    208,
    245,    247,    246,    244,    253,    255,    254,    252,    249,    251,    250,    248,    241,    243,    242,    240,
    229,    231,    230,    228,    237,    239,    238,    236,    233,    235,    234,    232,    225,    227,    226,    224,
    197,    199,    198,    196,    205,    207,    206,    204,    201,    203,    202,    200,    193,    195,    194,    192,
    149,    151,    150,    148,    157,    159,    158,    156,    153,    155,    154,    152,    145,    147,    146,    144,
    181,    183,    182,    180,    189,    191,    190,    188,    185,    187,    186,    184,    177,    179,    178,    176,
    165,    167,    166,    164,    173,    175,    174,    172,    169,    171,    170,    168,    161,    163,    162,    160,
    133,    135,    134,    132,    141,    143,    142,    140,    137,    139,    138,    136,    129,    131,    130,    128,
    21,     23,     22,     20,     29,     31,     30,     28,     25,     27,     26,     24,     17,     19,     18,     16,
    53,     55,     54,     52,     61,     63,     62,     60,     57,     59,     58,     56,     49,     51,     50,     48,
    37,     39,     38,     36,     45,     47,     46,     44,     41,     43,     42,     40,     33,     35,     34,     32,
    5,      7,      6,      4,      13,     15,     14,     12,     9,      11,     10,     8,      1,      3,      2,      0
};

void Worker::AddFrameImageData( uint64_t ptr, char* data, size_t sz )
{
    assert( m_pendingFrameImageData.find( ptr ) == m_pendingFrameImageData.end() );
    auto image = m_slab.AllocBig( sz );
    auto src = (uint8_t*)data;
    auto dst = (uint8_t*)image;
    assert( sz % 8 == 0 );
    for( size_t i=0; i<sz; i+=8 )
    {
        memcpy( dst, src, 4 );
        dst += 4;
        src += 4;
        for( int j=0; j<4; j++ ) *dst++ = DxtcIndexTable[*src++];
    }
    m_pendingFrameImageData.emplace( ptr, image );
}

uint64_t Worker::GetCanonicalPointer( const CallstackFrameId& id ) const
{
    assert( id.sel == 0 );
    return ( id.idx & 0x7FFFFFFFFFFFFFFF ) | ( ( id.idx & 0x4000000000000000 ) << 1 );
}

void Worker::AddCallstackPayload( uint64_t ptr, char* _data, size_t _sz )
{
    assert( m_pendingCallstackPtr == 0 );

    const auto sz = _sz / sizeof( uint64_t );
    const auto memsize = sizeof( VarArray<CallstackFrameId> ) + sz * sizeof( CallstackFrameId );
    auto mem = (char*)m_slab.AllocRaw( memsize );

    auto data = (CallstackFrameId*)mem;
    auto dst = data;
    auto src = (uint64_t*)_data;
    for( size_t i=0; i<sz; i++ )
    {
        *dst++ = PackPointer( *src++ );
    }

    auto arr = (VarArray<CallstackFrameId>*)( mem + sz * sizeof( CallstackFrameId ) );
    new(arr) VarArray<CallstackFrameId>( sz, data );

    uint32_t idx;
    auto it = m_data.callstackMap.find( arr );
    if( it == m_data.callstackMap.end() )
    {
        idx = m_data.callstackPayload.size();
        m_data.callstackMap.emplace( arr, idx );
        m_data.callstackPayload.push_back( arr );

        for( auto& frame : *arr )
        {
            auto fit = m_data.callstackFrameMap.find( frame );
            if( fit == m_data.callstackFrameMap.end() )
            {
                m_pendingCallstackFrames++;
                Query( ServerQueryCallstackFrame, GetCanonicalPointer( frame ) );
            }
        }
    }
    else
    {
        idx = it->second;
        m_slab.Unalloc( memsize );
    }

    m_pendingCallstackPtr = ptr;
    m_pendingCallstackId = idx;
}

void Worker::AddCallstackAllocPayload( uint64_t ptr, char* data, size_t _sz )
{
    assert( m_pendingCallstackPtr != 0 );

    CallstackFrameId stack[64];
    const auto sz = *(uint32_t*)data; data += 4;
    assert( sz <= 64 );
    for( uint32_t i=0; i<sz; i++ )
    {
        uint32_t sz;
        CallstackFrame cf;
        memcpy( &cf.line, data, 4 ); data += 4;
        memcpy( &sz, data, 4 ); data += 4;
        cf.name = StoreString( data, sz ).idx; data += sz;
        memcpy( &sz, data, 4 ); data += 4;
        cf.file = StoreString( data, sz ).idx; data += sz;
        CallstackFrameData cfd = { &cf, 1 };

        CallstackFrameId id;
        auto it = m_data.revFrameMap.find( &cfd );
        if( it == m_data.revFrameMap.end() )
        {
            auto frame = m_slab.Alloc<CallstackFrame>();
            memcpy( frame, &cf, sizeof( CallstackFrame ) );
            auto frameData = m_slab.Alloc<CallstackFrameData>();
            frameData->data = frame;
            frameData->size = 1;
            id.idx = m_callstackAllocNextIdx++;
            id.sel = 1;
            m_data.callstackFrameMap.emplace( id, frameData );
            m_data.revFrameMap.emplace( frameData, id );
        }
        else
        {
            id = it->second;
        }
        stack[i] = id;
    }

    const auto nativeCs = m_data.callstackPayload[m_pendingCallstackId];
    const auto nsz = nativeCs->size();
    const auto tsz = sz + nsz;

    const auto memsize = sizeof( VarArray<CallstackFrameId> ) + tsz * sizeof( CallstackFrameId );
    auto mem = (char*)m_slab.AllocRaw( memsize );
    memcpy( mem, stack, sizeof( CallstackFrameId ) * sz );
    memcpy( mem + sizeof( CallstackFrameId ) * sz, nativeCs->data(), sizeof( CallstackFrameId ) * nsz );

    auto arr = (VarArray<CallstackFrameId>*)( mem + tsz * sizeof( CallstackFrameId ) );
    new(arr) VarArray<CallstackFrameId>( tsz, (CallstackFrameId*)mem );

    uint32_t idx;
    auto it = m_data.callstackMap.find( arr );
    if( it == m_data.callstackMap.end() )
    {
        idx = m_data.callstackPayload.size();
        m_data.callstackMap.emplace( arr, idx );
        m_data.callstackPayload.push_back( arr );

        for( auto& frame : *arr )
        {
            auto fit = m_data.callstackFrameMap.find( frame );
            if( fit == m_data.callstackFrameMap.end() )
            {
                m_pendingCallstackFrames++;
                Query( ServerQueryCallstackFrame, GetCanonicalPointer( frame ) );
            }
        }
    }
    else
    {
        idx = it->second;
        m_slab.Unalloc( memsize );
    }

    m_pendingCallstackPtr = ptr;
    m_pendingCallstackId = idx;
}

void Worker::InsertPlot( PlotData* plot, int64_t time, double val )
{
    if( plot->data.empty() )
    {
        plot->min = val;
        plot->max = val;
        plot->data.push_back( { time, val } );
    }
    else if( plot->data.back().time < time )
    {
        if( plot->min > val ) plot->min = val;
        else if( plot->max < val ) plot->max = val;
        plot->data.push_back_non_empty( { time, val } );
    }
    else
    {
        if( plot->min > val ) plot->min = val;
        else if( plot->max < val ) plot->max = val;
        if( plot->postpone.empty() )
        {
            plot->postponeTime = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() ).count();
            plot->postpone.push_back( { time, val } );
        }
        else
        {
            plot->postpone.push_back_non_empty( { time, val } );
        }
    }
}

void Worker::HandlePlotName( uint64_t name, char* str, size_t sz )
{
    const auto sl = StoreString( str, sz );
    m_data.plots.StringDiscovered( name, sl, m_data.strings, [this] ( PlotData* dst, PlotData* src ) {
        for( auto& v : src->data )
        {
            InsertPlot( dst, v.time, v.val );
        }
    } );
}

void Worker::HandleFrameName( uint64_t name, char* str, size_t sz )
{
    const auto sl = StoreString( str, sz );
    m_data.frames.StringDiscovered( name, sl, m_data.strings, [] ( FrameData* dst, FrameData* src ) {
        auto sz = dst->frames.size();
        dst->frames.insert( dst->frames.end(), src->frames.begin(), src->frames.end() );
        std::inplace_merge( dst->frames.begin(), dst->frames.begin() + sz, dst->frames.end(), [] ( const auto& lhs, const auto& rhs ) { return lhs.start < rhs.start; } );
    } );
}

void Worker::HandlePostponedPlots()
{
    for( auto& plot : m_data.plots.Data() )
    {
        auto& src = plot->postpone;
        if( src.empty() ) continue;
        if( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() ).count() - plot->postponeTime < 100 ) continue;
        auto& dst = plot->data;
#ifdef MY_LIBCPP_SUCKS
        pdqsort_branchless( src.begin(), src.end(), [] ( const auto& l, const auto& r ) { return l.time < r.time; } );
#else
        std::sort( std::execution::par_unseq, src.begin(), src.end(), [] ( const auto& l, const auto& r ) { return l.time < r.time; } );
#endif
        const auto ds = std::lower_bound( dst.begin(), dst.end(), src.front().time, [] ( const auto& l, const auto& r ) { return l.time < r; } );
        const auto dsd = std::distance( dst.begin(), ds ) ;
        const auto de = std::lower_bound( ds, dst.end(), src.back().time, [] ( const auto& l, const auto& r ) { return l.time < r; } );
        const auto ded = std::distance( dst.begin(), de );
        dst.insert( de, src.begin(), src.end() );
        std::inplace_merge( dst.begin() + dsd, dst.begin() + ded, dst.begin() + ded + src.size(), [] ( const auto& l, const auto& r ) { return l.time < r.time; } );
        src.clear();
    }
}

StringLocation Worker::StoreString( char* str, size_t sz )
{
    StringLocation ret;
    const char backup = str[sz];
    str[sz] = '\0';
    charutil::StringKey key = { str, sz };
    auto sit = m_data.stringMap.find( key );
    if( sit == m_data.stringMap.end() )
    {
        auto ptr = m_slab.Alloc<char>( sz+1 );
        memcpy( ptr, str, sz );
        ptr[sz] = '\0';
        ret.ptr = ptr;
        ret.idx = m_data.stringData.size();
        m_data.stringMap.emplace( charutil::StringKey { ptr, sz }, m_data.stringData.size() );
        m_data.stringData.push_back( ptr );
    }
    else
    {
        ret.ptr = sit->first.ptr;
        ret.idx = sit->second;
    }
    str[sz] = backup;
    return ret;
}

bool Worker::Process( const QueueItem& ev )
{
    switch( ev.hdr.type )
    {
    case QueueType::ThreadContext:
        ProcessThreadContext( ev.threadCtx );
        break;
    case QueueType::ZoneBegin:
        ProcessZoneBegin( ev.zoneBegin );
        break;
    case QueueType::ZoneBeginCallstack:
        ProcessZoneBeginCallstack( ev.zoneBegin );
        break;
    case QueueType::ZoneBeginAllocSrcLoc:
        ProcessZoneBeginAllocSrcLoc( ev.zoneBegin );
        break;
    case QueueType::ZoneBeginAllocSrcLocCallstack:
        ProcessZoneBeginAllocSrcLocCallstack( ev.zoneBegin );
        break;
    case QueueType::ZoneEnd:
        ProcessZoneEnd( ev.zoneEnd );
        break;
    case QueueType::ZoneValidation:
        ProcessZoneValidation( ev.zoneValidation );
        break;
    case QueueType::FrameMarkMsg:
        ProcessFrameMark( ev.frameMark );
        break;
    case QueueType::FrameMarkMsgStart:
        ProcessFrameMarkStart( ev.frameMark );
        break;
    case QueueType::FrameMarkMsgEnd:
        ProcessFrameMarkEnd( ev.frameMark );
        break;
    case QueueType::FrameImage:
        ProcessFrameImage( ev.frameImage );
        break;
    case QueueType::SourceLocation:
        AddSourceLocation( ev.srcloc );
        m_serverQuerySpaceLeft++;
        break;
    case QueueType::ZoneText:
        ProcessZoneText( ev.zoneText );
        break;
    case QueueType::ZoneName:
        ProcessZoneName( ev.zoneText );
        break;
    case QueueType::LockAnnounce:
        ProcessLockAnnounce( ev.lockAnnounce );
        break;
    case QueueType::LockTerminate:
        ProcessLockTerminate( ev.lockTerminate );
        break;
    case QueueType::LockWait:
        ProcessLockWait( ev.lockWait );
        break;
    case QueueType::LockObtain:
        ProcessLockObtain( ev.lockObtain );
        break;
    case QueueType::LockRelease:
        ProcessLockRelease( ev.lockRelease );
        break;
    case QueueType::LockSharedWait:
        ProcessLockSharedWait( ev.lockWait );
        break;
    case QueueType::LockSharedObtain:
        ProcessLockSharedObtain( ev.lockObtain );
        break;
    case QueueType::LockSharedRelease:
        ProcessLockSharedRelease( ev.lockRelease );
        break;
    case QueueType::LockMark:
        ProcessLockMark( ev.lockMark );
        break;
    case QueueType::PlotData:
        ProcessPlotData( ev.plotData );
        break;
    case QueueType::Message:
        ProcessMessage( ev.message );
        break;
    case QueueType::MessageLiteral:
        ProcessMessageLiteral( ev.message );
        break;
    case QueueType::MessageColor:
        ProcessMessageColor( ev.messageColor );
        break;
    case QueueType::MessageLiteralColor:
        ProcessMessageLiteralColor( ev.messageColor );
        break;
    case QueueType::MessageAppInfo:
        ProcessMessageAppInfo( ev.message );
        break;
    case QueueType::GpuNewContext:
        ProcessGpuNewContext( ev.gpuNewContext );
        break;
    case QueueType::GpuZoneBegin:
        ProcessGpuZoneBegin( ev.gpuZoneBegin );
        break;
    case QueueType::GpuZoneBeginCallstack:
        ProcessGpuZoneBeginCallstack( ev.gpuZoneBegin );
        break;
    case QueueType::GpuZoneEnd:
        ProcessGpuZoneEnd( ev.gpuZoneEnd );
        break;
    case QueueType::GpuTime:
        ProcessGpuTime( ev.gpuTime );
        break;
    case QueueType::MemAlloc:
        ProcessMemAlloc( ev.memAlloc );
        break;
    case QueueType::MemFree:
        ProcessMemFree( ev.memFree );
        break;
    case QueueType::MemAllocCallstack:
        ProcessMemAllocCallstack( ev.memAlloc );
        break;
    case QueueType::MemFreeCallstack:
        ProcessMemFreeCallstack( ev.memFree );
        break;
    case QueueType::CallstackMemory:
        ProcessCallstackMemory( ev.callstackMemory );
        break;
    case QueueType::Callstack:
        ProcessCallstack( ev.callstack );
        break;
    case QueueType::CallstackAlloc:
        ProcessCallstackAlloc( ev.callstackAlloc );
        break;
    case QueueType::CallstackFrameSize:
        ProcessCallstackFrameSize( ev.callstackFrameSize );
        m_serverQuerySpaceLeft++;
        break;
    case QueueType::CallstackFrame:
        ProcessCallstackFrame( ev.callstackFrame );
        break;
    case QueueType::Terminate:
        m_terminate = true;
        break;
    case QueueType::KeepAlive:
        break;
    case QueueType::Crash:
        m_crashed = true;
        break;
    case QueueType::CrashReport:
        ProcessCrashReport( ev.crashReport );
        break;
    case QueueType::SysTimeReport:
        ProcessSysTime( ev.sysTime );
        break;
    case QueueType::ContextSwitch:
        ProcessContextSwitch( ev.contextSwitch );
        break;
    case QueueType::ThreadWakeup:
        ProcessThreadWakeup( ev.threadWakeup );
        break;
    case QueueType::TidToPid:
        ProcessTidToPid( ev.tidToPid );
        break;
    default:
        assert( false );
        break;
    }

    return m_failure == Failure::None;
}

void Worker::ProcessThreadContext( const QueueThreadContext& ev )
{
    m_threadCtx = ev.thread;
}

void Worker::ProcessZoneBeginImpl( ZoneEvent* zone, const QueueZoneBegin& ev )
{
    CheckSourceLocation( ev.srcloc );

    const auto start = TscTime( ev.time - m_data.baseTime );
    zone->SetStart( start );
    zone->end = -1;
    zone->SetSrcLoc( ShrinkSourceLocation( ev.srcloc ) );
    zone->callstack = 0;
    zone->child = -1;

    m_data.lastTime = std::max( m_data.lastTime, start );

    NewZone( zone, m_threadCtx );
}

void Worker::ProcessZoneBegin( const QueueZoneBegin& ev )
{
    auto zone = m_slab.AllocInit<ZoneEvent>();
    ProcessZoneBeginImpl( zone, ev );
}

void Worker::ProcessZoneBeginCallstack( const QueueZoneBegin& ev )
{
    auto zone = m_slab.AllocInit<ZoneEvent>();
    ProcessZoneBeginImpl( zone, ev );

    auto& next = m_nextCallstack[m_threadCtx];
    next.type = NextCallstackType::Zone;
    next.zone = zone;
}

void Worker::ProcessZoneBeginAllocSrcLocImpl( ZoneEvent* zone, const QueueZoneBegin& ev )
{
    auto it = m_pendingSourceLocationPayload.find( ev.srcloc );
    assert( it != m_pendingSourceLocationPayload.end() );

    const auto start = TscTime( ev.time - m_data.baseTime );
    zone->SetStart( start );
    zone->end = -1;
    zone->SetSrcLoc( it->second );
    zone->callstack = 0;
    zone->child = -1;

    m_data.lastTime = std::max( m_data.lastTime, start );

    NewZone( zone, m_threadCtx );

    m_pendingSourceLocationPayload.erase( it );
}

void Worker::ProcessZoneBeginAllocSrcLoc( const QueueZoneBegin& ev )
{
    auto zone = m_slab.AllocInit<ZoneEvent>();
    ProcessZoneBeginAllocSrcLocImpl( zone, ev );
}

void Worker::ProcessZoneBeginAllocSrcLocCallstack( const QueueZoneBegin& ev )
{
    auto zone = m_slab.AllocInit<ZoneEvent>();
    ProcessZoneBeginAllocSrcLocImpl( zone, ev );

    auto& next = m_nextCallstack[m_threadCtx];
    next.type = NextCallstackType::Zone;
    next.zone = zone;
}

void Worker::ProcessZoneEnd( const QueueZoneEnd& ev )
{
    auto tit = m_threadMap.find( m_threadCtx );
    if( tit == m_threadMap.end() || tit->second->zoneIdStack.empty() )
    {
        ZoneEndFailure( m_threadCtx );
        return;
    }

    auto td = tit->second;
    auto zoneId = td->zoneIdStack.back_and_pop();
    if( zoneId != td->nextZoneId )
    {
        ZoneStackFailure( m_threadCtx, td->stack.back() );
        return;
    }
    td->nextZoneId = 0;

    auto& stack = td->stack;
    assert( !stack.empty() );
    auto zone = stack.back_and_pop();
    assert( zone->end == -1 );
    zone->end = TscTime( ev.time - m_data.baseTime );
    assert( zone->end >= zone->Start() );

    m_data.lastTime = std::max( m_data.lastTime, zone->end );

    if( zone->child >= 0 )
    {
        auto& childVec = m_data.zoneChildren[zone->child];
        const auto sz = childVec.size();
        if( sz <= 8 * 1024 )
        {
            Vector<ZoneEvent*> fitVec;
            fitVec.reserve_exact( sz, m_slab );
            memcpy( fitVec.data(), childVec.data(), sz * sizeof( ZoneEvent* ) );
            fitVec.swap( childVec );
            m_data.zoneVectorCache.push_back( std::move( fitVec ) );
        }
    }

#ifndef TRACY_NO_STATISTICS
    auto timeSpan = zone->end - zone->Start();
    if( timeSpan > 0 )
    {
        auto it = m_data.sourceLocationZones.find( zone->SrcLoc() );
        assert( it != m_data.sourceLocationZones.end() );
        auto& slz = it->second;
        slz.min = std::min( slz.min, timeSpan );
        slz.max = std::max( slz.max, timeSpan );
        slz.total += timeSpan;
        slz.sumSq += double( timeSpan ) * timeSpan;
        if( zone->child >= 0 )
        {
            for( auto& v : GetZoneChildren( zone->child ) )
            {
                const auto childSpan = std::max( int64_t( 0 ), v->end - v->Start() );
                timeSpan -= childSpan;
            }
        }
        slz.selfMin = std::min( slz.selfMin, timeSpan );
        slz.selfMax = std::max( slz.selfMax, timeSpan );
        slz.selfTotal += timeSpan;
    }
#endif
}

void Worker::ZoneStackFailure( uint64_t thread, const ZoneEvent* ev )
{
    m_failure = Failure::ZoneStack;
    m_failureData.thread = thread;
    m_failureData.srcloc = ev->SrcLoc();
}

void Worker::ZoneEndFailure( uint64_t thread )
{
    m_failure = Failure::ZoneEnd;
    m_failureData.thread = thread;
    m_failureData.srcloc = 0;
}

void Worker::ZoneTextFailure( uint64_t thread )
{
    m_failure = Failure::ZoneText;
    m_failureData.thread = thread;
    m_failureData.srcloc = 0;
}

void Worker::ZoneNameFailure( uint64_t thread )
{
    m_failure = Failure::ZoneName;
    m_failureData.thread = thread;
    m_failureData.srcloc = 0;
}

void Worker::MemFreeFailure( uint64_t thread )
{
    m_failure = Failure::MemFree;
    m_failureData.thread = thread;
    m_failureData.srcloc = 0;
}

void Worker::FrameEndFailure()
{
    m_failure = Failure::ZoneEnd;
    m_failureData.thread = 0;
    m_failureData.srcloc = 0;
}

void Worker::FrameImageIndexFailure()
{
    m_failure = Failure::FrameImageIndex;
    m_failureData.thread = 0;
    m_failureData.srcloc = 0;
}

void Worker::FrameImageTwiceFailure()
{
    m_failure = Failure::FrameImageTwice;
    m_failureData.thread = 0;
    m_failureData.srcloc = 0;
}

void Worker::ProcessZoneValidation( const QueueZoneValidation& ev )
{
    auto td = NoticeThread( m_threadCtx );
    td->nextZoneId = ev.id;
}

void Worker::ProcessFrameMark( const QueueFrameMark& ev )
{
    auto fd = m_data.frames.Retrieve( ev.name, [this] ( uint64_t name ) {
        auto fd = m_slab.AllocInit<FrameData>();
        fd->name = name;
        fd->continuous = 1;
        return fd;
    }, [this] ( uint64_t name ) {
        Query( ServerQueryFrameName, name );
    } );

    int32_t frameImage = -1;
    auto fis = m_frameImageStaging.find( fd->frames.size() );
    if( fis != m_frameImageStaging.end() )
    {
        frameImage = fis->second;
        m_frameImageStaging.erase( fis );
    }

    assert( fd->continuous == 1 );
    const auto time = TscTime( ev.time - m_data.baseTime );
    assert( fd->frames.empty() || fd->frames.back().start <= time );
    fd->frames.push_back( FrameEvent{ time, -1, frameImage } );
    m_data.lastTime = std::max( m_data.lastTime, time );
}

void Worker::ProcessFrameMarkStart( const QueueFrameMark& ev )
{
    auto fd = m_data.frames.Retrieve( ev.name, [this] ( uint64_t name ) {
        auto fd = m_slab.AllocInit<FrameData>();
        fd->name = name;
        fd->continuous = 0;
        return fd;
    }, [this] ( uint64_t name ) {
        Query( ServerQueryFrameName, name );
    } );

    assert( fd->continuous == 0 );
    const auto time = TscTime( ev.time - m_data.baseTime );
    assert( fd->frames.empty() || ( fd->frames.back().end <= time && fd->frames.back().end != -1 ) );
    fd->frames.push_back( FrameEvent{ time, -1, -1 } );
    m_data.lastTime = std::max( m_data.lastTime, time );
}

void Worker::ProcessFrameMarkEnd( const QueueFrameMark& ev )
{
    auto fd = m_data.frames.Retrieve( ev.name, [this] ( uint64_t name ) {
        auto fd = m_slab.AllocInit<FrameData>();
        fd->name = name;
        fd->continuous = 0;
        return fd;
    }, [this] ( uint64_t name ) {
        Query( ServerQueryFrameName, name );
    } );

    assert( fd->continuous == 0 );
    const auto time = TscTime( ev.time - m_data.baseTime );
    if( fd->frames.empty() )
    {
        FrameEndFailure();
        return;
    }
    assert( fd->frames.back().end == -1 );
    fd->frames.back().end = time;
    m_data.lastTime = std::max( m_data.lastTime, time );
}

void Worker::ProcessFrameImage( const QueueFrameImage& ev )
{
    auto it = m_pendingFrameImageData.find( ev.image );
    assert( it != m_pendingFrameImageData.end() );

    auto& frames = m_data.framesBase->frames;
    const auto fidx = int64_t( ev.frame ) - int64_t( m_data.frameOffset ) + 1;
    if( m_onDemand && fidx <= 1 )
    {
        m_pendingFrameImageData.erase( it );
        return;
    }
    else if( fidx <= 0 )
    {
        FrameImageIndexFailure();
        return;
    }

    auto fi = m_slab.Alloc<FrameImage>();
    fi->ptr = PackFrameImage( (const char*)it->second, ev.w, ev.h, fi->csz );
    fi->w = ev.w;
    fi->h = ev.h;
    fi->frameRef = uint32_t( fidx );
    fi->flip = ev.flip;

    const auto idx = m_data.frameImage.size();
    m_data.frameImage.push_back( fi );
    m_pendingFrameImageData.erase( it );

    if( fidx >= frames.size() )
    {
        if( m_frameImageStaging.find( fidx ) != m_frameImageStaging.end() )
        {
            FrameImageTwiceFailure();
            return;
        }
        m_frameImageStaging.emplace( fidx, idx );
    }
    else if( frames[fidx].frameImage >= 0 )
    {
        FrameImageTwiceFailure();
    }
    else
    {
        frames[fidx].frameImage = idx;
    }
}

void Worker::ProcessZoneText( const QueueZoneText& ev )
{
    auto tit = m_threadMap.find( m_threadCtx );
    if( tit == m_threadMap.end() || tit->second->stack.empty() || tit->second->nextZoneId != tit->second->zoneIdStack.back() )
    {
        ZoneTextFailure( m_threadCtx );
        return;
    }

    auto td = tit->second;
    td->nextZoneId = 0;
    auto& stack = td->stack;
    auto zone = stack.back();
    auto it = m_pendingCustomStrings.find( ev.text );
    assert( it != m_pendingCustomStrings.end() );
    zone->text = StringIdx( it->second.idx );
    m_pendingCustomStrings.erase( it );
}

void Worker::ProcessZoneName( const QueueZoneText& ev )
{
    auto tit = m_threadMap.find( m_threadCtx );
    if( tit == m_threadMap.end() || tit->second->stack.empty() || tit->second->nextZoneId != tit->second->zoneIdStack.back() )
    {
        ZoneNameFailure( m_threadCtx );
        return;
    }

    auto td = tit->second;
    td->nextZoneId = 0;
    auto& stack = td->stack;
    auto zone = stack.back();
    auto it = m_pendingCustomStrings.find( ev.text );
    assert( it != m_pendingCustomStrings.end() );
    zone->name = StringIdx( it->second.idx );
    m_pendingCustomStrings.erase( it );
}

void Worker::ProcessLockAnnounce( const QueueLockAnnounce& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    if( it == m_data.lockMap.end() )
    {
        auto lm = m_slab.AllocInit<LockMap>();
        lm->srcloc = ShrinkSourceLocation( ev.lckloc );
        lm->type = ev.type;
        lm->timeAnnounce = TscTime( ev.time - m_data.baseTime );
        lm->timeTerminate = 0;
        lm->valid = true;
        lm->isContended = false;
        m_data.lockMap.emplace( ev.id, lm );
    }
    else
    {
        it->second->srcloc = ShrinkSourceLocation( ev.lckloc );
        assert( it->second->type == ev.type );
        it->second->timeAnnounce = TscTime( ev.time - m_data.baseTime );
        it->second->valid = true;
    }
    CheckSourceLocation( ev.lckloc );
}

void Worker::ProcessLockTerminate( const QueueLockTerminate& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    if( it == m_data.lockMap.end() )
    {
        auto lm = m_slab.AllocInit<LockMap>();
        lm->type = ev.type;
        lm->timeAnnounce = 0;
        lm->timeTerminate = TscTime( ev.time - m_data.baseTime );
        lm->valid = false;
        lm->isContended = false;
        m_data.lockMap.emplace( ev.id, lm );
    }
    else
    {
        assert( it->second->type == ev.type );
        it->second->timeTerminate = TscTime( ev.time - m_data.baseTime );
    }
}

void Worker::ProcessLockWait( const QueueLockWait& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    if( it == m_data.lockMap.end() )
    {
        auto lm = m_slab.AllocInit<LockMap>();
        lm->timeAnnounce = 0;
        lm->timeTerminate = 0;
        lm->valid = false;
        lm->type = ev.type;
        lm->isContended = false;
        it = m_data.lockMap.emplace( ev.id, lm ).first;
    }

    auto lev = ev.type == LockType::Lockable ? m_slab.Alloc<LockEvent>() : m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::Wait;

    InsertLockEvent( *it->second, lev, ev.thread );
}

void Worker::ProcessLockObtain( const QueueLockObtain& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    assert( it != m_data.lockMap.end() );
    auto& lock = *it->second;

    auto lev = lock.type == LockType::Lockable ? m_slab.Alloc<LockEvent>() : m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::Obtain;

    InsertLockEvent( lock, lev, ev.thread );
}

void Worker::ProcessLockRelease( const QueueLockRelease& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    assert( it != m_data.lockMap.end() );
    auto& lock = *it->second;

    auto lev = lock.type == LockType::Lockable ? m_slab.Alloc<LockEvent>() : m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::Release;

    InsertLockEvent( lock, lev, ev.thread );
}

void Worker::ProcessLockSharedWait( const QueueLockWait& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    if( it == m_data.lockMap.end() )
    {
        auto lm = m_slab.AllocInit<LockMap>();
        lm->valid = false;
        lm->type = ev.type;
        lm->isContended = false;
        it = m_data.lockMap.emplace( ev.id, lm ).first;
    }

    assert( ev.type == LockType::SharedLockable );
    auto lev = m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::WaitShared;

    InsertLockEvent( *it->second, lev, ev.thread );
}

void Worker::ProcessLockSharedObtain( const QueueLockObtain& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    assert( it != m_data.lockMap.end() );
    auto& lock = *it->second;

    assert( lock.type == LockType::SharedLockable );
    auto lev = m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::ObtainShared;

    InsertLockEvent( lock, lev, ev.thread );
}

void Worker::ProcessLockSharedRelease( const QueueLockRelease& ev )
{
    auto it = m_data.lockMap.find( ev.id );
    assert( it != m_data.lockMap.end() );
    auto& lock = *it->second;

    assert( lock.type == LockType::SharedLockable );
    auto lev = m_slab.Alloc<LockEventShared>();
    lev->SetTime( TscTime( ev.time - m_data.baseTime ) );
    lev->SetSrcLoc( 0 );
    lev->type = LockEvent::Type::ReleaseShared;

    InsertLockEvent( lock, lev, ev.thread );
}

void Worker::ProcessLockMark( const QueueLockMark& ev )
{
    CheckSourceLocation( ev.srcloc );
    auto lit = m_data.lockMap.find( ev.id );
    assert( lit != m_data.lockMap.end() );
    auto& lockmap = *lit->second;
    auto tid = lockmap.threadMap.find( ev.thread );
    assert( tid != lockmap.threadMap.end() );
    const auto thread = tid->second;
    auto it = lockmap.timeline.end();
    for(;;)
    {
        --it;
        if( it->ptr->thread == thread )
        {
            switch( it->ptr->type )
            {
            case LockEvent::Type::Obtain:
            case LockEvent::Type::ObtainShared:
            case LockEvent::Type::Wait:
            case LockEvent::Type::WaitShared:
                it->ptr->SetSrcLoc( ShrinkSourceLocation( ev.srcloc ) );
                return;
            default:
                break;
            }
        }
    }
}

void Worker::ProcessPlotData( const QueuePlotData& ev )
{
    PlotData* plot = m_data.plots.Retrieve( ev.name, [this] ( uint64_t name ) {
        auto plot = m_slab.AllocInit<PlotData>();
        plot->name = name;
        plot->type = PlotType::User;
        return plot;
    }, [this]( uint64_t name ) {
        Query( ServerQueryPlotName, name );
    } );

    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );
    switch( ev.type )
    {
    case PlotDataType::Double:
        InsertPlot( plot, time, ev.data.d );
        break;
    case PlotDataType::Float:
        InsertPlot( plot, time, (double)ev.data.f );
        break;
    case PlotDataType::Int:
        InsertPlot( plot, time, (double)ev.data.i );
        break;
    default:
        assert( false );
        break;
    }
}

void Worker::ProcessMessage( const QueueMessage& ev )
{
    auto it = m_pendingCustomStrings.find( ev.text );
    assert( it != m_pendingCustomStrings.end() );
    auto msg = m_slab.Alloc<MessageData>();
    msg->time = TscTime( ev.time - m_data.baseTime );
    msg->ref = StringRef( StringRef::Type::Idx, it->second.idx );
    msg->thread = CompressThread( m_threadCtx );
    msg->color = 0xFFFFFFFF;
    m_data.lastTime = std::max( m_data.lastTime, msg->time );
    InsertMessageData( msg, m_threadCtx );
    m_pendingCustomStrings.erase( it );
}

void Worker::ProcessMessageLiteral( const QueueMessage& ev )
{
    CheckString( ev.text );
    auto msg = m_slab.Alloc<MessageData>();
    msg->time = TscTime( ev.time - m_data.baseTime );
    msg->ref = StringRef( StringRef::Type::Ptr, ev.text );
    msg->thread = CompressThread( m_threadCtx );
    msg->color = 0xFFFFFFFF;
    m_data.lastTime = std::max( m_data.lastTime, msg->time );
    InsertMessageData( msg, m_threadCtx );
}

void Worker::ProcessMessageColor( const QueueMessageColor& ev )
{
    auto it = m_pendingCustomStrings.find( ev.text );
    assert( it != m_pendingCustomStrings.end() );
    auto msg = m_slab.Alloc<MessageData>();
    msg->time = TscTime( ev.time - m_data.baseTime );
    msg->ref = StringRef( StringRef::Type::Idx, it->second.idx );
    msg->thread = CompressThread( m_threadCtx );
    msg->color = 0xFF000000 | ( ev.r << 16 ) | ( ev.g << 8 ) | ev.b;
    m_data.lastTime = std::max( m_data.lastTime, msg->time );
    InsertMessageData( msg, m_threadCtx );
    m_pendingCustomStrings.erase( it );
}

void Worker::ProcessMessageLiteralColor( const QueueMessageColor& ev )
{
    CheckString( ev.text );
    auto msg = m_slab.Alloc<MessageData>();
    msg->time = TscTime( ev.time - m_data.baseTime );
    msg->ref = StringRef( StringRef::Type::Ptr, ev.text );
    msg->thread = CompressThread( m_threadCtx );
    msg->color = 0xFF000000 | ( ev.r << 16 ) | ( ev.g << 8 ) | ev.b;
    m_data.lastTime = std::max( m_data.lastTime, msg->time );
    InsertMessageData( msg, m_threadCtx );
}

void Worker::ProcessMessageAppInfo( const QueueMessage& ev )
{
    auto it = m_pendingCustomStrings.find( ev.text );
    assert( it != m_pendingCustomStrings.end() );
    m_data.appInfo.push_back( StringRef( StringRef::Type::Idx, it->second.idx ) );
    m_data.lastTime = std::max( m_data.lastTime, TscTime( ev.time - m_data.baseTime ) );
    m_pendingCustomStrings.erase( it );
}

void Worker::ProcessGpuNewContext( const QueueGpuNewContext& ev )
{
    assert( !m_gpuCtxMap[ev.context] );

    int64_t gpuTime;
    if( ev.period == 1.f )
    {
        gpuTime = ev.gpuTime;
    }
    else
    {
        gpuTime = int64_t( double( ev.period ) * ev.gpuTime );      // precision loss
    }

    auto gpu = m_slab.AllocInit<GpuCtxData>();
    memset( gpu->query, 0, sizeof( gpu->query ) );
    gpu->timeDiff = TscTime( ev.cpuTime - m_data.baseTime ) - gpuTime;
    gpu->thread = ev.thread;
    gpu->accuracyBits = ev.accuracyBits;
    gpu->period = ev.period;
    gpu->count = 0;
    m_data.gpuData.push_back( gpu );
    m_gpuCtxMap[ev.context] = gpu;
}

void Worker::ProcessGpuZoneBeginImpl( GpuEvent* zone, const QueueGpuZoneBegin& ev )
{
    auto ctx = m_gpuCtxMap[ev.context];
    assert( ctx );

    CheckSourceLocation( ev.srcloc );

    zone->cpuStart = TscTime( ev.cpuTime - m_data.baseTime );
    zone->cpuEnd = -1;
    zone->gpuStart = std::numeric_limits<int64_t>::max();
    zone->gpuEnd = -1;
    zone->srcloc = ShrinkSourceLocation( ev.srcloc );
    zone->callstack = 0;
    zone->child = -1;

    if( ctx->thread == 0 )
    {
        // Vulkan context is not bound to any single thread.
        zone->thread = CompressThread( ev.thread );
    }
    else
    {
        // OpenGL doesn't need per-zone thread id. It still can be sent,
        // because it may be needed for callstack collection purposes.
        zone->thread = 0;
    }

    m_data.lastTime = std::max( m_data.lastTime, zone->cpuStart );

    auto timeline = &ctx->timeline;
    if( !ctx->stack.empty() )
    {
        auto back = ctx->stack.back();
        if( back->child < 0 )
        {
            back->child = int32_t( m_data.gpuChildren.size() );
            m_data.gpuChildren.push_back( Vector<GpuEvent*>() );
        }
        timeline = &m_data.gpuChildren[back->child];
    }

    timeline->push_back( zone );

    ctx->stack.push_back( zone );

    assert( !ctx->query[ev.queryId] );
    ctx->query[ev.queryId] = zone;
}

void Worker::ProcessGpuZoneBegin( const QueueGpuZoneBegin& ev )
{
    auto zone = m_slab.Alloc<GpuEvent>();
    ProcessGpuZoneBeginImpl( zone, ev );
}

void Worker::ProcessGpuZoneBeginCallstack( const QueueGpuZoneBegin& ev )
{
    auto zone = m_slab.Alloc<GpuEvent>();
    ProcessGpuZoneBeginImpl( zone, ev );

    auto& next = m_nextCallstack[ev.thread];
    next.type = NextCallstackType::Gpu;
    next.gpu = zone;
}

void Worker::ProcessGpuZoneEnd( const QueueGpuZoneEnd& ev )
{
    auto ctx = m_gpuCtxMap[ev.context];
    assert( ctx );

    assert( !ctx->stack.empty() );
    auto zone = ctx->stack.back_and_pop();

    assert( !ctx->query[ev.queryId] );
    ctx->query[ev.queryId] = zone;

    zone->cpuEnd = TscTime( ev.cpuTime - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, zone->cpuEnd );
}

void Worker::ProcessGpuTime( const QueueGpuTime& ev )
{
    auto ctx = m_gpuCtxMap[ev.context];
    assert( ctx );

    int64_t gpuTime;
    if( ctx->period == 1.f )
    {
        gpuTime = ev.gpuTime;
    }
    else
    {
        gpuTime = int64_t( double( ctx->period ) * ev.gpuTime );      // precision loss
    }

    auto zone = ctx->query[ev.queryId];
    assert( zone );
    ctx->query[ev.queryId] = nullptr;

    if( zone->gpuStart == std::numeric_limits<int64_t>::max() )
    {
        zone->gpuStart = ctx->timeDiff + gpuTime;
        m_data.lastTime = std::max( m_data.lastTime, zone->gpuStart );
        ctx->count++;
    }
    else
    {
        zone->gpuEnd = ctx->timeDiff + gpuTime;
        m_data.lastTime = std::max( m_data.lastTime, zone->gpuEnd );

        if( zone->gpuEnd < zone->gpuStart )
        {
            std::swap( zone->gpuEnd, zone->gpuStart );
        }
    }
}

void Worker::ProcessMemAlloc( const QueueMemAlloc& ev )
{
    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );
    NoticeThread( ev.thread );

    assert( m_data.memory.active.find( ev.ptr ) == m_data.memory.active.end() );
    assert( m_data.memory.data.empty() || m_data.memory.data.back().TimeAlloc() <= time );

    m_data.memory.active.emplace( ev.ptr, m_data.memory.data.size() );

    const auto ptr = ev.ptr;
    uint32_t lo;
    uint16_t hi;
    memcpy( &lo, ev.size, 4 );
    memcpy( &hi, ev.size+4, 2 );
    const uint64_t size = lo | ( uint64_t( hi ) << 32 );

    auto& mem = m_data.memory.data.push_next();
    mem.ptr = ptr;
    mem.size = size;
    mem.SetTimeAlloc( time );
    mem.SetThreadAlloc( CompressThread( ev.thread ) );
    mem.SetTimeFree( -1 );
    mem.SetThreadFree( 0 );
    mem.csAlloc = 0;
    mem.csFree = 0;

    const auto low = m_data.memory.low;
    const auto high = m_data.memory.high;
    const auto ptrend = ptr + size;

    m_data.memory.low = std::min( low, ptr );
    m_data.memory.high = std::max( high, ptrend );
    m_data.memory.usage += size;

    MemAllocChanged( time );
}

bool Worker::ProcessMemFree( const QueueMemFree& ev )
{
    if( ev.ptr == 0 ) return false;

    auto it = m_data.memory.active.find( ev.ptr );
    if( it == m_data.memory.active.end() )
    {
        if( !m_ignoreMemFreeFaults )
        {
            MemFreeFailure( ev.thread );
        }
        return false;
    }

    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );
    NoticeThread( ev.thread );

    m_data.memory.frees.push_back( it->second );
    auto& mem = m_data.memory.data[it->second];
    mem.SetTimeFree( time );
    mem.SetThreadFree( CompressThread( ev.thread ) );
    m_data.memory.usage -= mem.size;
    m_data.memory.active.erase( it );

    MemAllocChanged( time );
    return true;
}

void Worker::ProcessMemAllocCallstack( const QueueMemAlloc& ev )
{
    m_lastMemActionCallstack = m_data.memory.data.size();
    ProcessMemAlloc( ev );
    m_lastMemActionWasAlloc = true;
}

void Worker::ProcessMemFreeCallstack( const QueueMemFree& ev )
{
    if( ProcessMemFree( ev ) )
    {
        m_lastMemActionCallstack = m_data.memory.frees.back();
        m_lastMemActionWasAlloc = false;
    }
    else
    {
        m_lastMemActionCallstack = std::numeric_limits<uint64_t>::max();
    }
}

void Worker::ProcessCallstackMemory( const QueueCallstackMemory& ev )
{
    assert( m_pendingCallstackPtr == ev.ptr );
    m_pendingCallstackPtr = 0;

    if( m_lastMemActionCallstack != std::numeric_limits<uint64_t>::max() )
    {
        auto& mem = m_data.memory.data[m_lastMemActionCallstack];
        if( m_lastMemActionWasAlloc )
        {
            mem.csAlloc = m_pendingCallstackId;
        }
        else
        {
            mem.csFree = m_pendingCallstackId;
        }
    }
}

void Worker::ProcessCallstack( const QueueCallstack& ev )
{
    assert( m_pendingCallstackPtr == ev.ptr );
    m_pendingCallstackPtr = 0;

    auto nit = m_nextCallstack.find( m_threadCtx );
    assert( nit != m_nextCallstack.end() );
    auto& next = nit->second;

    switch( next.type )
    {
    case NextCallstackType::Zone:
        next.zone->callstack = m_pendingCallstackId;
        break;
    case NextCallstackType::Gpu:
        next.gpu->callstack = m_pendingCallstackId;
        break;
    case NextCallstackType::Crash:
        m_data.crashEvent.callstack = m_pendingCallstackId;
        break;
    default:
        assert( false );
        break;
    }
}

void Worker::ProcessCallstackAlloc( const QueueCallstackAlloc& ev )
{
    assert( m_pendingCallstackPtr == ev.ptr );
    m_pendingCallstackPtr = 0;

    auto nit = m_nextCallstack.find( m_threadCtx );
    assert( nit != m_nextCallstack.end() );
    auto& next = nit->second;

    switch( next.type )
    {
    case NextCallstackType::Zone:
        next.zone->callstack = m_pendingCallstackId;
        break;
    case NextCallstackType::Gpu:
        next.gpu->callstack = m_pendingCallstackId;
        break;
    case NextCallstackType::Crash:
        m_data.crashEvent.callstack = m_pendingCallstackId;
        break;
    default:
        assert( false );
        break;
    }
}

void Worker::ProcessCallstackFrameSize( const QueueCallstackFrameSize& ev )
{
    assert( !m_callstackFrameStaging );
    assert( m_pendingCallstackSubframes == 0 );
    assert( m_pendingCallstackFrames > 0 );
    m_pendingCallstackFrames--;
    m_pendingCallstackSubframes = ev.size;

    // Frames may be duplicated due to recursion
    auto fmit = m_data.callstackFrameMap.find( PackPointer( ev.ptr ) );
    if( fmit == m_data.callstackFrameMap.end() )
    {
        m_callstackFrameStaging = m_slab.Alloc<CallstackFrameData>();
        m_callstackFrameStaging->size = ev.size;
        m_callstackFrameStaging->data = m_slab.Alloc<CallstackFrame>( ev.size );

        m_callstackFrameStagingPtr = ev.ptr;
    }
}

void Worker::ProcessCallstackFrame( const QueueCallstackFrame& ev )
{
    assert( m_pendingCallstackSubframes > 0 );

    auto nit = m_pendingCustomStrings.find( ev.name );
    assert( nit != m_pendingCustomStrings.end() );
    auto fit = m_pendingCustomStrings.find( ev.file );
    assert( fit != m_pendingCustomStrings.end() );

    if( m_callstackFrameStaging )
    {
        const auto idx = m_callstackFrameStaging->size - m_pendingCallstackSubframes;

        m_callstackFrameStaging->data[idx].name = StringIdx( nit->second.idx );
        m_callstackFrameStaging->data[idx].file = StringIdx( fit->second.idx );
        m_callstackFrameStaging->data[idx].line = ev.line;

        if( --m_pendingCallstackSubframes == 0 )
        {
            assert( m_data.callstackFrameMap.find( PackPointer( m_callstackFrameStagingPtr ) ) == m_data.callstackFrameMap.end() );
            m_data.callstackFrameMap.emplace( PackPointer( m_callstackFrameStagingPtr ), m_callstackFrameStaging );
            m_callstackFrameStaging = nullptr;
        }
    }
    else
    {
        m_pendingCallstackSubframes--;
    }

    m_pendingCustomStrings.erase( nit );
    m_pendingCustomStrings.erase( m_pendingCustomStrings.find( ev.file ) );
}

void Worker::ProcessCrashReport( const QueueCrashReport& ev )
{
    CheckString( ev.text );

    auto& next = m_nextCallstack[m_threadCtx];
    next.type = NextCallstackType::Crash;

    m_data.crashEvent.thread = m_threadCtx;
    m_data.crashEvent.time = TscTime( ev.time - m_data.baseTime );
    m_data.crashEvent.message = ev.text;
    m_data.crashEvent.callstack = 0;
}

void Worker::ProcessSysTime( const QueueSysTime& ev )
{
    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );
    const auto val = ev.sysTime;
    if( !m_sysTimePlot )
    {
        m_sysTimePlot = m_slab.AllocInit<PlotData>();
        m_sysTimePlot->name = 0;
        m_sysTimePlot->type = PlotType::SysTime;
        m_sysTimePlot->min = val;
        m_sysTimePlot->max = val;
        m_sysTimePlot->data.push_back( { time, val } );
        m_data.plots.Data().push_back( m_sysTimePlot );
    }
    else
    {
        assert( !m_sysTimePlot->data.empty() );
        assert( m_sysTimePlot->data.back().time <= time );
        if( m_sysTimePlot->min > val ) m_sysTimePlot->min = val;
        else if( m_sysTimePlot->max < val ) m_sysTimePlot->max = val;
        m_sysTimePlot->data.push_back_non_empty( { time, val } );
    }
}

void Worker::ProcessContextSwitch( const QueueContextSwitch& ev )
{
    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );

    auto& cs = m_data.cpuData[ev.cpu].cs;
    if( ev.oldThread != 0 )
    {
        auto it = m_data.ctxSwitch.find( ev.oldThread );
        if( it != m_data.ctxSwitch.end() )
        {
            auto& data = it->second->v;
            assert( !data.empty() );
            auto& item = data.back();
            assert( item.Start() <= time );
            item.SetEnd( time );
            item.SetReason( ev.reason );
            item.SetState( ev.state );

            const auto dt = time - item.Start();
            it->second->runningTime += dt;

            auto tdit = m_data.cpuThreadData.find( ev.oldThread );
            if( tdit == m_data.cpuThreadData.end() )
            {
                tdit = m_data.cpuThreadData.emplace( ev.oldThread, CpuThreadData {} ).first;
            }
            tdit->second.runningRegions++;
            tdit->second.runningTime += dt;
        }
        if( !cs.empty() )
        {
            auto& cx = cs.back();
            assert( m_data.externalThreadCompress.DecompressThread( cx.Thread() ) == ev.oldThread );
            cx.SetEnd( time );
        }
    }
    if( ev.newThread != 0 )
    {
        auto it = m_data.ctxSwitch.find( ev.newThread );
        if( it == m_data.ctxSwitch.end() )
        {
            auto ctx = m_slab.AllocInit<ContextSwitch>();
            it = m_data.ctxSwitch.emplace( ev.newThread, ctx ).first;
        }
        auto& data = it->second->v;
        ContextSwitchData* item = nullptr;
        bool migration = false;
        if( !data.empty() && data.back().Reason() == ContextSwitchData::Wakeup )
        {
            item = &data.back();
            if( data.size() > 1 )
            {
                migration = data[data.size()-2].Cpu() != ev.cpu;
            }
        }
        else
        {
            assert( data.empty() || (uint64_t)data.back().End() <= time );
            if( !data.empty() )
            {
                migration = data.back().Cpu() != ev.cpu;
            }
            item = &data.push_next();
            item->wakeup = time;
        }
        item->SetStart( time );
        item->SetEnd( -1 );
        item->SetCpu( ev.cpu );
        item->SetReason( -1 );
        item->SetState( -1 );

        auto& cx = cs.push_next();
        cx.SetStart( time );
        cx.SetEnd( -1 );
        cx.SetThread( m_data.externalThreadCompress.CompressThread( ev.newThread ) );

        CheckExternalName( ev.newThread );

        if( migration )
        {
            auto tdit = m_data.cpuThreadData.find( ev.newThread );
            if( tdit == m_data.cpuThreadData.end() )
            {
                tdit = m_data.cpuThreadData.emplace( ev.newThread, CpuThreadData {} ).first;
            }
            tdit->second.migrations++;
        }
    }
}

void Worker::ProcessThreadWakeup( const QueueThreadWakeup& ev )
{
    const auto time = TscTime( ev.time - m_data.baseTime );
    m_data.lastTime = std::max( m_data.lastTime, time );

    auto it = m_data.ctxSwitch.find( ev.thread );
    if( it == m_data.ctxSwitch.end() )
    {
        auto ctx = m_slab.AllocInit<ContextSwitch>();
        it = m_data.ctxSwitch.emplace( ev.thread, ctx ).first;
    }
    auto& data = it->second->v;
    if( !data.empty() && data.back().End() < 0 ) return;        // wakeup of a running thread
    auto& item = data.push_next();
    item.wakeup = time;
    item.SetStart( time );
    item.SetEnd( -1 );
    item.SetCpu( 0 );
    item.SetReason( ContextSwitchData::Wakeup );
    item.SetState( -1 );
}

void Worker::ProcessTidToPid( const QueueTidToPid& ev )
{
    assert( m_data.tidToPid.find( ev.tid ) == m_data.tidToPid.end() );
    m_data.tidToPid.emplace( ev.tid, ev.pid );
}

void Worker::MemAllocChanged( int64_t time )
{
    const auto val = (double)m_data.memory.usage;
    if( !m_data.memory.plot )
    {
        CreateMemAllocPlot();
        m_data.memory.plot->min = val;
        m_data.memory.plot->max = val;
        m_data.memory.plot->data.push_back( { time, val } );
    }
    else
    {
        assert( !m_data.memory.plot->data.empty() );
        assert( m_data.memory.plot->data.back().time <= time );
        if( m_data.memory.plot->min > val ) m_data.memory.plot->min = val;
        else if( m_data.memory.plot->max < val ) m_data.memory.plot->max = val;
        m_data.memory.plot->data.push_back_non_empty( { time, val } );
    }
}

void Worker::CreateMemAllocPlot()
{
    assert( !m_data.memory.plot );
    m_data.memory.plot = m_slab.AllocInit<PlotData>();
    m_data.memory.plot->name = 0;
    m_data.memory.plot->type = PlotType::Memory;
    m_data.memory.plot->data.push_back( { GetFrameBegin( *m_data.framesBase, 0 ), 0. } );
    m_data.plots.Data().push_back( m_data.memory.plot );
}

void Worker::ReconstructMemAllocPlot()
{
    auto& mem = m_data.memory;
#ifdef MY_LIBCPP_SUCKS
    pdqsort_branchless( mem.frees.begin(), mem.frees.end(), [&mem] ( const auto& lhs, const auto& rhs ) { return mem.data[lhs].TimeFree() < mem.data[rhs].TimeFree(); } );
#else
    std::sort( std::execution::par_unseq, mem.frees.begin(), mem.frees.end(), [&mem] ( const auto& lhs, const auto& rhs ) { return mem.data[lhs].TimeFree() < mem.data[rhs].TimeFree(); } );
#endif

    const auto psz = mem.data.size() + mem.frees.size() + 1;

    PlotData* plot;
    {
        std::lock_guard<std::shared_mutex> lock( m_data.lock );
        plot = m_slab.AllocInit<PlotData>();
    }

    plot->name = 0;
    plot->type = PlotType::Memory;
    plot->data.reserve_exact( psz, m_slab );

    auto aptr = mem.data.begin();
    auto aend = mem.data.end();
    auto fptr = mem.frees.begin();
    auto fend = mem.frees.end();

    double max = 0;
    double usage = 0;

    auto ptr = plot->data.data();
    ptr->time = GetFrameBegin( *m_data.framesBase, 0 );
    ptr->val = 0;
    ptr++;

    if( aptr != aend && fptr != fend )
    {
        auto atime = aptr->TimeAlloc();
        auto ftime = mem.data[*fptr].TimeFree();

        for(;;)
        {
            if( atime < ftime )
            {
                usage += int64_t( aptr->size );
                assert( usage >= 0 );
                if( max < usage ) max = usage;
                ptr->time = atime;
                ptr->val = usage;
                ptr++;
                aptr++;
                if( aptr == aend ) break;
                atime = aptr->TimeAlloc();
            }
            else
            {
                usage -= int64_t( mem.data[*fptr].size );
                assert( usage >= 0 );
                if( max < usage ) max = usage;
                ptr->time = ftime;
                ptr->val = usage;
                ptr++;
                fptr++;
                if( fptr == fend ) break;
                ftime = mem.data[*fptr].TimeFree();
            }
        }
    }

    while( aptr != aend )
    {
        assert( aptr->TimeFree() < 0 );
        int64_t time = aptr->TimeAlloc();
        usage += int64_t( aptr->size );
        assert( usage >= 0 );
        if( max < usage ) max = usage;
        ptr->time = time;
        ptr->val = usage;
        ptr++;
        aptr++;
    }
    while( fptr != fend )
    {
        const auto& memData = mem.data[*fptr];
        int64_t time = memData.TimeFree();
        usage -= int64_t( memData.size );
        assert( usage >= 0 );
        assert( max >= usage );
        ptr->time = time;
        ptr->val = usage;
        ptr++;
        fptr++;
    }

    plot->min = 0;
    plot->max = max;

    std::lock_guard<std::shared_mutex> lock( m_data.lock );
    m_data.plots.Data().insert( m_data.plots.Data().begin(), plot );
    m_data.memory.plot = plot;
}

void Worker::ReadTimeline( FileRead& f, ZoneEvent* zone, uint16_t thread, int64_t& refTime )
{
    uint64_t sz;
    f.Read( sz );
    if( sz == 0 )
    {
        zone->child = -1;
    }
    else
    {
        zone->child = m_data.zoneChildren.size();
        // Put placeholder to have proper size of zone children in nested calls
        m_data.zoneChildren.push_back( Vector<ZoneEvent*>() );
        // Real data buffer. Can't use placeholder, as the vector can be reallocated
        // and the buffer address will change, but the reference won't.
        Vector<ZoneEvent*> tmp;
        ReadTimeline( f, tmp, thread, sz, refTime );
        m_data.zoneChildren[zone->child] = std::move( tmp );
    }
}

void Worker::ReadTimelinePre042( FileRead& f, ZoneEvent* zone, uint16_t thread, int fileVer )
{
    uint64_t sz;
    f.Read( sz );
    if( sz == 0 )
    {
        zone->child = -1;
    }
    else
    {
        zone->child = m_data.zoneChildren.size();
        m_data.zoneChildren.push_back( Vector<ZoneEvent*>() );
        Vector<ZoneEvent*> tmp;
        ReadTimelinePre042( f, tmp, thread, sz, fileVer );
        m_data.zoneChildren[zone->child] = std::move( tmp );
    }
}

void Worker::ReadTimelinePre052( FileRead& f, ZoneEvent* zone, uint16_t thread, int64_t& refTime, int fileVer )
{
    uint64_t sz;
    f.Read( sz );
    if( sz == 0 )
    {
        zone->child = -1;
    }
    else
    {
        zone->child = m_data.zoneChildren.size();
        m_data.zoneChildren.push_back( Vector<ZoneEvent*>() );
        Vector<ZoneEvent*> tmp;
        ReadTimelinePre052( f, tmp, thread, sz, refTime, fileVer );
        m_data.zoneChildren[zone->child] = std::move( tmp );
    }
}

void Worker::ReadTimeline( FileRead& f, GpuEvent* zone, int64_t& refTime, int64_t& refGpuTime )
{
    uint64_t sz;
    f.Read( sz );
    if( sz == 0 )
    {
        zone->child = -1;
    }
    else
    {
        zone->child = m_data.gpuChildren.size();
        m_data.gpuChildren.push_back( Vector<GpuEvent*>() );
        Vector<GpuEvent*> tmp;
        ReadTimeline( f, tmp, sz, refTime, refGpuTime );
        m_data.gpuChildren[zone->child] = std::move( tmp );
    }
}

void Worker::ReadTimelinePre052( FileRead& f, GpuEvent* zone, int64_t& refTime, int64_t& refGpuTime, int fileVer )
{
    uint64_t sz;
    f.Read( sz );
    if( sz == 0 )
    {
        zone->child = -1;
    }
    else
    {
        zone->child = m_data.gpuChildren.size();
        m_data.gpuChildren.push_back( Vector<GpuEvent*>() );
        Vector<GpuEvent*> tmp;
        ReadTimelinePre052( f, tmp, sz, refTime, refGpuTime, fileVer );
        m_data.gpuChildren[zone->child] = std::move( tmp );
    }
}

void Worker::ReadTimelineUpdateStatistics( ZoneEvent* zone, uint16_t thread )
{
#ifndef TRACY_NO_STATISTICS
    auto it = m_data.sourceLocationZones.find( zone->SrcLoc() );
    assert( it != m_data.sourceLocationZones.end() );
    auto& slz = it->second;
    auto& ztd = slz.zones.push_next();
    ztd.zone = zone;
    ztd.thread = thread;

    if( zone->end >= 0 )
    {
        auto timeSpan = zone->end - zone->Start();
        if( timeSpan > 0 )
        {
            slz.min = std::min( slz.min, timeSpan );
            slz.max = std::max( slz.max, timeSpan );
            slz.total += timeSpan;
            slz.sumSq += double( timeSpan ) * timeSpan;
            if( zone->child >= 0 )
            {
                for( auto& v : GetZoneChildren( zone->child ) )
                {
                    const auto childSpan = std::max( int64_t( 0 ), v->end - v->Start() );
                    timeSpan -= childSpan;
                }
            }
            slz.selfMin = std::min( slz.selfMin, timeSpan );
            slz.selfMax = std::max( slz.selfMax, timeSpan );
            slz.selfTotal += timeSpan;
        }
    }
#else
    auto it = m_data.sourceLocationZonesCnt.find( zone->SrcLoc() );
    assert( it != m_data.sourceLocationZonesCnt.end() );
    it->second++;
#endif
}

void Worker::ReadTimeline( FileRead& f, Vector<ZoneEvent*>& vec, uint16_t thread, uint64_t size, int64_t& refTime )
{
    assert( size != 0 );
    vec.reserve_exact( size, m_slab );
    m_data.zonesCnt += size;
    auto zone = (ZoneEvent*)m_slab.AllocBig( sizeof( ZoneEvent ) * size );
    auto zptr = zone;
    auto vptr = vec.data();
    for( uint64_t i=0; i<size; i++ )
    {
        *vptr++ = zptr++;
    }
    do
    {
        s_loadProgress.subProgress.fetch_add( 1, std::memory_order_relaxed );
        uint16_t srcloc;
        f.Read( srcloc );
        zone->SetSrcLoc( srcloc );
        // Use zone->end as scratch buffer for zone start time offset.
        f.Read( &zone->end, sizeof( zone->end ) + sizeof( zone->text ) + sizeof( zone->callstack ) + sizeof( zone->name ) );
        refTime += zone->end;
        zone->SetStart( refTime );
        ReadTimeline( f, zone, thread, refTime );
        zone->end = ReadTimeOffset( f, refTime );
#ifdef TRACY_NO_STATISTICS
        ReadTimelineUpdateStatistics( zone, thread );
#endif
    }
    while( ++zone != zptr );
}

void Worker::ReadTimelinePre042( FileRead& f, Vector<ZoneEvent*>& vec, uint16_t thread, uint64_t size, int fileVer )
{
    assert( fileVer <= FileVersion( 0, 4, 1 ) );
    assert( size != 0 );
    vec.reserve_exact( size, m_slab );
    m_data.zonesCnt += size;

    for( uint64_t i=0; i<size; i++ )
    {
        s_loadProgress.subProgress.fetch_add( 1, std::memory_order_relaxed );
        auto zone = m_slab.Alloc<ZoneEvent>();
        vec[i] = zone;
        int64_t start;
        f.Read( start );
        zone->SetStart( start - m_data.baseTime );
        f.Read( zone->end );
        if( zone->end >= 0 ) zone->end -= m_data.baseTime;
        int16_t srcloc;
        f.Read( srcloc );
        zone->SetSrcLoc( srcloc );
        f.Skip( 4 );
        f.Read( &zone->text, sizeof( zone->text ) + sizeof( zone->callstack ) + sizeof( zone->name ) );
        ReadTimelinePre042( f, zone, thread, fileVer );
#ifdef TRACY_NO_STATISTICS
        ReadTimelineUpdateStatistics( zone, thread );
#endif
    }
}

void Worker::ReadTimelinePre052( FileRead& f, Vector<ZoneEvent*>& vec, uint16_t thread, uint64_t size, int64_t& refTime, int fileVer )
{
    assert( fileVer <= FileVersion( 0, 5, 1 ) );
    assert( size != 0 );
    vec.reserve_exact( size, m_slab );
    m_data.zonesCnt += size;
    auto zone = (ZoneEvent*)m_slab.AllocBig( sizeof( ZoneEvent ) * size );
    auto zptr = zone;
    auto vptr = vec.data();
    for( uint64_t i=0; i<size; i++ )
    {
        *vptr++ = zptr++;
    }
    do
    {
        s_loadProgress.subProgress.fetch_add( 1, std::memory_order_relaxed );
        // Use zone->end as scratch buffer for zone start time offset.
        f.Read( &zone->end, sizeof( zone->end ) );
        int16_t srcloc;
        f.Read( srcloc );
        zone->SetSrcLoc( srcloc );
        if( fileVer <= FileVersion( 0, 5, 0 ) )
        {
            f.Skip( 4 );
        }
        else
        {
            f.Skip( 2 );
        }
        f.Read( &zone->text, sizeof( zone->text ) + sizeof( zone->callstack ) + sizeof( zone->name ) );
        refTime += zone->end;
        zone->SetStart( refTime - m_data.baseTime );
        ReadTimelinePre052( f, zone, thread, refTime, fileVer );
        zone->end = ReadTimeOffset( f, refTime );
        if( zone->end >= 0 ) zone->end -= m_data.baseTime;
#ifdef TRACY_NO_STATISTICS
        ReadTimelineUpdateStatistics( zone, thread );
#endif
    }
    while( ++zone != zptr );
}

void Worker::ReadTimeline( FileRead& f, Vector<GpuEvent*>& vec, uint64_t size, int64_t& refTime, int64_t& refGpuTime )
{
    assert( size != 0 );
    vec.reserve_exact( size, m_slab );
    auto zone = (GpuEvent*)m_slab.AllocBig( sizeof( GpuEvent ) * size );
    auto zptr = zone;
    auto vptr = vec.data();
    for( uint64_t i=0; i<size; i++ )
    {
        *vptr++ = zptr++;
    }
    do
    {
        s_loadProgress.subProgress.fetch_add( 1, std::memory_order_relaxed );

        // Use zone->gpuStart as scratch buffer for CPU zone start time offset.
        // Use zone->gpuEnd as scratch buffer for GPU zone start time offset.
        f.Read( &zone->gpuStart, sizeof( zone->gpuStart ) + sizeof( zone->gpuEnd ) + sizeof( zone->srcloc ) + sizeof( zone->callstack ) + sizeof( zone->thread ) );
        refTime += zone->gpuStart;
        refGpuTime += zone->gpuEnd;
        zone->cpuStart = refTime;
        zone->gpuStart = refGpuTime;

        ReadTimeline( f, zone, refTime, refGpuTime );

        zone->cpuEnd = ReadTimeOffset( f, refTime );
        zone->gpuEnd = ReadTimeOffset( f, refGpuTime );
    }
    while( ++zone != zptr );
}

void Worker::ReadTimelinePre052( FileRead& f, Vector<GpuEvent*>& vec, uint64_t size, int64_t& refTime, int64_t& refGpuTime, int fileVer )
{
    assert( size != 0 );
    vec.reserve_exact( size, m_slab );

    for( uint64_t i=0; i<size; i++ )
    {
        s_loadProgress.subProgress.fetch_add( 1, std::memory_order_relaxed );
        auto zone = m_slab.Alloc<GpuEvent>();
        vec[i] = zone;

        if( fileVer <= FileVersion( 0, 4, 1 ) )
        {
            f.Read( zone, sizeof( GpuEvent::cpuStart ) + sizeof( GpuEvent::cpuEnd ) + sizeof( GpuEvent::gpuStart ) + sizeof( GpuEvent::gpuEnd ) );
            zone->cpuStart -= m_data.baseTime;
            if( zone->cpuEnd >= 0 ) zone->cpuEnd -= m_data.baseTime;
            if( zone->gpuStart != std::numeric_limits<int64_t>::max() ) zone->gpuStart -= m_data.baseTime;
            if( zone->gpuEnd >= 0 ) zone->gpuEnd -= m_data.baseTime;
            f.Read( zone->srcloc );
            f.Skip( 2 );
            f.Read( zone->callstack );
            uint64_t thread;
            f.Read( thread );
            if( thread == 0 )
            {
                zone->thread = 0;
            }
            else
            {
                zone->thread = CompressThread( thread );
            }
        }
        else if( fileVer <= FileVersion( 0, 4, 3 ) )
        {
            f.Read( &zone->gpuStart, sizeof( zone->gpuStart ) + sizeof( zone->gpuEnd ) );
            f.Read( zone->srcloc );
            f.Skip( 2 );
            f.Read( zone->callstack );
            refTime += zone->gpuStart;
            refGpuTime += zone->gpuEnd;
            zone->cpuStart = refTime - m_data.baseTime;
            zone->gpuStart = refGpuTime;
            if( zone->gpuStart != std::numeric_limits<int64_t>::max() ) zone->gpuStart -= m_data.baseTime;

            uint64_t thread;
            f.Read( thread );
            if( thread == 0 )
            {
                zone->thread = 0;
            }
            else
            {
                zone->thread = CompressThread( thread );
            }
        }
        else
        {
            // Use zone->gpuStart as scratch buffer for CPU zone start time offset.
            // Use zone->gpuEnd as scratch buffer for GPU zone start time offset.
            f.Read( &zone->gpuStart, sizeof( zone->gpuStart ) + sizeof( zone->gpuEnd ) );
            f.Read( zone->srcloc );
            f.Skip( 2 );
            f.Read( zone->callstack );
            f.Read( zone->thread );
            refTime += zone->gpuStart;
            refGpuTime += zone->gpuEnd;
            zone->cpuStart = refTime - m_data.baseTime;
            zone->gpuStart = refGpuTime;
            if( zone->gpuStart != std::numeric_limits<int64_t>::max() ) zone->gpuStart -= m_data.baseTime;
        }
        ReadTimelinePre052( f, zone, refTime, refGpuTime, fileVer );
        if( fileVer > FileVersion( 0, 4, 1 ) )
        {
            zone->cpuEnd = ReadTimeOffset( f, refTime );
            zone->gpuEnd = ReadTimeOffset( f, refGpuTime );
            if( zone->cpuEnd > 0 ) zone->cpuEnd -= m_data.baseTime;
            if( zone->gpuEnd > 0 ) zone->gpuEnd -= m_data.baseTime;
        }
    }
}

void Worker::Disconnect()
{
    Query( ServerQueryDisconnect, 0 );
    m_disconnect = true;
}

void Worker::Write( FileWrite& f )
{
    f.Write( FileHeader, sizeof( FileHeader ) );

    f.Write( &m_delay, sizeof( m_delay ) );
    f.Write( &m_resolution, sizeof( m_resolution ) );
    f.Write( &m_timerMul, sizeof( m_timerMul ) );
    f.Write( &m_data.lastTime, sizeof( m_data.lastTime ) );
    f.Write( &m_data.frameOffset, sizeof( m_data.frameOffset ) );
    f.Write( &m_pid, sizeof( m_pid ) );

    uint64_t sz = m_captureName.size();
    f.Write( &sz, sizeof( sz ) );
    f.Write( m_captureName.c_str(), sz );

    sz = m_captureProgram.size();
    f.Write( &sz, sizeof( sz ) );
    f.Write( m_captureProgram.c_str(), sz );

    f.Write( &m_captureTime, sizeof( m_captureTime ) );

    sz = m_hostInfo.size();
    f.Write( &sz, sizeof( sz ) );
    f.Write( m_hostInfo.c_str(), sz );

    f.Write( &m_data.crashEvent, sizeof( m_data.crashEvent ) );

    sz = m_data.frames.Data().size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& fd : m_data.frames.Data() )
    {
        int64_t refTime = 0;
        f.Write( &fd->name, sizeof( fd->name ) );
        f.Write( &fd->continuous, sizeof( fd->continuous ) );
        sz = fd->frames.size();
        f.Write( &sz, sizeof( sz ) );
        if( fd->continuous )
        {
            for( auto& fe : fd->frames )
            {
                WriteTimeOffset( f, refTime, fe.start );
                f.Write( &fe.frameImage, sizeof( fe.frameImage ) );
            }
        }
        else
        {
            for( auto& fe : fd->frames )
            {
                WriteTimeOffset( f, refTime, fe.start );
                WriteTimeOffset( f, refTime, fe.end );
                f.Write( &fe.frameImage, sizeof( fe.frameImage ) );
            }
        }
    }

    sz = m_data.stringData.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.stringData )
    {
        uint64_t ptr = (uint64_t)v;
        f.Write( &ptr, sizeof( ptr ) );
        sz = strlen( v );
        f.Write( &sz, sizeof( sz ) );
        f.Write( v, sz );
    }

    sz = m_data.strings.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.strings )
    {
        f.Write( &v.first, sizeof( v.first ) );
        uint64_t ptr = (uint64_t)v.second;
        f.Write( &ptr, sizeof( ptr ) );
    }

    sz = m_data.threadNames.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.threadNames )
    {
        f.Write( &v.first, sizeof( v.first ) );
        uint64_t ptr = (uint64_t)v.second;
        f.Write( &ptr, sizeof( ptr ) );
    }

    sz = m_data.externalNames.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.externalNames )
    {
        f.Write( &v.first, sizeof( v.first ) );
        uint64_t ptr = (uint64_t)v.second.first;
        f.Write( &ptr, sizeof( ptr ) );
        ptr = (uint64_t)v.second.second;
        f.Write( &ptr, sizeof( ptr ) );
    }

    m_data.localThreadCompress.Save( f );
    m_data.externalThreadCompress.Save( f );

    sz = m_data.sourceLocation.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.sourceLocation )
    {
        f.Write( &v.first, sizeof( v.first ) );
        f.Write( &v.second, sizeof( v.second ) );
    }

    sz = m_data.sourceLocationExpand.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.sourceLocationExpand )
    {
        f.Write( &v, sizeof( v ) );
    }

    sz = m_data.sourceLocationPayload.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.sourceLocationPayload )
    {
        f.Write( v, sizeof( *v ) );
    }

#ifndef TRACY_NO_STATISTICS
    sz = m_data.sourceLocationZones.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.sourceLocationZones )
    {
        int16_t id = v.first;
        uint64_t cnt = v.second.zones.size();
        f.Write( &id, sizeof( id ) );
        f.Write( &cnt, sizeof( cnt ) );
    }
#else
    sz = m_data.sourceLocationZonesCnt.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.sourceLocationZonesCnt )
    {
        int16_t id = v.first;
        uint64_t cnt = v.second;
        f.Write( &id, sizeof( id ) );
        f.Write( &cnt, sizeof( cnt ) );
    }
#endif

    sz = m_data.lockMap.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.lockMap )
    {
        f.Write( &v.first, sizeof( v.first ) );
        f.Write( &v.second->srcloc, sizeof( v.second->srcloc ) );
        f.Write( &v.second->type, sizeof( v.second->type ) );
        f.Write( &v.second->valid, sizeof( v.second->valid ) );
        f.Write( &v.second->timeAnnounce, sizeof( v.second->timeAnnounce ) );
        f.Write( &v.second->timeTerminate, sizeof( v.second->timeTerminate ) );
        sz = v.second->threadList.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& t : v.second->threadList )
        {
            f.Write( &t, sizeof( t ) );
        }
        int64_t refTime = v.second->timeAnnounce;
        sz = v.second->timeline.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& lev : v.second->timeline )
        {
            WriteTimeOffset( f, refTime, lev.ptr->Time() );
            const int16_t srcloc = lev.ptr->SrcLoc();
            f.Write( &srcloc, sizeof( srcloc ) );
            f.Write( &lev.ptr->thread, sizeof( lev.ptr->thread ) );
            f.Write( &lev.ptr->type, sizeof( lev.ptr->type ) );
        }
    }

    {
        int64_t refTime = 0;
        sz = m_data.messages.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& v : m_data.messages )
        {
            const auto ptr = (uint64_t)v;
            f.Write( &ptr, sizeof( ptr ) );
            WriteTimeOffset( f, refTime, v->time );
            f.Write( &v->ref, sizeof( v->ref ) );
            f.Write( &v->color, sizeof( v->color ) );
        }
    }

    sz = 0;
    for( auto& v : m_data.threads ) sz += v->count;
    f.Write( &sz, sizeof( sz ) );
    sz = m_data.threads.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& thread : m_data.threads )
    {
        int64_t refTime = 0;
        f.Write( &thread->id, sizeof( thread->id ) );
        f.Write( &thread->count, sizeof( thread->count ) );
        WriteTimeline( f, thread->timeline, refTime );
        sz = thread->messages.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& v : thread->messages )
        {
            auto ptr = uint64_t( v );
            f.Write( &ptr, sizeof( ptr ) );
        }
    }

    sz = 0;
    for( auto& v : m_data.gpuData ) sz += v->count;
    f.Write( &sz, sizeof( sz ) );
    sz = m_data.gpuData.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& ctx : m_data.gpuData )
    {
        int64_t refTime = 0;
        int64_t refGpuTime = 0;
        f.Write( &ctx->thread, sizeof( ctx->thread ) );
        f.Write( &ctx->accuracyBits, sizeof( ctx->accuracyBits ) );
        f.Write( &ctx->count, sizeof( ctx->count ) );
        f.Write( &ctx->period, sizeof( ctx->period ) );
        WriteTimeline( f, ctx->timeline, refTime, refGpuTime );
    }

    sz = m_data.plots.Data().size();
    for( auto& plot : m_data.plots.Data() ) { if( plot->type == PlotType::Memory ) sz--; }
    f.Write( &sz, sizeof( sz ) );
    for( auto& plot : m_data.plots.Data() )
    {
        if( plot->type == PlotType::Memory ) continue;
        f.Write( &plot->type, sizeof( plot->type ) );
        f.Write( &plot->name, sizeof( plot->name ) );
        f.Write( &plot->min, sizeof( plot->min ) );
        f.Write( &plot->max, sizeof( plot->max ) );
        int64_t refTime = 0;
        sz = plot->data.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& v : plot->data )
        {
            WriteTimeOffset( f, refTime, v.time );
            f.Write( &v.val, sizeof( v.val ) );
        }
    }

    {
        int64_t refTime = 0;
        sz = m_data.memory.data.size();
        f.Write( &sz, sizeof( sz ) );
        sz = m_data.memory.active.size();
        f.Write( &sz, sizeof( sz ) );
        sz = m_data.memory.frees.size();
        f.Write( &sz, sizeof( sz ) );
        for( auto& mem : m_data.memory.data )
        {
            f.Write( &mem.ptr, sizeof( mem.ptr ) );
            f.Write( &mem.size, sizeof( mem.size ) );
            f.Write( &mem.csAlloc, sizeof( mem.csAlloc ) );
            f.Write( &mem.csFree, sizeof( mem.csFree ) );

            int64_t timeAlloc = mem.TimeAlloc();
            uint16_t threadAlloc = mem.ThreadAlloc();
            int64_t timeFree = mem.TimeFree();
            uint16_t threadFree = mem.ThreadFree();
            WriteTimeOffset( f, refTime, timeAlloc );
            int64_t freeOffset = timeFree < 0 ? timeFree : timeFree - timeAlloc;
            f.Write( &freeOffset, sizeof( freeOffset ) );
            f.Write( &threadAlloc, sizeof( threadAlloc ) );
            f.Write( &threadFree, sizeof( threadFree ) );
        }
        f.Write( &m_data.memory.high, sizeof( m_data.memory.high ) );
        f.Write( &m_data.memory.low, sizeof( m_data.memory.low ) );
        f.Write( &m_data.memory.usage, sizeof( m_data.memory.usage ) );
    }

    sz = m_data.callstackPayload.size() - 1;
    f.Write( &sz, sizeof( sz ) );
    for( size_t i=1; i<=sz; i++ )
    {
        auto cs = m_data.callstackPayload[i];
        uint8_t csz = cs->size();
        f.Write( &csz, sizeof( csz ) );
        f.Write( cs->data(), sizeof( CallstackFrameId ) * csz );
    }

    sz = m_data.callstackFrameMap.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& frame : m_data.callstackFrameMap )
    {
        f.Write( &frame.first, sizeof( CallstackFrameId ) );
        f.Write( &frame.second->size, sizeof( frame.second->size ) );
        f.Write( frame.second->data, sizeof( CallstackFrame ) * frame.second->size );
    }

    sz = m_data.appInfo.size();
    f.Write( &sz, sizeof( sz ) );
    f.Write( m_data.appInfo.data(), sizeof( m_data.appInfo[0] ) * sz );

    sz = m_data.frameImage.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& fi : m_data.frameImage )
    {
        f.Write( &fi->w, sizeof( fi->w ) );
        f.Write( &fi->h, sizeof( fi->h ) );
        f.Write( &fi->flip, sizeof( fi->flip ) );
        const auto image = UnpackFrameImage( *fi );
        f.Write( image, fi->w * fi->h / 2 );
    }

    // Only save context switches relevant to active threads.
    std::vector<flat_hash_map<uint64_t, ContextSwitch*, nohash<uint64_t>>::const_iterator> ctxValid;
    ctxValid.reserve( m_data.ctxSwitch.size() );
    for( auto it = m_data.ctxSwitch.begin(); it != m_data.ctxSwitch.end(); ++it )
    {
        if( m_data.localThreadCompress.Exists( it->first ) )
        {
            ctxValid.emplace_back( it );
        }
    }
    sz = ctxValid.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& ctx : ctxValid )
    {
        f.Write( &ctx->first, sizeof( ctx->first ) );
        sz = ctx->second->v.size();
        f.Write( &sz, sizeof( sz ) );
        int64_t refTime = 0;
        for( auto& cs : ctx->second->v )
        {
            WriteTimeOffset( f, refTime, cs.wakeup );
            WriteTimeOffset( f, refTime, cs.Start() );
            WriteTimeOffset( f, refTime, cs.End() );
            uint8_t cpu = cs.Cpu();
            int8_t reason = cs.Reason();
            int8_t state = cs.State();
            f.Write( &cpu, sizeof( cpu ) );
            f.Write( &reason, sizeof( reason ) );
            f.Write( &state, sizeof( state ) );
        }
    }

    sz = GetContextSwitchPerCpuCount();
    f.Write( &sz, sizeof( sz ) );
    for( int i=0; i<256; i++ )
    {
        sz = m_data.cpuData[i].cs.size();
        f.Write( &sz, sizeof( sz ) );
        int64_t refTime = 0;
        for( auto& cx : m_data.cpuData[i].cs )
        {
            WriteTimeOffset( f, refTime, cx.Start() );
            WriteTimeOffset( f, refTime, cx.End() );
            uint16_t thread = cx.Thread();
            f.Write( &thread, sizeof( thread ) );
        }
    }

    sz = m_data.tidToPid.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.tidToPid )
    {
        f.Write( &v.first, sizeof( v.first ) );
        f.Write( &v.second, sizeof( v.second ) );
    }

    sz = m_data.cpuThreadData.size();
    f.Write( &sz, sizeof( sz ) );
    for( auto& v : m_data.cpuThreadData )
    {
        f.Write( &v.first, sizeof( v.first ) );
        f.Write( &v.second, sizeof( v.second ) );
    }
}

void Worker::WriteTimeline( FileWrite& f, const Vector<ZoneEvent*>& vec, int64_t& refTime )
{
    uint64_t sz = vec.size();
    f.Write( &sz, sizeof( sz ) );

    for( auto& v : vec )
    {
        int16_t srcloc = v->SrcLoc();
        f.Write( &srcloc, sizeof( srcloc ) );
        int64_t start = v->Start();
        WriteTimeOffset( f, refTime, start );
        f.Write( &v->text, sizeof( v->text ) );
        f.Write( &v->callstack, sizeof( v->callstack ) );
        f.Write( &v->name, sizeof( v->name ) );

        if( v->child < 0 )
        {
            sz = 0;
            f.Write( &sz, sizeof( sz ) );
        }
        else
        {
            WriteTimeline( f, GetZoneChildren( v->child ), refTime );
        }

        WriteTimeOffset( f, refTime, v->end );
    }
}

void Worker::WriteTimeline( FileWrite& f, const Vector<GpuEvent*>& vec, int64_t& refTime, int64_t& refGpuTime )
{
    uint64_t sz = vec.size();
    f.Write( &sz, sizeof( sz ) );

    for( auto& v : vec )
    {
        WriteTimeOffset( f, refTime, v->cpuStart );
        WriteTimeOffset( f, refGpuTime, v->gpuStart );
        f.Write( &v->srcloc, sizeof( v->srcloc ) );
        f.Write( &v->callstack, sizeof( v->callstack ) );
        f.Write( &v->thread, sizeof( v->thread ) );

        if( v->child < 0 )
        {
            sz = 0;
            f.Write( &sz, sizeof( sz ) );
        }
        else
        {
            WriteTimeline( f, GetGpuChildren( v->child ), refTime, refGpuTime );
        }

        WriteTimeOffset( f, refTime, v->cpuEnd );
        WriteTimeOffset( f, refGpuTime, v->gpuEnd );
    }
}

static const char* s_failureReasons[] = {
    "<unknown reason>",
    "Invalid order of zone begin and end events.",
    "Received zone end event without a matching zone begin event.",
    "Zone text transfer destination doesn't match active zone.",
    "Zone name transfer destination doesn't match active zone.",
    "Memory free event without a matching allocation.",
    "Discontinuous frame begin/end mismatch.",
    "Frame image offset is invalid.",
    "Multiple frame images were sent for a single frame.",
};

static_assert( sizeof( s_failureReasons ) / sizeof( *s_failureReasons ) == (int)Worker::Failure::NUM_FAILURES, "Missing failure reason description." );

const char* Worker::GetFailureString( Worker::Failure failure )
{
    return s_failureReasons[(int)failure];
}

const char* Worker::PackFrameImage( const char* image, uint16_t w, uint16_t h, uint32_t& csz )
{
    const auto insz = size_t( w ) * size_t( h ) / 2;
    const auto maxout = LZ4_COMPRESSBOUND( insz );
    if( m_frameImageBufferSize < maxout )
    {
        m_frameImageBufferSize = maxout;
        delete[] m_frameImageBuffer;
        m_frameImageBuffer = new char[maxout];
    }
    const auto outsz = LZ4_compress_default( image, m_frameImageBuffer, insz, maxout );
    csz = uint32_t( outsz );
    auto ptr = (char*)m_slab.AllocBig( outsz );
    memcpy( ptr, m_frameImageBuffer, outsz );
    return ptr;
}

const char* Worker::UnpackFrameImage( const FrameImage& image )
{
    const auto outsz = size_t( image.w ) * size_t( image.h ) / 2;
    if( m_frameImageBufferSize < outsz )
    {
        m_frameImageBufferSize = outsz;
        delete[] m_frameImageBuffer;
        m_frameImageBuffer = new char[outsz];
    }
    LZ4_decompress_safe( image.ptr, m_frameImageBuffer, image.csz, outsz );
    return m_frameImageBuffer;
}

}
