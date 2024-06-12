#include "projects/crossroads/barrier_alt.h"
#include "threads/interrupt.h"

void barrier_init(struct barrier *barrier, unsigned count) {
    barrier->initial_count = count;
    barrier->count = count;
    barrier->waiting = 0;
    lock_init(&barrier->lock);
    for(int i=0;i<MAX_THREADS;i++){
        sema_init(&barrier->wait_sema[i], 0);
    }
}

bool barrier_wait(struct barrier *barrier) {
    /* to determine to detect the last thread arrived at the barrier,
    the function returns bool type */

    // critical section
    lock_acquire(&barrier->lock);
    unsigned index = barrier->waiting++;
    
    // if all threads are arrived, send signal to all threads
    // until then, all threads are waiting
    if (barrier->waiting < barrier->count) {
        lock_release(&barrier->lock);
        sema_down(&barrier->wait_sema[index]);
        return false;
    } else {
        // last thread arrived at the barrier
        barrier->waiting = 0;
        for(int i = 0; i < barrier->count - 1; i++){
            sema_up(&barrier->wait_sema[i]);
        }
        lock_release(&barrier->lock);
        return true;
    }
}

void barrier_count_down(struct barrier *barrier) {
    // count down the current barrier bound
    lock_acquire(&barrier->lock);
    barrier->count--;
    lock_release(&barrier->lock);
}

void barrier_count_up(struct barrier *barrier) {
    // count up the current barrier bound
    lock_acquire(&barrier->lock);
    barrier->count++;
    lock_release(&barrier->lock);
}

void barrier_count_set(struct barrier *barrier, int new_count){
    // set the count to input parameter of the current barrier bound
    lock_acquire(&barrier->lock);
    barrier->count = new_count;
    lock_release(&barrier->lock);
}

int get_barrier_count(struct barrier *barrier){
    int count;
    lock_acquire(&barrier->lock);
    count = barrier->count;
    lock_release(&barrier->lock);
    return count;
}