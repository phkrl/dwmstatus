#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

//char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
char *tzmoscow = "Europe/Moscow";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
getmemory(void)
{
	FILE *info=fopen("/proc/meminfo","r");
	char buf[7];
	double mem, swap;
	int memtotal=-1,memfree=-1,swaptotal=-1,swapfree=-1, count=0;
	char c='\0';
	do
	{
		for(short i=0;i<17;i+=1)
			fgetc(info);
		for(short i=0;i<7;i+=1)
		{
			c=(char)fgetc(info);
			buf[i]=c;
		}
		if(count==0)
			memtotal=atoi(buf);
		else if(count==2)
			memfree=atoi(buf);
		else if(count==14)
			swaptotal=atoi(buf);
		else if(count==15)
			{swapfree=atoi(buf); break;}
		do
		{c=(char)fgetc(info);}
		while(c!='\n');
		count+=1;
	}
	while(c!=EOF);
	mem=100*(double)(memtotal-memfree)/memtotal;
	swap=100*(double)(swaptotal-swapfree)/swaptotal;
	fclose(info);
	return smprintf("mem: %.1f%% swap: %.1f%%", mem, swap);
}

char *
temperature(void)
{
	double temp[8];
	for (short i=0;i<8;i++)
    {
        char *add;
        add=smprintf("/sys/class/thermal/thermal_zone%d/temp",i);
        FILE *f=fopen(add,"r");
        int t;
        fscanf(f,"%d",&t);
        temp[i]=(int)(t/1000)+0.1*((int)(t/100)-10*(int)(t/1000));
        fclose(f);
        free(add);
    }
    return smprintf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f",temp[0],temp[1],temp[2],temp[3],temp[4],temp[5],temp[6],temp[7]);
}

char *
power(void)
{
	unsigned int a,b;
	FILE *f1=fopen("/sys/class/power_supply/BAT0/energy_now","r"),*f2=fopen("/sys/class/power_supply/BAT0/energy_full","r");
	fscanf(f1,"%u",&a);
	fscanf(f2,"%u",&b);
	fclose(f1);
	fclose(f2);
	return smprintf("%d",100*a/b);
}

int
main(void)
{
	char *status;
	char *avgs;
	char *tmar;
	char *swapmem;
	char *temp;
	char *battery;
	//char *tmutc;
	//char *tmbln;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}
	for (;;sleep(1)) {
		avgs = loadavg();
		tmar = mktimes("%D %H:%M:%S", tzmoscow);
		swapmem = getmemory();
		temp = temperature();
		battery = power();
		//tmutc = mktimes("%H:%M", tzutc);
		//tmbln = mktimes("KW %W %a %d %b %H:%M %Z %Y", tzberlin);

		status = smprintf("%s | loadavg:%s | температура %s | Батарея:%s | %s",
				swapmem, avgs, temp, battery, tmar);
		setstatus(status);
		free(avgs);
		free(tmar);
		free(temp);
		free(swapmem);
		free(battery);
		//free(tmutc);
		//free(tmbln);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

