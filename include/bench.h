/*
    Benchmarking
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _BENCH_H
#define _BENCH_H

#define	TRIES	11

#define	REAL_SHORT	   50000
#define	SHORT	 	 1000000
#define	MEDIUM	 	 2000000
#define	LONGER		 7500000	/* for networking data transfers */
#define	ENOUGH		REAL_SHORT

struct result_t
{
  int N;
  uint64 u [TRIES];
  uint64 n [TRIES];
};

struct timeval
{
  long tv_sec;         /* seconds */
  long tv_usec;        /* and microseconds */
};

void    insertinit(result_t *r);
void    insertsort(uint64, uint64, result_t *);
void	save_results(result_t *r);
int	get_enough(int);
void	start(struct timeval *tv);
uint64	stop(struct timeval *begin, struct timeval *end);
void	save_n(uint64);
void	settime(uint64 usecs);
uint64	t_overhead(void);
double	l_overhead(void);
uint64	gettime(void);
uint64	get_n(void);
void	bandwidth(uint64 bytes, uint64 times);
void	use_int(int result);
void	use_pointer(void *result);
int	gettimeofday(struct timeval *tv, struct timezone *tz);

#define	BENCHO(loop_body, overhead_body, enough) { 			\
	int 		__i, __N;					\
	double 		__oh;						\
	result_t 	__overhead, __r;				\
	insertinit(&__overhead); insertinit(&__r);			\
	__N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;\
	if (enough < LONGER) {loop_body;} /* warm the cache */		\
	for (__i = 0; __i < __N; ++__i) {				\
		BENCH1(overhead_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__overhead);	\
		BENCH1(loop_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__r);		\
	}								\
	for (__i = 0; __i < __r.N; ++__i) {				\
		__oh = __overhead.u[__i] / (double)__overhead.n[__i];	\
		if (__r.u[__i] > (uint64)((double)__r.n[__i] * __oh))	\
			__r.u[__i] -= (uint64)((double)__r.n[__i] * __oh); \
		else							\
			__r.u[__i] = 0;					\
	}								\
	save_results(&__r);						\
}

#define	BENCH(loop_body, enough) { 					\
	long		__i, __N;					\
	result_t 	__r;						\
	insertinit(&__r);						\
	__N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;\
	if (enough < LONGER) {loop_body;} /* warm the cache */		\
	for (__i = 0; __i < __N; ++__i) {				\
		BENCH1(loop_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__r);		\
	}								\
	save_results(&__r);						\
}

#define	BENCH1(loop_body, enough) { 					\
	double		__usecs;					\
	BENCH_INNER(loop_body, enough);  				\
	__usecs = (double)gettime();					\
	__usecs -= (double)t_overhead() + (double)get_n() * l_overhead();\
	settime(__usecs >= 0. ? (uint64)__usecs : 0);			\
}
	
#define	BENCH_INNER(loop_body, enough) { 				\
	static uint	__iterations = 1;				\
	int		__enough = get_enough(enough);			\
	uint		__n;						\
	double		__result = 0.;					\
									\
	while(__result < 0.95 * __enough) {				\
		start(0);						\
		for (__n = __iterations; __n > 0; __n--) {		\
			loop_body;					\
		}							\
		__result = (double)stop(0,0);				\
		if (__result < 0.99 * __enough 				\
		    || __result > 1.2 * __enough) {			\
			if (__result > 150.) {				\
				double tmp = __iterations / __result;	\
				tmp *= 1.1 * __enough;			\
				__iterations = (uint)(tmp + 1);	\
			} else {					\
				if (__iterations > (uint)1<<27) {	\
					__result = 0.;			\
					break;				\
				}					\
				__iterations <<= 3;			\
			}						\
		}							\
	} /* while */							\
	save_n((uint64)__iterations); settime((uint64)__result);	\
}

// Perform miscelaneous memory benchmarks
extern void bw_mem(int nbytes, char *mode);

#endif /* _BENCH_H */
