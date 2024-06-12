#ifndef __PROJECTS_PROJECT2_VEHICLE_H__
#define __PROJECTS_PROJECT2_VEHICLE_H__

#include "projects/crossroads/position.h"
#include "projects/crossroads/barrier.h"

#define MAX_THREADS_IN_DANGER 7

#define VEHICLE_STATUS_READY 	0
#define VEHICLE_STATUS_RUNNING	1
#define VEHICLE_STATUS_FINISHED	2

struct vehicle_info {
	char id;
	char state;
	char start;
	char dest;
	struct position position;
	struct lock **map_locks;
};

extern int crossroads_step;

struct barrier barrier;
struct semaphore in_danger;

struct terminated {
	struct lock wait_lock;
	struct condition wait_cond;
	struct barrier barrier;
} terminated;

void init_on_mainthread(int thread_cnt);
void vehicle_loop(void *vi);

#endif /* __PROJECTS_PROJECT2_VEHICLE_H__ */