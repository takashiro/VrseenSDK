#pragma once

#include "vglobal.h"

#include "Atomic.h"

// Define this to compile-in Lockless test logic
//#define OVR_LOCKLESS_TEST

NV_NAMESPACE_BEGIN

// ***** LocklessUpdater

// For single producer cases where you only care about the most recent update, not
// necessarily getting every one that happens (vsync timing, SensorFusion updates).
//
// This is multiple consumer safe.
//
// TODO: This is Android specific

template<class T>
class LocklessUpdater
{
public:
    LocklessUpdater() : updateBegin( 0 ), updateEnd( 0 ) {}

    T		state() const
	{
		// Copy the state out, then retry with the alternate slot
		// if we determine that our copy may have been partially
		// stepped on by a new update.
		T	state;
		int	begin, end, final;

		for(;;)
		{
			// We are adding 0, only using these as atomic memory barriers, so it
			// is ok to cast off the const, allowing GetState() to remain const.
            end   = updateEnd.ExchangeAdd_Sync(0);
            state = slots[ end & 1 ];
            begin = updateBegin.ExchangeAdd_Sync(0);
			if ( begin == end ) {
				return state;
			}

			// The producer is potentially blocked while only having partially
			// written the update, so copy out the other slot.
            state = slots[ (begin & 1) ^ 1 ];
            final = updateBegin.ExchangeAdd_NoSync(0);
			if ( final == begin ) {
				return state;
			}

			// The producer completed the last update and started a new one before
			// we got it copied out, so try fetching the current buffer again.
		}
		return state;
	}

    void	setState( T state )
	{
        const int slot = updateBegin.ExchangeAdd_Sync(1) & 1;
        // Write to (slot ^ 1) because ExchangeAdd returns 'previous' value before add.
        slots[slot ^ 1] = state;
        updateEnd.ExchangeAdd_Sync(1);
	}

    mutable AtomicInt<int> updateBegin;
    mutable AtomicInt<int> updateEnd;
    T		               slots[2];
};


#ifdef OVR_LOCKLESS_TEST
void StartLocklessTest();
#endif


NV_NAMESPACE_END


