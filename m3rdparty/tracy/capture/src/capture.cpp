#ifdef _WIN32
#  include <windows.h>
#endif

#include <chrono>
#include <inttypes.h>
#include <mutex>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/TracyProtocol.hpp"
#include "../../server/TracyFileWrite.hpp"
#include "../../server/TracyMemory.hpp"
#include "../../server/TracyPrint.hpp"
#include "../../server/TracyWorker.hpp"
#include "getopt.h"

#ifndef _MSC_VER
struct sigaction oldsigint;
bool disconnect = false;

void SigInt( int )
{
    disconnect = true;
}
#endif


void Usage()
{
    printf( "Usage: capture -a address -o output.tracy\n" );
    exit( 1 );
}

int main( int argc, char** argv )
{
#ifdef _WIN32
    if( !AttachConsole( ATTACH_PARENT_PROCESS ) )
    {
        AllocConsole();
        SetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), 0x07 );
    }
#endif

    const char* address = nullptr;
    const char* output = nullptr;

    int c;
    while( ( c = getopt( argc, argv, "a:o:" ) ) != -1 )
    {
        switch( c )
        {
        case 'a':
            address = optarg;
            break;
        case 'o':
            output = optarg;
            break;
        default:
            Usage();
            break;
        }
    }

    if( !address || !output ) Usage();

    printf( "Connecting to %s...", address );
    fflush( stdout );
    tracy::Worker worker( address );
    while( !worker.IsConnected() )
    {
        const auto handshake = worker.GetHandshakeStatus();
        if( handshake == tracy::HandshakeProtocolMismatch )
        {
            printf( "\nThe client you are trying to connect to uses incompatible protocol version.\nMake sure you are using the same Tracy version on both client and server.\n" );
            return 1;
        }
        if( handshake == tracy::HandshakeNotAvailable )
        {
            printf( "\nThe client you are trying to connect to is no longer able to sent profiling data,\nbecause another server was already connected to it.\nYou can do the following:\n\n  1. Restart the client application.\n  2. Rebuild the client application with on-demand mode enabled.\n" );
            return 2;
        }
        if( handshake == tracy::HandshakeDropped )
        {
            printf( "\nThe client you are trying to connect to has disconnected during the initial\nconnection handshake. Please check your network configuration.\n" );
            return 3;
        }
    }
    while( !worker.HasData() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    printf( "\nQueue delay: %s\nTimer resolution: %s\n", tracy::TimeToString( worker.GetDelay() ), tracy::TimeToString( worker.GetResolution() ) );

#ifndef _MSC_VER
    struct sigaction sigint;
    memset( &sigint, 0, sizeof( sigint ) );
    sigint.sa_handler = SigInt;
    sigaction( SIGINT, &sigint, &oldsigint );
#endif

    auto& lock = worker.GetMbpsDataLock();

    while( worker.IsConnected() )
    {
#ifndef _MSC_VER
        if( disconnect )
        {
            worker.Disconnect();
            disconnect = false;
        }
#endif

        lock.lock();
        const auto mbps = worker.GetMbpsData().back();
        const auto compRatio = worker.GetCompRatio();
        lock.unlock();

        if( mbps < 0.1f )
        {
            printf( "\33[2K\r\033[36;1m%7.2f Kbps", mbps * 1000.f );
        }
        else
        {
            printf( "\33[2K\r\033[36;1m%7.2f Mbps", mbps );
        }
        printf( " \033[0m /\033[36;1m%5.1f%% \033[0m=\033[33;1m%7.2f Mbps \033[0m| Mem: \033[31;1m%.2f MB\033[0m | \033[33mTime: %s\033[0m", compRatio * 100.f, mbps / compRatio, tracy::memUsage.load( std::memory_order_relaxed ) / ( 1024.f * 1024.f ), tracy::TimeToString( worker.GetLastTime() ) );
        fflush( stdout );

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    const auto& failure = worker.GetFailureType();
    if( failure != tracy::Worker::Failure::None )
    {
        printf( "\n\033[31;1mInstrumentation failure: %s\033[0m", tracy::Worker::GetFailureString( failure ) );
    }

    printf( "\nFrames: %" PRIu64 "\nTime span: %s\nZones: %s\nSaving trace...", worker.GetFrameCount( *worker.GetFramesBase() ), tracy::TimeToString( worker.GetLastTime() ), tracy::RealToString( worker.GetZoneCount(), true ) );
    fflush( stdout );
    auto f = std::unique_ptr<tracy::FileWrite>( tracy::FileWrite::Open( output ) );
    if( f )
    {
        worker.Write( *f );
        printf( " \033[32;1mdone!\033[0m\n" );
    }
    else
    {
        printf( " \033[31;1failed!\033[0m\n" );
    }

    return 0;
}
