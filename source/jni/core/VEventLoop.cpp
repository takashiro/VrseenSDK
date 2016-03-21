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
            messages[i].sychronized = false;
        }

        pthread_mutexattr_t	attr;
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
        pthread_mutex_init( &mutex, &attr );
        pthread_mutexattr_destroy( &attr );
        pthread_cond_init( &posted, NULL );
        pthread_cond_init( &received, NULL );
    }

    bool			shutdown;
    const int 		maxMessages;

    struct message_t
    {
        const char *string;
        bool sychronized;
    };

    message_t *messages;

    volatile int head;
    volatile int tail;
    bool synced;
    pthread_mutex_t mutex;
    pthread_cond_t posted;
    pthread_cond_t received;

    bool post(const char *msg, bool sync)
    {
        if (shutdown) {
            vInfo("PostMessage(" << msg << ") to shutdown queue");
            return false;
        }

        pthread_mutex_lock(&mutex);
        if (tail - head >= maxMessages) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        const int index = tail % maxMessages;
        messages[index].string = strdup( msg );
        messages[index].sychronized = sync;
        tail++;
        pthread_cond_signal(&posted);
        if (sync) {
            pthread_cond_wait(&received, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        return true;
    }
};

VEventLoop::VEventLoop(int capacity )
    : d(new Private(capacity))
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

    delete d;
}

void VEventLoop::quit()
{
    vInfo( "VMessageQueue shutdown");
    d->shutdown = true;
}

void VEventLoop::post( const char * msg )
{
    d->post(msg, false);
}

void VEventLoop::postf( const char * fmt, ... )
{
    char bigBuffer[4096];
    va_list	args;
    va_start( args, fmt );
    vsnprintf( bigBuffer, sizeof( bigBuffer ), fmt, args );
    va_end( args );
    d->post(bigBuffer, false);
}

void VEventLoop::send( const char * msg )
{
    d->post(msg, true);
}

void VEventLoop::sendf( const char * fmt, ... )
{
    char bigBuffer[4096];
    va_list	args;
    va_start(args, fmt);
    vsnprintf(bigBuffer, sizeof( bigBuffer ), fmt, args );
    va_end(args);
    d->post(bigBuffer, true);
}

// Returns false if there are no more messages, otherwise returns
// a string that the caller must free.
const char* VEventLoop::nextMessage()
{
    if (d->synced) {
        pthread_cond_signal(&d->received);
        d->synced = false;
    }

    pthread_mutex_lock(&d->mutex);
    if (d->tail <= d->head) {
        pthread_mutex_unlock(&d->mutex);
        return NULL;
    }

    const int index = d->head % d->maxMessages;
    const char* msg = d->messages[index].string;
    d->synced = d->messages[index].sychronized;
    d->messages[index].string = NULL;
    d->messages[index].sychronized = false;
    d->head++;
    pthread_mutex_unlock(&d->mutex);

    return msg;
}

// Returns immediately if there is already a message in the queue.
void VEventLoop::wait()
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

    pthread_cond_wait( &d->posted, &d->mutex );
    pthread_mutex_unlock( &d->mutex );
}

void VEventLoop::clear()
{
    for ( const char* msg = nextMessage(); msg != NULL; msg = nextMessage() )
    {
        vInfo( "ClearMessages: discarding "<<msg);
        free( (void *)msg );
    }
}

NV_NAMESPACE_END
