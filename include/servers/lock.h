#ifndef LOCKSERVER_H
#define LOCKSERVER_H

/* Register a channel to be used exclusively as a lock.
 * Returns a server channel ID which is used in unregister. */
int lockchan_register(int chan);
/* Unregister a previously registered lock, freeing up server
 * resources. */
int lockchan_unregister(int serverchan);
/* Request to lock a channel. Blocks until the lock is acquired.
 * Locks are non-recursive; locking a lock you already hold will
 * result in deadlock. */
int lock_channel(int chan);
/* Unlock a previously-locked channel. Attempting to unlock an
 * unlocked channel will produce UNPREDICTABLE behaviour. */
int unlock_channel(int chan);

void lockserver_init(void);

#endif /* LOCKSERVER_H */
