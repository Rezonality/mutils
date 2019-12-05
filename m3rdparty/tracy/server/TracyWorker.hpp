#ifndef __TRACYWORKER_HPP__
#define __TRACYWORKER_HPP__

#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string.h>
#include <thread>
#include <vector>

#include "../common/TracyForceInline.hpp"
#include "../common/TracyQueue.hpp"
#include "../common/TracyProtocol.hpp"
#include "../common/TracySocket.hpp"
#include "tracy_flat_hash_map.hpp"
#include "TracyEvent.hpp"
#include "TracyShortPtr.hpp"
#include "TracySlab.hpp"
#include "TracyStringDiscovery.hpp"
#include "TracyThreadCompress.hpp"
#include "TracyVarArray.hpp"

namespace tracy
{

class FileRead;
class FileWrite;

namespace EventType
{
    enum Type : uint32_t
    {
        Locks           = 1 << 0,
        Messages        = 1 << 1,
        Plots           = 1 << 2,
        Memory          = 1 << 3,
        FrameImages     = 1 << 4,
        ContextSwitches = 1 << 5,

        None            = 0,
        All             = std::numeric_limits<uint32_t>::max()
    };
}

struct UnsupportedVersion : public std::exception
{
    UnsupportedVersion( int version ) : version( version ) {}
    int version;
};

struct LegacyVersion : public std::exception
{
    LegacyVersion( int version ) : version ( version ) {}
    int version;
};

struct LoadProgress
{
    enum Stage
    {
        Initialization,
        Locks,
        Messages,
        Zones,
        GpuZones,
        Plots,
        Memory,
        CallStacks,
        FrameImages,
        ContextSwitches,
        ContextSwitchesPerCpu
    };

    LoadProgress() : total( 0 ), progress( 0 ), subTotal( 0 ), subProgress( 0 ) {}

    std::atomic<uint64_t> total;
    std::atomic<uint64_t> progress;
    std::atomic<uint64_t> subTotal;
    std::atomic<uint64_t> subProgress;
};

class Worker
{
public:
#pragma pack( 1 )
    struct ZoneThreadData
    {
        tracy_force_inline ZoneEvent* Zone() const { return (ZoneEvent*)( _zone_thread >> 16 ); }
        tracy_force_inline void SetZone( ZoneEvent* zone ) { assert( ( uint64_t( zone ) & 0xFFFF000000000000 ) == 0 ); memcpy( ((char*)&_zone_thread)+2, &zone, 4 ); memcpy( ((char*)&_zone_thread)+6, ((char*)&zone)+4, 2 ); }
        tracy_force_inline uint16_t Thread() const { return uint16_t( _zone_thread & 0xFFFF ); }
        tracy_force_inline void SetThread( uint16_t thread ) { memcpy( &_zone_thread, &thread, 2 ); }

        uint64_t _zone_thread;
    };

    enum { ZoneThreadDataSize = sizeof( ZoneThreadData ) };
#pragma pack()

    struct CpuThreadTopology
    {
        uint32_t package;
        uint32_t core;
    };

private:
    struct SourceLocationZones
    {
        Vector<ZoneThreadData> zones;
        int64_t min = std::numeric_limits<int64_t>::max();
        int64_t max = std::numeric_limits<int64_t>::min();
        int64_t total = 0;
        double sumSq = 0;
        int64_t selfMin = std::numeric_limits<int64_t>::max();
        int64_t selfMax = std::numeric_limits<int64_t>::min();
        int64_t selfTotal = 0;
    };

    struct CallstackFrameIdHash
    {
        size_t operator()( const CallstackFrameId& id ) const { return id.data; }
        typedef tracy::power_of_two_hash_policy hash_policy;
    };

    struct CallstackFrameIdCompare
    {
        bool operator()( const CallstackFrameId& lhs, const CallstackFrameId& rhs ) const { return lhs.data == rhs.data; }
    };

    struct RevFrameHash
    {
        size_t operator()( const CallstackFrameData* data ) const
        {
            size_t hash = data->size;
            for( uint8_t i=0; i<data->size; i++ )
            {
                const auto& v = data->data[i];
                hash = ( ( hash << 5 ) + hash ) ^ size_t( v.line );
                hash = ( ( hash << 5 ) + hash ) ^ size_t( v.file.Idx() );
                hash = ( ( hash << 5 ) + hash ) ^ size_t( v.name.Idx() );
            }
            return hash;
        }
        typedef tracy::power_of_two_hash_policy hash_policy;
    };

    struct RevFrameComp
    {
        bool operator()( const CallstackFrameData* lhs, const CallstackFrameData* rhs ) const
        {
            if( lhs->size != rhs->size ) return false;
            for( uint8_t i=0; i<lhs->size; i++ )
            {
                if( memcmp( lhs->data + i, rhs->data + i, sizeof( CallstackFrame ) ) != 0 ) return false;
            }
            return true;
        }
    };

    struct DataBlock
    {
        std::shared_mutex lock;
        StringDiscovery<FrameData*> frames;
        FrameData* framesBase;
        Vector<GpuCtxData*> gpuData;
        Vector<short_ptr<MessageData>> messages;
        StringDiscovery<PlotData*> plots;
        Vector<ThreadData*> threads;
        MemData memory;
        uint64_t zonesCnt = 0;
        uint64_t gpuCnt = 0;
        int64_t baseTime = 0;
        int64_t lastTime = 0;
        uint64_t frameOffset = 0;

        flat_hash_map<uint64_t, const char*, nohash<uint64_t>> strings;
        Vector<const char*> stringData;
        flat_hash_map<charutil::StringKey, uint32_t, charutil::StringKey::HasherPOT, charutil::StringKey::Comparator> stringMap;
        flat_hash_map<uint64_t, const char*, nohash<uint64_t>> threadNames;
        flat_hash_map<uint64_t, std::pair<const char*, const char*>, nohash<uint64_t>> externalNames;

        flat_hash_map<uint64_t, SourceLocation, nohash<uint64_t>> sourceLocation;
        Vector<short_ptr<SourceLocation>> sourceLocationPayload;
        flat_hash_map<const SourceLocation*, int16_t, SourceLocationHasher, SourceLocationComparator> sourceLocationPayloadMap;
        Vector<uint64_t> sourceLocationExpand;
#ifndef TRACY_NO_STATISTICS
        flat_hash_map<int16_t, SourceLocationZones, nohash<int16_t>> sourceLocationZones;
        bool sourceLocationZonesReady;
#else
        flat_hash_map<int16_t, uint64_t> sourceLocationZonesCnt;
#endif

        flat_hash_map<VarArray<CallstackFrameId>*, uint32_t, VarArrayHasherPOT<CallstackFrameId>, VarArrayComparator<CallstackFrameId>> callstackMap;
        Vector<short_ptr<VarArray<CallstackFrameId>>> callstackPayload;
        flat_hash_map<CallstackFrameId, CallstackFrameData*, CallstackFrameIdHash, CallstackFrameIdCompare> callstackFrameMap;
        flat_hash_map<CallstackFrameData*, CallstackFrameId, RevFrameHash, RevFrameComp> revFrameMap;

        flat_hash_map<uint32_t, LockMap*, nohash<uint32_t>> lockMap;

        ThreadCompress localThreadCompress;
        ThreadCompress externalThreadCompress;

        Vector<Vector<short_ptr<ZoneEvent>>> zoneChildren;
        Vector<Vector<short_ptr<GpuEvent>>> gpuChildren;

        Vector<Vector<short_ptr<ZoneEvent>>> zoneVectorCache;

        Vector<short_ptr<FrameImage>> frameImage;
        Vector<StringRef> appInfo;

        CrashEvent crashEvent;

        flat_hash_map<uint64_t, ContextSwitch*, nohash<uint64_t>> ctxSwitch;

        CpuData cpuData[256];
        int cpuDataCount = 0;
        flat_hash_map<uint64_t, uint64_t, nohash<uint64_t>> tidToPid;
        flat_hash_map<uint64_t, CpuThreadData, nohash<uint64_t>> cpuThreadData;

        std::pair<uint64_t, ThreadData*> threadDataLast = std::make_pair( std::numeric_limits<uint64_t>::max(), nullptr );
        std::pair<uint64_t, ContextSwitch*> ctxSwitchLast = std::make_pair( std::numeric_limits<uint64_t>::max(), nullptr );
        uint64_t checkSrclocLast = 0;
        std::pair<uint64_t, uint16_t> shrinkSrclocLast = std::make_pair( std::numeric_limits<uint64_t>::max(), 0 );
#ifndef TRACY_NO_STATISTICS
        std::pair<uint16_t, SourceLocationZones*> srclocZonesLast = std::make_pair( std::numeric_limits<uint16_t>::max(), nullptr );
#else
        std::pair<uint16_t, uint64_t*> srclocCntLast = std::make_pair( std::numeric_limits<uint16_t>::max(), nullptr );
#endif

#ifndef TRACY_NO_STATISTICS
        Vector<ContextSwitchUsage> ctxUsage;
        bool ctxUsageReady = false;
#endif

        flat_hash_map<uint32_t, flat_hash_map<uint32_t, std::vector<uint32_t>>> cpuTopology;
        flat_hash_map<uint32_t, CpuThreadTopology, nohash<uint32_t>> cpuTopologyMap;
    };

    struct MbpsBlock
    {
        MbpsBlock() : mbps( 64 ), compRatio( 1.0 ), queue( 0 ), transferred( 0 ) {}

        std::shared_mutex lock;
        std::vector<float> mbps;
        float compRatio;
        size_t queue;
        uint64_t transferred;
    };

    enum class NextCallstackType
    {
        Zone,
        Gpu,
        Crash,
        Message
    };

    struct NextCallstack
    {
        NextCallstackType type;
        union
        {
            ZoneEvent* zone;
            GpuEvent* gpu;
        };
    };

    struct FailureData
    {
        uint64_t thread;
        int16_t srcloc;
    };

    struct FrameImagePending
    {
        const char* image;
        uint32_t csz;
    };

public:
    enum class Failure
    {
        None,
        ZoneStack,
        ZoneText,
        ZoneName,
        MemFree,
        FrameEnd,
        FrameImageIndex,
        FrameImageTwice,

        NUM_FAILURES
    };

    Worker( const char* addr, int port );
    Worker( FileRead& f, EventType::Type eventMask = EventType::All, bool bgTasks = true );
    ~Worker();

    const std::string& GetAddr() const { return m_addr; }
    const std::string& GetCaptureName() const { return m_captureName; }
    const std::string& GetCaptureProgram() const { return m_captureProgram; }
    uint64_t GetCaptureTime() const { return m_captureTime; }
    const std::string& GetHostInfo() const { return m_hostInfo; }
    int64_t GetDelay() const { return m_delay; }
    int64_t GetResolution() const { return m_resolution; }
    uint64_t GetPid() const { return m_pid; };

    std::shared_mutex& GetDataLock() { return m_data.lock; }
    size_t GetFrameCount( const FrameData& fd ) const { return fd.frames.size(); }
    size_t GetFullFrameCount( const FrameData& fd ) const;
    int64_t GetLastTime() const { return m_data.lastTime; }
    uint64_t GetZoneCount() const { return m_data.zonesCnt; }
    uint64_t GetGpuZoneCount() const { return m_data.gpuCnt; }
    uint64_t GetLockCount() const;
    uint64_t GetPlotCount() const;
    uint64_t GetContextSwitchCount() const;
    uint64_t GetContextSwitchPerCpuCount() const;
    bool HasContextSwitches() const { return !m_data.ctxSwitch.empty(); }
    uint64_t GetSrcLocCount() const { return m_data.sourceLocationPayload.size() + m_data.sourceLocation.size(); }
    uint64_t GetCallstackPayloadCount() const { return m_data.callstackPayload.size() - 1; }
    uint64_t GetCallstackFrameCount() const { return m_data.callstackFrameMap.size(); }
    uint32_t GetFrameImageCount() const { return (uint32_t)m_data.frameImage.size(); }
    uint64_t GetStringsCount() const { return m_data.strings.size() + m_data.stringData.size(); }
    uint64_t GetFrameOffset() const { return m_data.frameOffset; }
    const FrameData* GetFramesBase() const { return m_data.framesBase; }
    const Vector<FrameData*>& GetFrames() const { return m_data.frames.Data(); }
    const ContextSwitch* const GetContextSwitchData( uint64_t thread )
    {
        if( m_data.ctxSwitchLast.first == thread ) return m_data.ctxSwitchLast.second;
        return GetContextSwitchDataImpl( thread );
    }
    const CpuData* GetCpuData() const { return m_data.cpuData; }
    int GetCpuDataCpuCount() const { return m_data.cpuDataCount; }
    uint64_t GetPidFromTid( uint64_t tid ) const;
    const flat_hash_map<uint64_t, CpuThreadData, nohash<uint64_t>>& GetCpuThreadData() const { return m_data.cpuThreadData; }
    void GetCpuUsageAtTime( int64_t time, int& own, int& other ) const;

    int64_t GetFrameTime( const FrameData& fd, size_t idx ) const;
    int64_t GetFrameBegin( const FrameData& fd, size_t idx ) const;
    int64_t GetFrameEnd( const FrameData& fd, size_t idx ) const;
    const FrameImage* GetFrameImage( const FrameData& fd, size_t idx ) const;
    std::pair<int, int> GetFrameRange( const FrameData& fd, int64_t from, int64_t to );

    const flat_hash_map<uint32_t, LockMap*, nohash<uint32_t>>& GetLockMap() const { return m_data.lockMap; }
    const Vector<short_ptr<MessageData>>& GetMessages() const { return m_data.messages; }
    const Vector<GpuCtxData*>& GetGpuData() const { return m_data.gpuData; }
    const Vector<PlotData*>& GetPlots() const { return m_data.plots.Data(); }
    const Vector<ThreadData*>& GetThreadData() const { return m_data.threads; }
    const ThreadData* GetThreadData( uint64_t tid ) const;
    const MemData& GetMemData() const { return m_data.memory; }
    const Vector<short_ptr<FrameImage>>& GetFrameImages() const { return m_data.frameImage; }
    const Vector<StringRef>& GetAppInfo() const { return m_data.appInfo; }

    const VarArray<CallstackFrameId>& GetCallstack( uint32_t idx ) const { return *m_data.callstackPayload[idx]; }
    const CallstackFrameData* GetCallstackFrame( const CallstackFrameId& ptr ) const;
    uint64_t GetCanonicalPointer( const CallstackFrameId& id ) const;

    const CrashEvent& GetCrashEvent() const { return m_data.crashEvent; }

    // Some zones may have incomplete timing data (only start time is available, end hasn't arrived yet).
    // GetZoneEnd() will try to infer the end time by looking at child zones (parent zone can't end
    //     before its children have ended).
    // GetZoneEndDirect() will only return zone's direct timing data, without looking at children.
    int64_t GetZoneEnd( const ZoneEvent& ev );
    int64_t GetZoneEnd( const GpuEvent& ev );
    static tracy_force_inline int64_t GetZoneEndDirect( const ZoneEvent& ev ) { return ev.End() >= 0 ? ev.End() : ev.Start(); }
    static tracy_force_inline int64_t GetZoneEndDirect( const GpuEvent& ev ) { return ev.GpuEnd() >= 0 ? ev.GpuEnd() : ev.GpuStart(); }

    const char* GetString( uint64_t ptr ) const;
    const char* GetString( const StringRef& ref ) const;
    const char* GetString( const StringIdx& idx ) const;
    const char* GetThreadName( uint64_t id ) const;
    bool IsThreadLocal( uint64_t id ) const;
    const SourceLocation& GetSourceLocation( int16_t srcloc ) const;
    std::pair<const char*, const char*> GetExternalName( uint64_t id ) const;

    const char* GetZoneName( const SourceLocation& srcloc ) const;
    const char* GetZoneName( const ZoneEvent& ev ) const;
    const char* GetZoneName( const ZoneEvent& ev, const SourceLocation& srcloc ) const;
    const char* GetZoneName( const GpuEvent& ev ) const;
    const char* GetZoneName( const GpuEvent& ev, const SourceLocation& srcloc ) const;

    tracy_force_inline const Vector<short_ptr<ZoneEvent>>& GetZoneChildren( int32_t idx ) const { return m_data.zoneChildren[idx]; }
    tracy_force_inline const Vector<short_ptr<GpuEvent>>& GetGpuChildren( int32_t idx ) const { return m_data.gpuChildren[idx]; }

    std::vector<int16_t> GetMatchingSourceLocation( const char* query, bool ignoreCase ) const;

#ifndef TRACY_NO_STATISTICS
    const SourceLocationZones& GetZonesForSourceLocation( int16_t srcloc ) const;
    const flat_hash_map<int16_t, SourceLocationZones, nohash<int16_t>>& GetSourceLocationZones() const { return m_data.sourceLocationZones; }
    bool AreSourceLocationZonesReady() const { return m_data.sourceLocationZonesReady; }
    bool IsCpuUsageReady() const { return m_data.ctxUsageReady; }
#endif

    tracy_force_inline uint16_t CompressThread( uint64_t thread ) { return m_data.localThreadCompress.CompressThread( thread ); }
    tracy_force_inline uint64_t DecompressThread( uint16_t thread ) const { return m_data.localThreadCompress.DecompressThread( thread ); }
    tracy_force_inline uint64_t DecompressThreadExternal( uint16_t thread ) const { return m_data.externalThreadCompress.DecompressThread( thread ); }

    std::shared_mutex& GetMbpsDataLock() { return m_mbpsData.lock; }
    const std::vector<float>& GetMbpsData() const { return m_mbpsData.mbps; }
    float GetCompRatio() const { return m_mbpsData.compRatio; }
    size_t GetSendQueueSize() const { return m_mbpsData.queue; }
    uint64_t GetDataTransferred() const { return m_mbpsData.transferred; }

    bool HasData() const { return m_hasData.load( std::memory_order_acquire ); }
    bool IsConnected() const { return m_connected.load( std::memory_order_relaxed ); }
    bool IsDataStatic() const { return !m_thread.joinable(); }
    bool IsBackgroundDone() const { return m_backgroundDone.load( std::memory_order_relaxed ); }
    void Shutdown() { m_shutdown.store( true, std::memory_order_relaxed ); }
    void Disconnect();

    void Write( FileWrite& f );
    int GetTraceVersion() const { return m_traceVersion; }
    uint8_t GetHandshakeStatus() const { return m_handshake.load( std::memory_order_relaxed ); }

    static const LoadProgress& GetLoadProgress() { return s_loadProgress; }
    int64_t GetLoadTime() const { return m_loadTime; }

    void ClearFailure() { m_failure = Failure::None; }
    Failure GetFailureType() const { return m_failure; }
    const FailureData& GetFailureData() const { return m_failureData; }
    static const char* GetFailureString( Failure failure );

    void PackFrameImage( char*& buf, size_t& bufsz, const char* image, uint32_t inBytes, uint32_t& csz ) const;
    const char* PackFrameImage( const char* image, uint32_t inBytes, uint32_t& csz );
    const char* UnpackFrameImage( const FrameImage& image );

    const Vector<Parameter>& GetParameters() const { return m_params; }
    void SetParameter( size_t paramIdx, int32_t val );

    const decltype(DataBlock::cpuTopology)& GetCpuTopology() const { return m_data.cpuTopology; }
    const CpuThreadTopology* GetThreadTopology( uint32_t cpuThread ) const;

private:
    void Network();
    void Exec();
    void Query( ServerQuery type, uint64_t data );
    void QueryTerminate();

    tracy_force_inline bool DispatchProcess( const QueueItem& ev, const char*& ptr );
    tracy_force_inline bool Process( const QueueItem& ev );
    tracy_force_inline void ProcessThreadContext( const QueueThreadContext& ev );
    tracy_force_inline void ProcessZoneBegin( const QueueZoneBegin& ev );
    tracy_force_inline void ProcessZoneBeginCallstack( const QueueZoneBegin& ev );
    tracy_force_inline void ProcessZoneBeginAllocSrcLoc( const QueueZoneBegin& ev );
    tracy_force_inline void ProcessZoneBeginAllocSrcLocCallstack( const QueueZoneBegin& ev );
    tracy_force_inline void ProcessZoneEnd( const QueueZoneEnd& ev );
    tracy_force_inline void ProcessZoneValidation( const QueueZoneValidation& ev );
    tracy_force_inline void ProcessFrameMark( const QueueFrameMark& ev );
    tracy_force_inline void ProcessFrameMarkStart( const QueueFrameMark& ev );
    tracy_force_inline void ProcessFrameMarkEnd( const QueueFrameMark& ev );
    tracy_force_inline void ProcessFrameImage( const QueueFrameImage& ev );
    tracy_force_inline void ProcessZoneText( const QueueZoneText& ev );
    tracy_force_inline void ProcessZoneName( const QueueZoneText& ev );
    tracy_force_inline void ProcessLockAnnounce( const QueueLockAnnounce& ev );
    tracy_force_inline void ProcessLockTerminate( const QueueLockTerminate& ev );
    tracy_force_inline void ProcessLockWait( const QueueLockWait& ev );
    tracy_force_inline void ProcessLockObtain( const QueueLockObtain& ev );
    tracy_force_inline void ProcessLockRelease( const QueueLockRelease& ev );
    tracy_force_inline void ProcessLockSharedWait( const QueueLockWait& ev );
    tracy_force_inline void ProcessLockSharedObtain( const QueueLockObtain& ev );
    tracy_force_inline void ProcessLockSharedRelease( const QueueLockRelease& ev );
    tracy_force_inline void ProcessLockMark( const QueueLockMark& ev );
    tracy_force_inline void ProcessPlotData( const QueuePlotData& ev );
    tracy_force_inline void ProcessPlotConfig( const QueuePlotConfig& ev );
    tracy_force_inline void ProcessMessage( const QueueMessage& ev );
    tracy_force_inline void ProcessMessageLiteral( const QueueMessage& ev );
    tracy_force_inline void ProcessMessageColor( const QueueMessageColor& ev );
    tracy_force_inline void ProcessMessageLiteralColor( const QueueMessageColor& ev );
    tracy_force_inline void ProcessMessageCallstack( const QueueMessage& ev );
    tracy_force_inline void ProcessMessageLiteralCallstack( const QueueMessage& ev );
    tracy_force_inline void ProcessMessageColorCallstack( const QueueMessageColor& ev );
    tracy_force_inline void ProcessMessageLiteralColorCallstack( const QueueMessageColor& ev );
    tracy_force_inline void ProcessMessageAppInfo( const QueueMessage& ev );
    tracy_force_inline void ProcessGpuNewContext( const QueueGpuNewContext& ev );
    tracy_force_inline void ProcessGpuZoneBegin( const QueueGpuZoneBegin& ev, bool serial );
    tracy_force_inline void ProcessGpuZoneBeginCallstack( const QueueGpuZoneBegin& ev, bool serial );
    tracy_force_inline void ProcessGpuZoneEnd( const QueueGpuZoneEnd& ev, bool serial );
    tracy_force_inline void ProcessGpuTime( const QueueGpuTime& ev );
    tracy_force_inline void ProcessMemAlloc( const QueueMemAlloc& ev );
    tracy_force_inline bool ProcessMemFree( const QueueMemFree& ev );
    tracy_force_inline void ProcessMemAllocCallstack( const QueueMemAlloc& ev );
    tracy_force_inline void ProcessMemFreeCallstack( const QueueMemFree& ev );
    tracy_force_inline void ProcessCallstackMemory( const QueueCallstackMemory& ev );
    tracy_force_inline void ProcessCallstack( const QueueCallstack& ev );
    tracy_force_inline void ProcessCallstackAlloc( const QueueCallstackAlloc& ev );
    tracy_force_inline void ProcessCallstackFrameSize( const QueueCallstackFrameSize& ev );
    tracy_force_inline void ProcessCallstackFrame( const QueueCallstackFrame& ev );
    tracy_force_inline void ProcessCrashReport( const QueueCrashReport& ev );
    tracy_force_inline void ProcessSysTime( const QueueSysTime& ev );
    tracy_force_inline void ProcessContextSwitch( const QueueContextSwitch& ev );
    tracy_force_inline void ProcessThreadWakeup( const QueueThreadWakeup& ev );
    tracy_force_inline void ProcessTidToPid( const QueueTidToPid& ev );
    tracy_force_inline void ProcessParamSetup( const QueueParamSetup& ev );
    tracy_force_inline void ProcessCpuTopology( const QueueCpuTopology& ev );

    tracy_force_inline ZoneEvent* AllocZoneEvent();
    tracy_force_inline void ProcessZoneBeginImpl( ZoneEvent* zone, const QueueZoneBegin& ev );
    tracy_force_inline void ProcessZoneBeginAllocSrcLocImpl( ZoneEvent* zone, const QueueZoneBegin& ev );
    tracy_force_inline void ProcessGpuZoneBeginImpl( GpuEvent* zone, const QueueGpuZoneBegin& ev, bool serial );

    void ZoneStackFailure( uint64_t thread, const ZoneEvent* ev );
    void ZoneTextFailure( uint64_t thread );
    void ZoneNameFailure( uint64_t thread );
    void MemFreeFailure( uint64_t thread );
    void FrameEndFailure();
    void FrameImageIndexFailure();
    void FrameImageTwiceFailure();

    tracy_force_inline void CheckSourceLocation( uint64_t ptr );
    void NewSourceLocation( uint64_t ptr );
    tracy_force_inline int16_t ShrinkSourceLocation( uint64_t srcloc )
    {
        if( m_data.shrinkSrclocLast.first == srcloc ) return m_data.shrinkSrclocLast.second;
        return ShrinkSourceLocationReal( srcloc );
    }
    int16_t ShrinkSourceLocationReal( uint64_t srcloc );
    int16_t NewShrinkedSourceLocation( uint64_t srcloc );

    tracy_force_inline void MemAllocChanged( int64_t time );
    void CreateMemAllocPlot();
    void ReconstructMemAllocPlot();

    void InsertMessageData( MessageData* msg );

    ThreadData* NoticeThreadReal( uint64_t thread );
    ThreadData* NewThread( uint64_t thread );
    tracy_force_inline ThreadData* NoticeThread( uint64_t thread )
    {
        if( m_data.threadDataLast.first == thread ) return m_data.threadDataLast.second;
        return NoticeThreadReal( thread );
    }
    ThreadData* RetrieveThreadReal( uint64_t thread );
    tracy_force_inline ThreadData* RetrieveThread( uint64_t thread )
    {
        if( m_data.threadDataLast.first == thread ) return m_data.threadDataLast.second;
        return RetrieveThreadReal( thread );
    }

#ifndef TRACY_NO_STATISTICS
    SourceLocationZones* GetSourceLocationZones( uint16_t srcloc )
    {
        if( m_data.srclocZonesLast.first == srcloc ) return m_data.srclocZonesLast.second;
        return GetSourceLocationZonesReal( srcloc );
    }
    SourceLocationZones* GetSourceLocationZonesReal( uint16_t srcloc );
#else
    uint64_t* GetSourceLocationZonesCnt( uint16_t srcloc )
    {
        if( m_data.srclocCntLast.first == srcloc ) return m_data.srclocCntLast.second;
        return GetSourceLocationZonesCntReal( srcloc );
    }
    uint64_t* GetSourceLocationZonesCntReal( uint16_t srcloc );
#endif

    tracy_force_inline void NewZone( ZoneEvent* zone, uint64_t thread );

    void InsertLockEvent( LockMap& lockmap, LockEvent* lev, uint64_t thread, int64_t time );

    void CheckString( uint64_t ptr );
    void CheckThreadString( uint64_t id );
    void CheckExternalName( uint64_t id );

    void AddSourceLocation( const QueueSourceLocation& srcloc );
    void AddSourceLocationPayload( uint64_t ptr, const char* data, size_t sz );

    void AddString( uint64_t ptr, const char* str, size_t sz );
    void AddThreadString( uint64_t id, const char* str, size_t sz );
    void AddCustomString( uint64_t ptr, const char* str, size_t sz );
    void AddExternalName( uint64_t ptr, const char* str, size_t sz );
    void AddExternalThreadName( uint64_t ptr, const char* str, size_t sz );
    void AddFrameImageData( uint64_t ptr, const char* data, size_t sz );

    tracy_force_inline void AddCallstackPayload( uint64_t ptr, const char* data, size_t sz );
    tracy_force_inline void AddCallstackAllocPayload( uint64_t ptr, const char* data, size_t sz );

    void InsertPlot( PlotData* plot, int64_t time, double val );
    void HandlePlotName( uint64_t name, const char* str, size_t sz );
    void HandleFrameName( uint64_t name, const char* str, size_t sz );

    void HandlePostponedPlots();

    StringLocation StoreString( const char* str, size_t sz );
    const ContextSwitch* const GetContextSwitchDataImpl( uint64_t thread );

    tracy_force_inline Vector<short_ptr<ZoneEvent>>& GetZoneChildrenMutable( int32_t idx ) { return m_data.zoneChildren[idx]; }

#ifndef TRACY_NO_STATISTICS
    void ReconstructContextSwitchUsage();
#endif

    tracy_force_inline void ReadTimeline( FileRead& f, ZoneEvent* zone, int64_t& refTime, int32_t& childIdx );
    tracy_force_inline void ReadTimelinePre0510( FileRead& f, ZoneEvent* zone, int64_t& refTime, int fileVer );
    tracy_force_inline void ReadTimeline( FileRead& f, GpuEvent* zone, int64_t& refTime, int64_t& refGpuTime, int32_t& childIdx );
    tracy_force_inline void ReadTimelinePre0510( FileRead& f, GpuEvent* zone, int64_t& refTime, int64_t& refGpuTime, int fileVer );

#ifndef TRACY_NO_STATISTICS
    tracy_force_inline void ReconstructZoneStatistics( ZoneEvent& zone, uint16_t thread );
#else
    tracy_force_inline void CountZoneStatistics( ZoneEvent* zone );
#endif

    void ReadTimeline( FileRead& f, Vector<short_ptr<ZoneEvent>>& vec, uint64_t size, int64_t& refTime, int32_t& childIdx );
    void ReadTimelinePre0510( FileRead& f, Vector<short_ptr<ZoneEvent>>& vec, uint64_t size, int64_t& refTime, int fileVer );
    void ReadTimeline( FileRead& f, Vector<short_ptr<GpuEvent>>& vec, uint64_t size, int64_t& refTime, int64_t& refGpuTime, int32_t& childIdx );
    void ReadTimelinePre0510( FileRead& f, Vector<short_ptr<GpuEvent>>& vec, uint64_t size, int64_t& refTime, int64_t& refGpuTime, int fileVer );

    tracy_force_inline void WriteTimeline( FileWrite& f, const Vector<short_ptr<ZoneEvent>>& vec, int64_t& refTime );
    tracy_force_inline void WriteTimeline( FileWrite& f, const Vector<short_ptr<GpuEvent>>& vec, int64_t& refTime, int64_t& refGpuTime );
    template<typename Adapter, typename V>
    void WriteTimelineImpl( FileWrite& f, const V& vec, int64_t& refTime );
    template<typename Adapter, typename V>
    void WriteTimelineImpl( FileWrite& f, const V& vec, int64_t& refTime, int64_t& refGpuTime );

    int64_t TscTime( int64_t tsc ) { return int64_t( tsc * m_timerMul ); }
    int64_t TscTime( uint64_t tsc ) { return int64_t( tsc * m_timerMul ); }

    Socket m_sock;
    std::string m_addr;
    int m_port;

    std::thread m_thread;
    std::thread m_threadNet;
    std::atomic<bool> m_connected { false };
    std::atomic<bool> m_hasData;
    std::atomic<bool> m_shutdown { false };

    std::atomic<bool> m_backgroundDone { true };
    std::thread m_threadBackground;

    int64_t m_delay;
    int64_t m_resolution;
    double m_timerMul;
    std::string m_captureName;
    std::string m_captureProgram;
    uint64_t m_captureTime;
    std::string m_hostInfo;
    uint64_t m_pid;
    bool m_terminate = false;
    bool m_crashed = false;
    bool m_disconnect = false;
    void* m_stream;     // LZ4_streamDecode_t*
    char* m_buffer;
    int m_bufferOffset;
    bool m_onDemand;
    bool m_ignoreMemFreeFaults;

    short_ptr<GpuCtxData> m_gpuCtxMap[256];
    flat_hash_map<uint64_t, StringLocation, nohash<uint64_t>> m_pendingCustomStrings;
    uint64_t m_pendingCallstackPtr = 0;
    uint32_t m_pendingCallstackId;
    flat_hash_map<uint64_t, int16_t, nohash<uint64_t>> m_pendingSourceLocationPayload;
    Vector<uint64_t> m_sourceLocationQueue;
    flat_hash_map<uint64_t, int16_t, nohash<uint64_t>> m_sourceLocationShrink;
    flat_hash_map<uint64_t, ThreadData*, nohash<uint64_t>> m_threadMap;
    flat_hash_map<uint64_t, NextCallstack, nohash<uint64_t>> m_nextCallstack;
    flat_hash_map<uint64_t, FrameImagePending, nohash<uint64_t>> m_pendingFrameImageData;

    uint32_t m_pendingStrings;
    uint32_t m_pendingThreads;
    uint32_t m_pendingExternalNames;
    uint32_t m_pendingSourceLocation;
    uint32_t m_pendingCallstackFrames;
    uint8_t m_pendingCallstackSubframes;

    CallstackFrameData* m_callstackFrameStaging;
    uint64_t m_callstackFrameStagingPtr;
    uint64_t m_callstackAllocNextIdx = 0;

    uint64_t m_lastMemActionCallstack;
    bool m_lastMemActionWasAlloc;

    Slab<64*1024*1024> m_slab;

    DataBlock m_data;
    MbpsBlock m_mbpsData;

    int m_traceVersion;
    std::atomic<uint8_t> m_handshake { 0 };

    static LoadProgress s_loadProgress;
    int64_t m_loadTime;

    Failure m_failure = Failure::None;
    FailureData m_failureData;

    PlotData* m_sysTimePlot = nullptr;

    Vector<ServerQueryPacket> m_serverQueryQueue;
    size_t m_serverQuerySpaceLeft;

    flat_hash_map<uint64_t, int32_t> m_frameImageStaging;
    char* m_frameImageBuffer = nullptr;
    size_t m_frameImageBufferSize = 0;
    char* m_frameImageCompressedBuffer = nullptr;
    size_t m_frameImageCompressedBufferSize = 0;

    uint64_t m_threadCtx = 0;
    ThreadData* m_threadCtxData = nullptr;
    int64_t m_refTimeThread = 0;
    int64_t m_refTimeSerial = 0;
    int64_t m_refTimeCtx = 0;
    int64_t m_refTimeGpu = 0;

    std::atomic<uint64_t> m_bytes { 0 };
    std::atomic<uint64_t> m_decBytes { 0 };

    struct NetBuffer
    {
        int bufferOffset;
        int size;
    };

    std::vector<NetBuffer> m_netRead;
    std::mutex m_netReadLock;
    std::condition_variable m_netReadCv;

    int m_netWriteCnt = 0;
    std::mutex m_netWriteLock;
    std::condition_variable m_netWriteCv;

#ifdef TRACY_NO_STATISTICS
    Vector<ZoneEvent*> m_zoneEventPool;
#endif

    Vector<Parameter> m_params;
};

}

#endif
