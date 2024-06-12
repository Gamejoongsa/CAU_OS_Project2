#ifndef __PROJECTS_PROJECT2_BARRIER_ALT_H__
#define __PROJECTS_PROJECT2_BARRIER_ALT_H__

#include <threads/synch.h>

#define MAX_THREADS 30

struct barrier {
    unsigned initial_count; // initial count of threads to reach
    unsigned count;         // current count of threads to reach
    unsigned waiting;       // current waiting threads
    struct semaphore wait_sema[MAX_THREADS];
    struct lock lock;
};

void barrier_init(struct barrier *barrier, unsigned count);
bool barrier_wait(struct barrier *barrier);
void barrier_count_down(struct barrier *barrier);
void barrier_count_up(struct barrier *barrier);
void barrier_count_set(struct barrier *barrier, int new_count);
int get_barrier_count(struct barrier *barrier);

#endif /* __PROJECTS_PROJECT2_BARRIER_ALT_H__ */