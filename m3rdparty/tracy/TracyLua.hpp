#ifndef __TRACYLUA_HPP__
#define __TRACYLUA_HPP__

// Include this file after you include lua headers.

#ifndef TRACY_ENABLE

#include <string.h>

namespace tracy
{

namespace detail
{
static inline int noop( lua_State* L ) { return 0; }
}

static inline void LuaRegister( lua_State* L )
{
    lua_newtable( L );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneBegin" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneBeginN" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneBeginS" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneBeginNS" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneEnd" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneText" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "ZoneName" );
    lua_pushcfunction( L, detail::noop );
    lua_setfield( L, -2, "Message" );
    lua_setglobal( L, "tracy" );
}

static inline char* FindEnd( char* ptr )
{
    unsigned int cnt = 1;
    while( cnt != 0 )
    {
        if( *ptr == '(' ) cnt++;
        else if( *ptr == ')' ) cnt--;
        ptr++;
    }
    return ptr;
}

static inline void LuaRemove( char* script )
{
    while( *script )
    {
        if( strncmp( script, "tracy.", 6 ) == 0 )
        {
            if( strncmp( script + 6, "Zone", 4 ) == 0 )
            {
                if( strncmp( script + 10, "End()", 5 ) == 0 )
                {
                    memset( script, ' ', 15 );
                    script += 15;
                }
                else if( strncmp( script + 10, "Begin()", 7 ) == 0 )
                {
                    memset( script, ' ', 17 );
                    script += 17;
                }
                else if( strncmp( script + 10, "Text(", 5 ) == 0 )
                {
                    auto end = FindEnd( script + 15 );
                    memset( script, ' ', end - script );
                    script = end;
                }
                else if( strncmp( script + 10, "Name(", 5 ) == 0 )
                {
                    auto end = FindEnd( script + 15 );
                    memset( script, ' ', end - script );
                    script = end;
                }
                else if( strncmp( script + 10, "BeginN(", 7 ) == 0 )
                {
                    auto end = FindEnd( script + 17 );
                    memset( script, ' ', end - script );
                    script = end;
                }
                else if( strncmp( script + 10, "BeginS(", 7 ) == 0 )
                {
                    auto end = FindEnd( script + 17 );
                    memset( script, ' ', end - script );
                    script = end;
                }
                else if( strncmp( script + 10, "BeginNS(", 8 ) == 0 )
                {
                    auto end = FindEnd( script + 18 );
                    memset( script, ' ', end - script );
                    script = end;
                }
                else
                {
                    script += 10;
                }
            }
            else if( strncmp( script + 6, "Message(", 8 ) == 0 )
            {
                auto end = FindEnd( script + 14 );
                memset( script, ' ', end - script );
                script = end;
            }
            else
            {
                script += 6;
            }
        }
        else
        {
            script++;
        }
    }
}

}

#else

#include <assert.h>

#include "common/TracyColor.hpp"
#include "common/TracyAlign.hpp"
#include "common/TracyForceInline.hpp"
#include "common/TracySystem.hpp"
#include "client/TracyProfiler.hpp"

namespace tracy
{

#ifdef TRACY_ON_DEMAND
TRACY_API LuaZoneState& GetLuaZoneState();
#endif

namespace detail
{

#ifdef TRACY_HAS_CALLSTACK
static tracy_force_inline void SendLuaCallstack( lua_State* L, uint32_t depth )
{
    assert( depth <= 64 );
    lua_Debug dbg[64];
    const char* func[64];
    uint32_t fsz[64];
    uint32_t ssz[64];
    uint32_t spaceNeeded = 4;     // cnt

    uint32_t cnt;
    for( cnt=0; cnt<depth; cnt++ )
    {
        if( lua_getstack( L, cnt+1, dbg+cnt ) == 0 ) break;
        lua_getinfo( L, "Snl", dbg+cnt );
        func[cnt] = dbg[cnt].name ? dbg[cnt].name : dbg[cnt].short_src;
        fsz[cnt] = uint32_t( strlen( func[cnt] ) );
        ssz[cnt] = uint32_t( strlen( dbg[cnt].source ) );
        spaceNeeded += fsz[cnt] + ssz[cnt];
    }
    spaceNeeded += cnt * ( 4 + 4 + 4 );     // source line, function string length, source string length

    auto ptr = (char*)tracy_malloc( spaceNeeded + 4 );
    auto dst = ptr;
    memcpy( dst, &spaceNeeded, 4 ); dst += 4;
    memcpy( dst, &cnt, 4 ); dst += 4;
    for( uint32_t i=0; i<cnt; i++ )
    {
        const uint32_t line = dbg[i].currentline;
        memcpy( dst, &line, 4 ); dst += 4;
        memcpy( dst, fsz+i, 4 ); dst += 4;
        memcpy( dst, func[i], fsz[i] ); dst += fsz[i];
        memcpy( dst, ssz+i, 4 ); dst += 4;
        memcpy( dst, dbg[i].source, ssz[i] ), dst += ssz[i];
    }
    assert( dst - ptr == spaceNeeded + 4 );

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::CallstackAlloc );
    MemWrite( &item->callstackAlloc.ptr, (uint64_t)ptr );
    MemWrite( &item->callstackAlloc.nativePtr, (uint64_t)Callstack( depth ) );
    tail.store( magic + 1, std::memory_order_release );
}

static inline int LuaZoneBeginS( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    const auto zoneCnt = GetLuaZoneState().counter++;
    if( zoneCnt != 0 && !GetLuaZoneState().active ) return 0;
    GetLuaZoneState().active = GetProfiler().IsConnected();
    if( !GetLuaZoneState().active ) return 0;
#endif

    lua_Debug dbg;
    lua_getstack( L, 1, &dbg );
    lua_getinfo( L, "Snl", &dbg );

    const uint32_t line = dbg.currentline;
    const auto func = dbg.name ? dbg.name : dbg.short_src;
    const auto fsz = strlen( func );
    const auto ssz = strlen( dbg.source );

    // Data layout:
    //  4b  payload size
    //  4b  color
    //  4b  source line
    //  fsz function name
    //  1b  null terminator
    //  ssz source file name
    //  1b  null terminator
    const uint32_t sz = uint32_t( 4 + 4 + 4 + fsz + 1 + ssz + 1 );
    auto ptr = (char*)tracy_malloc( sz );
    memcpy( ptr, &sz, 4 );
    memset( ptr + 4, 0, 4 );
    memcpy( ptr + 8, &line, 4 );
    memcpy( ptr + 12, func, fsz+1 );
    memcpy( ptr + 12 + fsz + 1, dbg.source, ssz + 1 );

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneBeginAllocSrcLocCallstack );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );

#ifdef TRACY_CALLSTACK
    const uint32_t depth = TRACY_CALLSTACK;
#else
    const auto depth = uint32_t( lua_tointeger( L, 1 ) );
#endif
    SendLuaCallstack( L, depth );

    return 0;
}

static inline int LuaZoneBeginNS( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    const auto zoneCnt = GetLuaZoneState().counter++;
    if( zoneCnt != 0 && !GetLuaZoneState().active ) return 0;
    GetLuaZoneState().active = GetProfiler().IsConnected();
    if( !GetLuaZoneState().active ) return 0;
#endif

    lua_Debug dbg;
    lua_getstack( L, 1, &dbg );
    lua_getinfo( L, "Snl", &dbg );

    const uint32_t line = dbg.currentline;
    const auto func = dbg.name ? dbg.name : dbg.short_src;
    size_t nsz;
    const auto name = lua_tolstring( L, 1, &nsz );
    const auto fsz = strlen( func );
    const auto ssz = strlen( dbg.source );

    // Data layout:
    //  4b  payload size
    //  4b  color
    //  4b  source line
    //  fsz function name
    //  1b  null terminator
    //  ssz source file name
    //  1b  null terminator
    //  nsz zone name
    const uint32_t sz = uint32_t( 4 + 4 + 4 + fsz + 1 + ssz + 1 + nsz );
    auto ptr = (char*)tracy_malloc( sz );
    memcpy( ptr, &sz, 4 );
    memset( ptr + 4, 0, 4 );
    memcpy( ptr + 8, &line, 4 );
    memcpy( ptr + 12, func, fsz+1 );
    memcpy( ptr + 12 + fsz + 1, dbg.source, ssz + 1 );
    memcpy( ptr + 12 + fsz + 1 + ssz + 1, name, nsz );

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneBeginAllocSrcLocCallstack );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );

#ifdef TRACY_CALLSTACK
    const uint32_t depth = TRACY_CALLSTACK;
#else
    const auto depth = uint32_t( lua_tointeger( L, 2 ) );
#endif
    SendLuaCallstack( L, depth );

    return 0;
}
#endif

static inline int LuaZoneBegin( lua_State* L )
{
#if defined TRACY_HAS_CALLSTACK && defined TRACY_CALLSTACK
    return LuaZoneBeginS( L );
#else
#ifdef TRACY_ON_DEMAND
    const auto zoneCnt = GetLuaZoneState().counter++;
    if( zoneCnt != 0 && !GetLuaZoneState().active ) return 0;
    GetLuaZoneState().active = GetProfiler().IsConnected();
    if( !GetLuaZoneState().active ) return 0;
#endif

    lua_Debug dbg;
    lua_getstack( L, 1, &dbg );
    lua_getinfo( L, "Snl", &dbg );

    const uint32_t line = dbg.currentline;
    const auto func = dbg.name ? dbg.name : dbg.short_src;
    const auto fsz = strlen( func );
    const auto ssz = strlen( dbg.source );

    // Data layout:
    //  4b  payload size
    //  4b  color
    //  4b  source line
    //  fsz function name
    //  1b  null terminator
    //  ssz source file name
    //  1b  null terminator
    const uint32_t sz = uint32_t( 4 + 4 + 4 + fsz + 1 + ssz + 1 );
    auto ptr = (char*)tracy_malloc( sz );
    memcpy( ptr, &sz, 4 );
    memset( ptr + 4, 0, 4 );
    memcpy( ptr + 8, &line, 4 );
    memcpy( ptr + 12, func, fsz+1 );
    memcpy( ptr + 12 + fsz + 1, dbg.source, ssz + 1 );

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneBeginAllocSrcLoc );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
#endif
}

static inline int LuaZoneBeginN( lua_State* L )
{
#if defined TRACY_HAS_CALLSTACK && defined TRACY_CALLSTACK
    return LuaZoneBeginNS( L );
#else
#ifdef TRACY_ON_DEMAND
    const auto zoneCnt = GetLuaZoneState().counter++;
    if( zoneCnt != 0 && !GetLuaZoneState().active ) return 0;
    GetLuaZoneState().active = GetProfiler().IsConnected();
    if( !GetLuaZoneState().active ) return 0;
#endif

    lua_Debug dbg;
    lua_getstack( L, 1, &dbg );
    lua_getinfo( L, "Snl", &dbg );

    const uint32_t line = dbg.currentline;
    const auto func = dbg.name ? dbg.name : dbg.short_src;
    size_t nsz;
    const auto name = lua_tolstring( L, 1, &nsz );
    const auto fsz = strlen( func );
    const auto ssz = strlen( dbg.source );

    // Data layout:
    //  4b  payload size
    //  4b  color
    //  4b  source line
    //  fsz function name
    //  1b  null terminator
    //  ssz source file name
    //  1b  null terminator
    //  nsz zone name
    const uint32_t sz = uint32_t( 4 + 4 + 4 + fsz + 1 + ssz + 1 + nsz );
    auto ptr = (char*)tracy_malloc( sz );
    memcpy( ptr, &sz, 4 );
    memset( ptr + 4, 0, 4 );
    memcpy( ptr + 8, &line, 4 );
    memcpy( ptr + 12, func, fsz+1 );
    memcpy( ptr + 12 + fsz + 1, dbg.source, ssz + 1 );
    memcpy( ptr + 12 + fsz + 1 + ssz + 1, name, nsz );

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneBeginAllocSrcLoc );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
#endif
}

static inline int LuaZoneEnd( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    assert( GetLuaZoneState().counter != 0 );
    GetLuaZoneState().counter--;
    if( !GetLuaZoneState().active ) return 0;
    if( !GetProfiler().IsConnected() )
    {
        GetLuaZoneState().active = false;
        return 0;
    }
#endif

    Magic magic;
    auto token = GetToken();
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
}

static inline int LuaZoneText( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    if( !GetLuaZoneState().active ) return 0;
    if( !GetProfiler().IsConnected() )
    {
        GetLuaZoneState().active = false;
        return 0;
    }
#endif

    auto txt = lua_tostring( L, 1 );
    const auto size = strlen( txt );

    Magic magic;
    auto token = GetToken();
    auto ptr = (char*)tracy_malloc( size+1 );
    memcpy( ptr, txt, size );
    ptr[size] = '\0';
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneText );
    MemWrite( &item->zoneText.text, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
}

static inline int LuaZoneName( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    if( !GetLuaZoneState().active ) return 0;
    if( !GetProfiler().IsConnected() )
    {
        GetLuaZoneState().active = false;
        return 0;
    }
#endif

    auto txt = lua_tostring( L, 1 );
    const auto size = strlen( txt );

    Magic magic;
    auto token = GetToken();
    auto ptr = (char*)tracy_malloc( size+1 );
    memcpy( ptr, txt, size );
    ptr[size] = '\0';
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::ZoneName );
    MemWrite( &item->zoneText.text, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
}

static inline int LuaMessage( lua_State* L )
{
#ifdef TRACY_ON_DEMAND
    if( !GetProfiler().IsConnected() ) return 0;
#endif

    auto txt = lua_tostring( L, 1 );
    const auto size = strlen( txt );

    Magic magic;
    auto token = GetToken();
    auto ptr = (char*)tracy_malloc( size+1 );
    memcpy( ptr, txt, size );
    ptr[size] = '\0';
    auto& tail = token->get_tail_index();
    auto item = token->enqueue_begin( magic );
    MemWrite( &item->hdr.type, QueueType::Message );
    MemWrite( &item->message.time, Profiler::GetTime() );
    MemWrite( &item->message.text, (uint64_t)ptr );
    tail.store( magic + 1, std::memory_order_release );
    return 0;
}

}

static inline void LuaRegister( lua_State* L )
{
    lua_newtable( L );
    lua_pushcfunction( L, detail::LuaZoneBegin );
    lua_setfield( L, -2, "ZoneBegin" );
    lua_pushcfunction( L, detail::LuaZoneBeginN );
    lua_setfield( L, -2, "ZoneBeginN" );
#ifdef TRACY_HAS_CALLSTACK
    lua_pushcfunction( L, detail::LuaZoneBeginS );
    lua_setfield( L, -2, "ZoneBeginS" );
    lua_pushcfunction( L, detail::LuaZoneBeginNS );
    lua_setfield( L, -2, "ZoneBeginNS" );
#else
    lua_pushcfunction( L, detail::LuaZoneBegin );
    lua_setfield( L, -2, "ZoneBeginS" );
    lua_pushcfunction( L, detail::LuaZoneBeginN );
    lua_setfield( L, -2, "ZoneBeginNS" );
#endif
    lua_pushcfunction( L, detail::LuaZoneEnd );
    lua_setfield( L, -2, "ZoneEnd" );
    lua_pushcfunction( L, detail::LuaZoneText );
    lua_setfield( L, -2, "ZoneText" );
    lua_pushcfunction( L, detail::LuaZoneName );
    lua_setfield( L, -2, "ZoneName" );
    lua_pushcfunction( L, detail::LuaMessage );
    lua_setfield( L, -2, "Message" );
    lua_setglobal( L, "tracy" );
}

static inline void LuaRemove( char* script ) {}

}

#endif

#endif
