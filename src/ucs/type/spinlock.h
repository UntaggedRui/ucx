/*
* Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_SPINLOCK_H
#define UCS_SPINLOCK_H

#include <ucs/type/status.h>
#include <pthread.h>
#include <errno.h>

BEGIN_C_DECLS

/** @file spinlock.h */

/**
 * Reentrant spinlock.
 */
typedef struct ucs_spinlock {
    pthread_spinlock_t lock;
    int                count;
    pthread_t          owner;
} ucs_spinlock_t;


#define UCS_SPINLOCK_OWNER_NULL ((pthread_t)-1)

static inline ucs_status_t ucs_spinlock_init(ucs_spinlock_t *lock)
{
    int ret;

    ret = pthread_spin_init(&lock->lock, 0);
    if (ret != 0) {
        return UCS_ERR_IO_ERROR;
    }

    lock->count = 0;
    lock->owner = UCS_SPINLOCK_OWNER_NULL;

    return UCS_OK;
}

static inline ucs_status_t ucs_spinlock_destroy(ucs_spinlock_t *lock)
{
    int ret;

    if (lock->count != 0) {
        return UCS_ERR_BUSY;
    }

    ret = pthread_spin_destroy(&lock->lock);
    if (ret != 0) {
        if (errno == EBUSY) {
            return UCS_ERR_BUSY;
        } else {
            return UCS_ERR_INVALID_PARAM;
        }
    }

    return UCS_OK;
}

static inline int ucs_spin_is_owner(ucs_spinlock_t *lock, pthread_t self)
{
    return lock->owner == self;
}

static inline void ucs_spin_lock(ucs_spinlock_t *lock)
{
    pthread_t self = pthread_self();

    if (ucs_spin_is_owner(lock, self)) {
        ++lock->count;
        return;
    }

    pthread_spin_lock(&lock->lock);
    lock->owner = self;
    ++lock->count;
}

static inline int ucs_spin_trylock(ucs_spinlock_t *lock)
{
    pthread_t self = pthread_self();

    if (ucs_spin_is_owner(lock, self)) {
        ++lock->count;
        return 1;
    }

    if (pthread_spin_trylock(&lock->lock) != 0) {
        return 0;
    }

    lock->owner = self;
    ++lock->count;
    return 1;
}

static inline void ucs_spin_unlock(ucs_spinlock_t *lock)
{
    --lock->count;
    if (lock->count == 0) {
        lock->owner = UCS_SPINLOCK_OWNER_NULL;
        pthread_spin_unlock(&lock->lock);
    }
}

END_C_DECLS

#endif