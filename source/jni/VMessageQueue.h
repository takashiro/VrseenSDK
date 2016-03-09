#pragma once

#include "vglobal.h"
#include "core/VString.h"

NV_NAMESPACE_BEGIN

// This is a multiple-producer, single-consumer message queue.

class VMessageQueue
{
public:
    VMessageQueue( int maxMessages );
    ~VMessageQueue();

    // Shut down the message queue once messages are no longer polled
    // to avoid overflowing the queue on message spam.
    void			Shutdown();

    // Thread safe, callable by any thread.
    // The msg text is copied off before return, the caller can free
    // the buffer.
    // The app will abort() with a dump of all messages if the message
    // buffer overflows.
    void			PostString( const char * msg );
    // Builds a printf string and sends it as a message.
    void			PostPrintf( const char * fmt, ... );

    // Same as above but these return false if the queue is full instead of an abort.
    bool			TryPostString( const char * msg );
    bool			TryPostPrintf( const char * fmt, ... );

    // Same as above but these wait until the message has been processed.
    void			SendString( const char * msg );
    void			SendPrintf( const char * fmt, ... );

    // Returns the number slots available for new messages.
    int				SpaceAvailable() const;

    // The other methods are NOT thread safe, and should only be
    // called by the thread that owns the VMessageQueue.

    // Returns NULL if there are no more messages, otherwise returns
    // a string that the caller is now responsible for freeing.
    VString 	nextMessage();

    // Returns immediately if there is already a message in the queue.
    void			SleepUntilMessage();

    // Dumps all unread messages
    void			ClearMessages();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMessageQueue)
};

NV_NAMESPACE_END