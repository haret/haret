/*
 * a timing utilities library
 *
 * Requires 64bit integers to work.
 *
 * %W% %@%
 *
 * Copyright (c) 1994-1998 Larry McVoy.
 */

#ifdef _WIN32_WCE
#include <windows.h>
#endif

#include "xtypes.h"
#include "bench.h"
#include "output.h"

#define	nz(x)	((x) == 0 ? 1 : (x))

/*
 * I know you think these should be 2^10 and 2^20, but people are quoting
 * disk sizes in powers of 10, and bandwidths are all power of ten.
 * Deal with it.
 */
#define	MB	(1000*1000.0)
#define	KB	(1000.0)

#define bzero(buf,n) memset (buf, 0, n)

static struct timeval start_tv, stop_tv;
volatile uint64	use_result_dummy;	/* !static for optimizers. */
static	uint64	iterations;
static	void	init_timing(void);
static	uint64	tvdelta(struct timeval *start, struct timeval *stop);
static	void	tvsub(struct timeval * tdiff, struct timeval * t1, struct timeval * t0);
static	void	save_median();
static	int	getpagesize();

/*
 * Start timing now.
 */
void
start(struct timeval *tv)
{
	if (tv == NULL) {
		tv = &start_tv;
	}
#ifdef	RUSAGE
	getrusage(RUSAGE_SELF, &ru_start);
#endif
	(void) gettimeofday(tv, (struct timezone *) 0);
}

/*
 * Stop timing and return real time in microseconds.
 */
uint64
stop(struct timeval *begin, struct timeval *end)
{
	if (end == NULL) {
		end = &stop_tv;
	}
	(void) gettimeofday(end, (struct timezone *) 0);
#ifdef	RUSAGE
	getrusage(RUSAGE_SELF, &ru_stop);
#endif

	if (begin == NULL) {
		begin = &start_tv;
	}
	return tvdelta(begin, end);
}

uint64
now(void)
{
	struct timeval t;
	uint64	m;

	(void) gettimeofday(&t, (struct timezone *) 0);
	m = t.tv_sec;
	m *= 1000000;
	m += t.tv_usec;
	return (m);
}

double
Now(void)
{
	struct timeval t;

	(void) gettimeofday(&t, (struct timezone *) 0);
	return (t.tv_sec * 1000000.0 + t.tv_usec);
}

uint64
delta(void)
{
	static struct timeval last;
	struct timeval t;
	struct timeval diff;
	uint64	m;

	(void) gettimeofday(&t, (struct timezone *) 0);
	if (last.tv_usec) {
		tvsub(&diff, &t, &last);
		last = t;
		m = diff.tv_sec;
		m *= 1000000;
		m += diff.tv_usec;
		return (m);
	} else {
		last = t;
		return (0);
	}
}

double
Delta(void)
{
	struct timeval t;
	struct timeval diff;

	(void) gettimeofday(&t, (struct timezone *) 0);
	tvsub(&diff, &t, &start_tv);
	return (diff.tv_sec + diff.tv_usec / 1000000.0);
}

void
save_n(uint64 n)
{
	iterations = n;
}

uint64
get_n(void)
{
	return (iterations);
}

/*
 * Make the time spend be usecs.
 */
void
settime(uint64 usecs)
{
	bzero((void*)&start_tv, sizeof(start_tv));
	stop_tv.tv_sec = (uint)(usecs / 1000000);
	stop_tv.tv_usec = (uint)(usecs % 1000000);
}

void
bandwidth(uint64 bytes, uint64 times)
{
	struct timeval tdiff;
	double  mb, secs;

	tvsub(&tdiff, &stop_tv, &start_tv);
	secs = tdiff.tv_sec;
	secs *= 1000000;
	secs += tdiff.tv_usec;
	secs /= 1000000;
	secs /= times;
	mb = bytes / MB;
	Output (L"%.4f MB in %.4f secs, %.4f MB/sec",	mb, secs, mb/secs);
}

void
kb(uint64 bytes)
{
	struct timeval td;
	double  s, bs;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	bs = bytes / nz(s);
	Output(L"%.0f KB/sec", bs / KB);
}

void
mb(uint64 bytes)
{
	struct timeval td;
	double  s, bs;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	bs = bytes / nz(s);
	Output(L"%.2f MB/sec", bs / MB);
}

void
latency(uint64 xfers, uint64 size)
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	if (xfers > 1) {
		Output(L"%d %dKB xfers in %.2f secs, \t",
		    (int) xfers, (int) (size / KB), s);
	} else {
		Output(L"%.1fKB in \t", size / KB);
	}
	if ((s * 1000 / xfers) > 100) {
		Output(L"%.0f millisec%hs, \t",
		    s * 1000 / xfers, xfers > 1 ? "/xfer" : "s");
	} else {
		Output(L"%.4f millisec%hs, \t",
		    s * 1000 / xfers, xfers > 1 ? "/xfer" : "s");
	}
	if (((xfers * size) / (MB * s)) > 1) {
		Output(L"%.2f MB/sec", (xfers * size) / (MB * s));
	} else {
		Output(L"%.2f KB/sec", (xfers * size) / (KB * s));
	}
}

void
context(uint64 xfers)
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	Output(L"%d context switches in %.2f secs, %.0f microsec/switch",
	    (int)xfers, s, s * 1000000 / xfers);
}

void
nano(char *s, uint64 n)
{
	struct timeval td;
	double  micro;

	tvsub(&td, &stop_tv, &start_tv);
	micro = td.tv_sec * 1000000 + td.tv_usec;
	micro *= 1000;
	Output(L"%s: %.0f nanoseconds", s, micro / n);
}

void
micro(char *s, uint64 n)
{
	struct timeval td;
	double	micro;

	tvsub(&td, &stop_tv, &start_tv);
	micro = td.tv_sec * 1000000 + td.tv_usec;
	micro /= n;
	Output(L"%hs: %.4f microseconds", s, micro);
}

void
micromb(uint64 sz, uint64 n)
{
	struct timeval td;
	double	mb, micro;

	tvsub(&td, &stop_tv, &start_tv);
	micro = td.tv_sec * 1000000 + td.tv_usec;
	micro /= n;
	mb = (double)sz;
	mb /= MB;
	if (micro >= 10) {
		Output(L"%.6f %.0f", mb, micro);
	} else {
		Output(L"%.6f %.3f", mb, micro);
	}
}

void
milli(char *s, uint64 n)
{
	struct timeval td;
	uint64 milli;

	tvsub(&td, &stop_tv, &start_tv);
	milli = td.tv_sec * 1000 + td.tv_usec / 1000;
	milli /= n;
	Output(L"%s: %d milliseconds", s, (int)milli);
}

void
ptime(uint64 n)
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	Output(L"%d in %.2f secs, %.0f microseconds each",
	    (int)n, s, s * 1000000 / n);
}

static uint64
tvdelta(struct timeval *start, struct timeval *stop)
{
	struct timeval td;
	uint64	usecs;

	tvsub(&td, stop, start);
	usecs = td.tv_sec;
	usecs *= 1000000;
	usecs += td.tv_usec;
	return (usecs);
}

static void
tvsub(struct timeval * tdiff, struct timeval * t1, struct timeval * t0)
{
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0 && tdiff->tv_sec > 0) {
		tdiff->tv_sec--;
		tdiff->tv_usec += 1000000;
	}

	/* time shouldn't go backwards!!! */
	if (tdiff->tv_usec < 0 || t1->tv_sec < t0->tv_sec) {
		tdiff->tv_sec = 0;
		tdiff->tv_usec = 0;
	}
}

uint64
gettime(void)
{
	return (tvdelta(&start_tv, &stop_tv));
}

double
timespent(void)
{
	struct timeval td;

	tvsub(&td, &stop_tv, &start_tv);
	return (td.tv_sec + td.tv_usec / 1000000.0);
}

static	char	p64buf[10][20];
static	int	n;

char	*
p64(uint64 big)
{
	char	*s = p64buf[n++];

	if (n == 10) n = 0;
#ifdef  linux
	{
        int     *a = (int*)&big;

        if (a[1]) {
                sprintf(s, "0x%x%08x", a[1], a[0]);
        } else {
                sprintf(s, "0x%x", a[0]);
        }
	}
#endif
#ifdef	__sgi
        sprintf(s, "0x%llx", big);
#endif
	return (s);
}

char *
p64sz(uint64 big)
{
	double	d = (double)big;
	char	*tags = " KMGTPE";
	int	t = 0;
	char	*s = p64buf[n++];

	if (n == 10) n = 0;
	while (d > 512) t++, d /= 1024;
	if (d == 0) {
		return ("0");
	}
	if (d < 100) {
		sprintf(s, "%.4f%c", d, tags[t]);
	} else {
		sprintf(s, "%.2f%c", d, tags[t]);
	}
	return (s);
}

char
last(char *s)
{
	while (*s++)
		;
	return (s[-2]);
}

int
bytes(char *s)
{
	int	n = atoi(s);

	if ((last(s) == 'k') || (last(s) == 'K'))
		n *= 1024;
	if ((last(s) == 'm') || (last(s) == 'M'))
		n *= (1024 * 1024);
	return (n);
}

void
use_int(int result) { use_result_dummy += result; }

void
use_pointer(void *result) { use_result_dummy += (int)result; }

void
insertinit(result_t *r)
{
	int	i;

	r->N = 0;
	for (i = 0; i < TRIES; i++) {
		r->u[i] = 0;
		r->n[i] = 1;
	}
}

/* biggest to smallest */
void
insertsort(uint64 u, uint64 n, result_t *r)
{
	int	i, j;

	if (u == 0) return;

	for (i = 0; i < r->N; ++i) {
		if (u/(double)n > r->u[i]/(double)r->n[i]) {
			for (j = r->N; j > i; --j) {
				r->u[j] = r->u[j-1];
				r->n[j] = r->n[j-1];
			}
			break;
		}
	}
	r->u[i] = u;
	r->n[i] = n;
	r->N++;
}

static result_t results;

void
print_results(int details)
{
	int	i;

	for (i = 0; i < results.N; ++i) {
		fprintf(stderr, "%.2f", (double)results.u[i]/results.n[i]);
		if (i < results.N - 1) fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
	if (details) {
		for (i = 0; i < results.N; ++i) {
			fprintf(stderr, 
				"%llu/%llu", results.u[i], results.n[i]);
			if (i < results.N - 1) fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
	}
		
}

void
get_results(result_t *r)
{
	*r = results;
}

void
save_results(result_t *r)
{
	results = *r;
	save_median();
}

void
save_minimum()
{
	if (results.N == 0) {
		save_n(1);
		settime(0);
	} else {
		save_n(results.n[results.N - 1]);
		settime(results.u[results.N - 1]);
	}
}

static void
save_median()
{
	int	i = results.N / 2;
	uint64	u, n;

	if (results.N == 0) {
		n = 1;
		u = 0;
	} else if (results.N % 2) {
		n = results.n[i];
		u = results.u[i];
	} else {
		n = (results.n[i] + results.n[i-1]) / 2;
		u = (results.u[i] + results.u[i-1]) / 2;
	}
	save_n(n); settime(u);
}

/*
 * The inner loop tracks bench.h but uses a different results array.
 */
static long *
one_op(register long *p)
{
	BENCH_INNER(p = (long *)*p, 0);
	return (p);
}

static long *
two_op(register long *p, register long *q)
{
	BENCH_INNER(p = (long *)*q; q = (long*)*p, 0);
	return (p);
}

static long	*p = (long *)&p;
static long	*q = (long *)&q;

double
l_overhead(void)
{
	int	i;
	uint64	N_save, u_save;
	static	double overhead;
	static	int initialized = 0;
	result_t one, two, r_save;

	init_timing();
	if (initialized) return (overhead);

	initialized = 1;
	get_results(&r_save); N_save = get_n(); u_save = gettime(); 
	insertinit(&one);
	insertinit(&two);
	for (i = 0; i < TRIES; ++i) {
		use_pointer((void*)one_op(p));
		if (gettime() > t_overhead())
			insertsort(gettime() - t_overhead(), get_n(), &one);
		use_pointer((void *)two_op(p, q));
		if (gettime() > t_overhead())
			insertsort(gettime() - t_overhead(), get_n(), &two);
	}
	/*
	 * u1 = (n1 * (overhead + work))
	 * u2 = (n2 * (overhead + 2 * work))
	 * ==> overhead = 2. * u1 / n1 - u2 / n2
	 */
	save_results(&one); save_minimum();
	overhead = 2. * gettime() / (double)get_n();
		
	save_results(&two); save_minimum();
	overhead -= gettime() / (double)get_n();
		
	if (overhead < 0.) overhead = 0.;	/* Gag */

	save_results(&r_save); save_n(N_save); settime(u_save); 
	return (overhead);
}

/*
 * Figure out the timing overhead.  This has to track bench.h
 */
uint64
t_overhead(void)
{
	uint64		N_save, u_save;
	static int	initialized = 0;
	static uint64	overhead = 0;
	struct timeval	tv;
	result_t	r_save;

	init_timing();
	if (initialized) return (overhead);

	initialized = 1;
	if (get_enough(0) <= 50000) {
		/* it is not in the noise, so compute it */
		int		i;
		result_t	r;

		get_results(&r_save); N_save = get_n(); u_save = gettime(); 
		insertinit(&r);
		for (i = 0; i < TRIES; ++i) {
			BENCH_INNER(gettimeofday(&tv, 0), 0);
			insertsort(gettime(), get_n(), &r);
		}
		save_results(&r);
		save_minimum();
		overhead = gettime() / get_n();

		save_results(&r_save); save_n(N_save); settime(u_save); 
	}
	return (overhead);
}

/*
 * Figure out how long to run it.
 * If enough == 0, then they want us to figure it out.
 * If enough is !0 then return it unless we think it is too short.
 */
static	int	long_enough;
static	int	compute_enough();

int
get_enough(int e)
{
	init_timing();
	return (long_enough > e ? long_enough : e);
}


static void
init_timing(void)
{
	static	int done = 0;

	if (done) return;
	done = 1;
	long_enough = compute_enough();
	t_overhead();
	l_overhead();
}

typedef long TYPE;

static TYPE **
enough_duration(register long N, register TYPE ** p)
{
#define	ENOUGH_DURATION_TEN(one)	one one one one one one one one one one
	while (N-- > 0) {
		ENOUGH_DURATION_TEN(p = (TYPE **) *p;);
	}
	return (p);
}

static uint64
duration(long N)
{
	uint64	usecs;
	TYPE   *x = (TYPE *)&x;
	TYPE  **p = (TYPE **)&x;

	start(0);
	p = enough_duration(N, p);
	usecs = stop(0, 0);
	use_pointer((void *)p);
	return (usecs);
}

/*
 * find the minimum time that work "N" takes in "tries" tests
 */
static uint64
time_N(long N)
{
	int     i;
	uint64	usecs;
	result_t r;

	insertinit(&r);
	for (i = 1; i < TRIES; ++i) {
		usecs = duration(N);
		insertsort(usecs, N, &r);
	}
	save_results(&r);
	save_minimum();
	return (gettime());
}

/*
 * return the amount of work needed to run "enough" microseconds
 */
static long
find_N(int enough)
{
	int		tries;
	static long	N = 10000;
	static uint64	usecs = 0;

	if (!usecs) usecs = time_N(N);

	for (tries = 0; tries < 10; ++tries) {
		if (0.98 * enough < usecs && usecs < 1.02 * enough)
			return (N);
		if (usecs < 1000)
			N *= 10;
		else {
			double  n = N;

			n /= usecs;
			n *= enough;
			N = (long)n + 1;
		}
		usecs = time_N(N);
	}
	return (-1);
}

/*
 * We want to verify that small modifications proportionally affect the runtime
 */
static double test_points[] = {1.015, 1.02, 1.035};
static int
test_time(int enough)
{
	int     i;
	long	N;
	uint64	usecs, expected, baseline, diff;

	if ((N = find_N(enough)) <= 0)
		return (0);

	baseline = time_N(N);

	for (i = 0; i < sizeof(test_points) / sizeof(double); ++i) {
		usecs = time_N((int)((double) N * test_points[i]));
		expected = (uint64)((double)baseline * test_points[i]);
		diff = expected > usecs ? expected - usecs : usecs - expected;
		if (diff / (double)expected > 0.0025)
			return (0);
	}
	return (1);
}


/*
 * We want to find the smallest timing interval that has accurate timing
 */
static int     possibilities[] = { 5000, 10000, 50000, 100000 };
static int
compute_enough()
{
	int     i;

	for (i = 0; i < sizeof(possibilities) / sizeof(int); ++i) {
		if (test_time(possibilities[i]))
			return (possibilities[i]);
	}

	/* 
	 * if we can't find a timing interval that is sufficient, 
	 * then use SHORT as a default.
	 */
	return (SHORT);
}

/*
 * This stuff isn't really lib_timing, but ...
 */
void
morefds(void)
{
#ifdef	RLIMIT_NOFILE
	struct	rlimit r;

	getrlimit(RLIMIT_NOFILE, &r);
	r.rlim_cur = r.rlim_max;
	setrlimit(RLIMIT_NOFILE, &r);
#endif
}

void
touch(char *buf, int nbytes)
{
	static	int psize;

	if (!psize) {
		psize = getpagesize();
	}
	while (nbytes > 0) {
		*buf = 1;
		buf += psize;
		nbytes -= psize;
	}
}

#ifdef _WIN32_WCE
static int
getpagesize()
{
	SYSTEM_INFO s;

	GetSystemInfo(&s);
	return ((int)s.dwPageSize);
}

LARGE_INTEGER
getFILETIMEoffset()
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	LARGE_INTEGER		t;
	double			microseconds;
	static LARGE_INTEGER	offset;
	static double		frequencyToMicroseconds;
	static int		initialized = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		QueryPerformanceFrequency(&performanceFrequency);
		QueryPerformanceCounter(&offset);
		frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
	}
	QueryPerformanceCounter(&t);

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = (long)microseconds;
	tv->tv_sec = (long)(t.QuadPart / 1000000);
	tv->tv_usec = (long)(t.QuadPart % 1000000);
	return (0);
}
#endif
