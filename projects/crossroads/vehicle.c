
#include <stdio.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"

/* path. A:0 B:1 C:2 D:3 */
const struct position vehicle_path[4][4][12] = {
	/* from A */ {
		/* to A */
		{{4,0},{4,1},{4,2},{4,3},{4,4},{3,4},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
		/* to B */
		{{4,0},{4,1},{4,2},{5,2},{6,2},{-1,-1},},
		/* to C */
		{{4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
		/* to D */
		{{4,0},{4,1},{4,2},{4,3},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
	},
	/* from B */ {
		/* to A */
		{{6,4},{5,4},{4,4},{3,4},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
		/* to B */
		{{6,4},{5,4},{4,4},{3,4},{2,4},{2,3},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
		/* to C */
		{{6,4},{5,4},{4,4},{4,5},{4,6},{-1,-1},},
		/* to D */
		{{6,4},{5,4},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
	},
	/* from C */ {
		/* to A */
		{{2,6},{2,5},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
		/* to B */
		{{2,6},{2,5},{2,4},{2,3},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
		/* to C */
		{{2,6},{2,5},{2,4},{2,3},{2,2},{3,2},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
		/* to D */
		{{2,6},{2,5},{2,4},{1,4},{0,4},{-1,-1},}
	},
	/* from D */ {
		/* to A */
		{{0,2},{1,2},{2,2},{2,1},{2,0},{-1,-1},},
		/* to B */
		{{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
		/* to C */
		{{0,2},{1,2},{2,2},{3,2},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
		/* to D */
		{{0,2},{1,2},{2,2},{3,2},{4,2},{4,3},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
	}
};

// declaration of danger zone of crossroad 
const struct position danger[8] = {
	{2,2},{2,3},{2,4},{3,2},{3,4},{4,2},{4,3},{4,4}
};

static int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

static bool is_in_danger(struct position pos)
{
	for(int i=0;i<8;i++){
		if(pos.row == danger[i].row && pos.col == danger[i].col){
			return true;
		}
	}
	return false;
}

/* return 0:termination, 1:success, -1:fail */
static int try_move(int start, int dest, int step, struct vehicle_info *vi)
{
	struct position pos_cur, pos_next;

	pos_next = vehicle_path[start][dest][step];
	pos_cur = vi->position;

	if (vi->state == VEHICLE_STATUS_RUNNING) {
		/* check termination */
		if (is_position_outside(pos_next)) {
			/* release previous */
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
			return 0;
		}
	}
	/* check for danger zone */
	/* if it is going to enter the danger zone, cound down the semaphore */
	if(!is_in_danger(pos_cur) && is_in_danger(pos_next)) {
		if(!sema_try_down(&in_danger)){
			/* all resources are being used - return fail */
			return -1;
		}
	}

	/* lock next position */
	if(lock_try_acquire(&vi->map_locks[pos_next.row][pos_next.col])){
		if (vi->state == VEHICLE_STATUS_READY) {
			/* start this vehicle */
			vi->state = VEHICLE_STATUS_RUNNING;
		} else {
			/* release current position */
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);

			/* if it is going to leave the danger zone, cound up the semaphore */
			if(is_in_danger(pos_cur) && !is_in_danger(pos_next)){
				sema_up(&in_danger);
			}
		}
		return 1;
	}
	else{
		/* other car is preemptimg it wants, return fail
		before return, rollback the semaphore if we touched */
		if(!is_in_danger(pos_cur) && is_in_danger(pos_next)) {
			sema_up(&in_danger);
		}
		return -1;
	}
}

void init_on_mainthread(int thread_cnt){
	/* Called once before spawning threads */
	barrier_init(&barrier, thread_cnt);
	sema_init(&in_danger, MAX_THREADS_IN_DANGER);

	barrier_init(&terminated.barrier, 0);
	lock_init(&terminated.wait_lock);
	cond_init(&terminated.wait_cond);
}

void vehicle_loop(void *_vi)
{
	int res;
	int start, dest, step;

	struct vehicle_info *vi = _vi;
	struct position pos_next;

	start = vi->start - 'A';
	dest = vi->dest - 'A';

	vi->position.row = vi->position.col = -1;
	vi->state = VEHICLE_STATUS_READY;

	step = 0;
	while (1) {
		/* vehicle main code */
		res = try_move(start, dest, step, vi);
		if (res == 1) {
			pos_next = vehicle_path[start][dest][step];
			step++;
		}

		/* termination condition. */ 
		else if (res == 0) {
			pos_next = vehicle_path[start][dest][step];
			barrier_count_up(&terminated.barrier);
		}

		else if (res == -1) {
			pos_next = vi->position;
		}

		/* wait for all threads to decide */
		barrier_wait(&barrier);

		/* actual move */
		vi->position = pos_next;

		if(get_barrier_count(&terminated.barrier) != 0){
			lock_acquire(&terminated.wait_lock);

			if(res != 0){
				cond_wait(&terminated.wait_cond, &terminated.wait_lock);
				lock_release(&terminated.wait_lock);
			}
			else{
				barrier_count_down(&barrier);
				lock_release(&terminated.wait_lock);

				if(barrier_wait(&terminated.barrier)){
					lock_acquire(&terminated.wait_lock);
					cond_broadcast(&terminated.wait_cond, &terminated.wait_lock);
					barrier_count_set(&terminated.barrier, 0);
					lock_release(&terminated.wait_lock);
				}
				break;
			}
		}
		/* explicit barrier - all living threads must be reached
		crossroad_step should increase only 1: last thread arrived at the barrier will do. */
		if(barrier_wait(&barrier)){
			// crossroad_step should be protected - barrier is protecting
			crossroads_step++;
			/* unitstep change! */
			unitstep_changed();
		}

		/* wait for unitstep_changed */
		barrier_wait(&barrier);
	}	

	/* status transition must happen before sema_up */
	vi->state = VEHICLE_STATUS_FINISHED;
}
