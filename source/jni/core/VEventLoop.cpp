#include "VEventLoop.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/resource.h>

#include "Android/LogUtils.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN

struct VEventLoop::Private
{
    Private(int maxMessages_):
            shutdown( false ),
            maxMessages( maxMessages_ ),
            messages( new message_t[ maxMessages_ ] ),
            head( 0 ),
            tail( 0 ),
            synced( false )
    {
        assert( maxMessages > 0 );

        for ( int i = 0; i < maxMessages; i++ )
        {
            messages[i].string = NULL;
            messages[i].synced = false;
        }

        pthread_mutexattr_t	attr;
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
        pthread_mutex_init( &mutex, &attr );
        pthread_mutexattr_destroy( &attr );
        pthread_cond_init( &posted, NULL );
        pthread_cond_init( &received, NULL );
    }

    // If set true, print all message sends and gets to the log
    static bool		debug;

    bool			shutdown;
    const int 		maxMessages;

    struct message_t
    {
        const char*	 string;
        bool			synced;
    };

    // All messages will be allocated with strdup, and returned to
    // the caller on nextMessage().
    message_t * 	messages;

    // PostMessage() fills in messages[tail%maxMessages], then increments tail
    // If tail > head, nextMessage() will fetch messages[head%maxMessages],
    // then increment head.
    volatile int	head;
    volatile int	tail;
    bool			synced;
    pthread_mutex_t	mutex;
    pthread_cond_t	posted;
    pthread_cond_t	received;

    bool PostMessage( const char * msg, bool sync, bool abortIfFull );
};

bool VEventLoop::Private::debug = false;

int VEventLoop::SpaceAvailable() const
{
    return d->maxMessages - ( d->tail - d->head );
}

VEventLoop::VEventLoop( int maxMessages_ ) :
        d(new Private(maxMessages_))
{

}

VEventLoop::~VEventLoop()
{
    // Free any messages remaining on the queue.
    for ( ; ; )
    {
        const char* msg = nextMessage();
        if ( !msg ) {
            break;
        }
        vInfo( "~VMessageQueue: still on queue: "<<msg);
        free( (void *)msg );
    }

    // Free the queue itself.
    delete[] d->messages;

    pthread_mutex_destroy( &d->mutex );
    pthread_cond_destroy( &d->posted );
    pthread_cond_destroy( &d->received );
}

void VEventLoop::Shutdown()
{
    vInfo( "VMessageQueue shutdown");
    d->shutdown = true;
}

// Thread safe, callable by any thread.
// The msg text is copied off before return, the caller can free
// the buffer.
// The app will abort() with a dump of all messages if the message
// buffer overflows.
bool VEventLoop::Private::PostMessage( const char * msg, bool sync, bool abortIfFull )
{
    if ( shutdown )
    {
        vInfo( "PostMessage( "<<msg<<" ) to shutdown queue");
        return false;
    }
    if ( debug )
    {
        vInfo( "PostMessage( "<<msg<<" )");
    }

    pthread_mutex_lock( &mutex );
    if ( tail - head >= maxMessages )
    {
        pthread_mutex_unlock( &mutex );
        if ( abortIfFull )
        {
            vInfo( "VMessageQueue overflow" );
            for ( int i = head; i < tail; i++ )
            {
                vInfo( messages[i % maxMessages].string );
            }
            vFatal( "Message buffer overflowed" );
        }
        return false;
    }
    const int index = tail % maxMessages;
    messages[index].string = strdup( msg );
    messages[index].synced = sync;
    tail++;
    pthread_cond_signal( &posted );
    if ( sync )
    {
        pthread_cond_wait( &received, &mutex );
    }
    pthread_mutex_unlock( &mutex );

    return true;
}

void VEventLoop::PostString( const char * msg )
{
    d->PostMessage( msg, false, true );
}

void VEventLoop::PostPrintf( const char * fmt, ... )
{
    char bigBuffer[4096];
    va_list	args;
    va_start( args, fmt );
    vsnprintf( bigBuffer, sizeof( bigBuffer ), fmt, args );
    va_end( args );
    d->PostMessage( bigBuffer, false, true );
}

bool VEventLoop::TryPostString( const char * msg )
{
    return d->PostMessage( msg, false, false );
}

bool VEventLoop::TryPostPrintf( const char * fmt, ... )
{
    char bigBuffer[4096];
    va_list	args;
    va_start( args, fmt );
    vsnprintf( bigBuffer, sizeof( bigBuffer ), fmt, args );
    va_end( args );
    return d->PostMessage( bigBuffer, false, false );
}

void VEventLoop::SendString( const char * msg )
{
    d->PostMessage( msg, true, true );
}

void VEventLoop::SendPrintf( const char * fmt, ... )
{
    char bigBuffer[4096];
    va_list	args;
    va_start( args, fmt );
    vsnprintf( bigBuffer, sizeof( bigBuffer ), fmt, args );
    va_end( args );
    d->PostMessage( bigBuffer, true, true );
}

// Returns false if there are no more messages, otherwise returns
// a string that the caller must free.
const char* VEventLoop::nextMessage()
{
    if ( d->synced )
    {
        pthread_cond_signal( &d->received );
        d->synced = false;
    }

    pthread_mutex_lock( &d->mutex );
    if ( d->tail <= d->head )
    {
        pthread_mutex_unlock( &d->mutex );
        return NULL;
    }

    const int index = d->head % d->maxMessages;
    const char* msg = d->messages[index].string;
    d->synced = d->messages[index].synced;
    d->messages[index].string = NULL;
    d->messages[index].synced = false;
    d->head++;
    pthread_mutex_unlock( &d->mutex );

    if ( d->debug )
    {
        vInfo( "nextMessage() : "<<msg);
    }

    return msg;
}

// Returns immediately if there is already a message in the queue.
void VEventLoop::SleepUntilMessage()
{
    if ( d->synced )
    {
        pthread_cond_signal( &d->received );
        d->synced = false;
    }

    pthread_mutex_lock( &d->mutex );
    if ( d->tail > d->head )
    {
        pthread_mutex_unlock( &d->mutex );
        return;
    }

    if ( d->debug )
    {
        vInfo( "SleepUntilMessage() : sleep");
    }

    pthread_cond_wait( &d->posted, &d->mutex );
    pthread_mutex_unlock( &d->mutex );

    if ( d->debug )
    {
        vInfo( "SleepUntilMessage() : awoke");
    }
}

void VEventLoop::ClearMessages()
{
    if ( d->debug )
    {
        vInfo( "ClearMessages()");
    }
    for ( const char* msg = nextMessage(); msg != NULL; msg = nextMessage() )
    {
        vInfo( "ClearMessages: discarding "<<msg);
        free( (void *)msg );
    }
}

NV_NAMESPACE_END