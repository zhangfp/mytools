/*
1.system time program(kernel time)
2.hardware time program
*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/*
time data struct:

time_t : long int also call timestamp

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;  //daylight saving time
};

//more accurate time
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec; //microseconds
};
*/
/*
time transformation:
timestamp -> utc time   gmtime
timestamp -> local time localtime
timestamp -> string format ctime
struct tm -> string format acstime
*/

time_t getTimeZoneDiffFromUTC()
{
	time_t local_sec;
	time_t utc_sec;
	local_sec = time(NULL); //seconds between 1970.1.1 00:00:00 and now
	struct tm *p_tm = NULL;
	p_tm = gmtime(&local_sec);
	utc_sec = mktime(p_tm);
	return local_sec - utc_sec;
}

int main(int argc, char *argv[])
{
	//get system time
	time_t sec = 0;
	sec = time(NULL); //local time from 1970.1.1 00:00:00 seconds, time_t=long int,在32位系统上到2038年溢出，在64位上没有这个问题。
	printf("sec = %d\n", sec);

	//set system time
	//int stime(time_t *t)

	//struct tm* gmtime(time_t *timep); //utc time time_t -> struct tm
	struct tm *p_tm = NULL;
	struct tm utc = {0};
	p_tm = gmtime(&sec);
	utc = *p_tm;
	printf("utc time hour: %d\n", p_tm->tm_hour);

	printf("timegm:    %d\n", timegm(p_tm));  //utc(tm struct) -> local(time_t)
	printf("timelocal: %d\n", timelocal(p_tm));

	//struct tm* localtime(time_t *timep); //local time time_t -> struct tm
	struct tm local = {0};
	p_tm = localtime(&sec);
	local = *p_tm;
	printf("local time hour: %d\n", p_tm->tm_hour);

	//char *ctime(const time_t *timep); //string time  time_t -> string
	char *p_time = NULL;
	p_time = ctime(&sec);
	printf("string time(ctime): %s", p_time);

	//char *asctime(const struct tm* tm); //string time  struct tm -> string
	p_time = asctime(&utc);
	printf("utc string time(asctime): %s", p_time);

	p_time = asctime(&local);
	printf("local string time(asctime): %s", p_time);

	//time_t mktime(struct tm *tm); //struct tm -> time_t
	time_t mk_sec;
	mk_sec = mktime(&local);
	printf("local timestamp(mktime): %d\n", mk_sec);

	//get more accurate system time
	//gettimeofday
	struct timeval tv;
	gettimeofday(&tv, NULL);
	printf("tv sec: %d, tv_usec: %d\n", tv.tv_sec, tv.tv_usec);

	//set system time
	//settimeofday

	//format date and time
	char str_time[64]={0};
	strftime(str_time, sizeof(str_time), "%Y-%m-%dT%H:%M:%SZ", &local);
	printf("str_time: %s\n", str_time);
	printf("local year: %d\n", local.tm_year);

	//convert string format to tm struct
	struct tm tmp_tm = {0};
	strptime(str_time, "%Y-%m-%dT%H:%M:%SZ", &tmp_tm);
	printf("year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d\n",
			tmp_tm.tm_year+1900,
			tmp_tm.tm_mon+1,
			tmp_tm.tm_mday,
			tmp_tm.tm_hour,
			tmp_tm.tm_min,
			tmp_tm.tm_sec);

	printf("timezonetime: %ds\n", getTimeZoneDiffFromUTC());
	printf("timezonetime: %dh\n", getTimeZoneDiffFromUTC()/3600);
	return 0;
}
