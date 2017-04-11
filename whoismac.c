#define _GNU_SOURCE
#include <gcrypt.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <libgen.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <curl/curl.h>

#include "common.h"

#define LINEBUFFER 256

/*===========================================================================*/
int downloadoui(const char *ouiname)
{
CURLcode ret;
CURL *hnd;

FILE* fhoui;

printf("start downloading to: %s\n", ouiname);

if((fhoui = fopen(ouiname, "w")) == NULL)
	{
	fprintf(stderr, "error creating file %s", ouiname);
	exit(EXIT_FAILURE);
	}

hnd = curl_easy_init ();
curl_easy_setopt(hnd, CURLOPT_URL, "http://standards-oui.ieee.org/oui.txt");
curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.35.0");
curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
curl_easy_setopt(hnd, CURLOPT_WRITEDATA, fhoui) ;

ret = curl_easy_perform(hnd);
curl_easy_cleanup(hnd);
fclose(fhoui);
if(ret != 0)
	{
	fprintf(stderr, "download not successfull");
	exit(EXIT_FAILURE);
	}
		
printf("download finished\n");
return TRUE;	
}
/*===========================================================================*/
size_t chop(char *buffer,  size_t len)
{
char *ptr = buffer +len -1;

while (len) {
	if (*ptr != '\n') break;
	*ptr-- = 0;
	len--;
	}

while (len) {
	if (*ptr != '\r') break;
	*ptr-- = 0;
	len--;
	}
return len;
}
/*---------------------------------------------------------------------------*/
int fgetline(FILE *inputstream, size_t size, char *buffer)
{
if (feof(inputstream)) return -1;
		char *buffptr = fgets (buffer, size, inputstream);

	if (buffptr == NULL) return -1;

	size_t len = strlen(buffptr);
	len = chop(buffptr, len);

return len;
}
/*===========================================================================*/
void getoui(const char *ouiname, unsigned long long int oui)
{
int len;

FILE* fhoui;
char *vendorptr;
unsigned long long int vendoroui;

char linein[LINEBUFFER];


if ((fhoui = fopen(ouiname, "r")) == NULL)
	{
	fprintf(stderr, "unable to open database %s\n", ouiname);
	exit (EXIT_FAILURE);
	}

while((len = fgetline(fhoui, LINEBUFFER, linein)) != -1)
	{
	if (len < 10)
		continue;

	if(strstr(linein, "(base 16)") != NULL)
		{
		sscanf(linein, "%06llx", &vendoroui);
		if(oui == vendoroui)
			{
			vendorptr = strrchr(linein, '\t');
			fprintf(stdout, "%06llx%s\n", vendoroui, vendorptr);
			}
		}
	}
fclose(fhoui);
return;	
}
/*===========================================================================*/
void getvendor(const char *ouiname, char *vendorstring)
{
int len;

FILE* fhoui;
char *vendorptr;
unsigned long long int vendoroui;

char linein[LINEBUFFER];


if ((fhoui = fopen(ouiname, "r")) == NULL)
	{
	fprintf(stderr, "unable to open database %s\n", ouiname);
	exit (EXIT_FAILURE);
	}

while((len = fgetline(fhoui, LINEBUFFER, linein)) != -1)
	{
	if (len < 10)
		continue;

	if(strstr(linein, "(base 16)") != NULL)
		{
		if(strstr(linein, vendorstring) != NULL)
			{
			sscanf(linein, "%06llx", &vendoroui);
			vendorptr = strrchr(linein, '\t');
			fprintf(stdout, "%06llx%s\n", vendoroui, vendorptr);
			}
		}
	}
fclose(fhoui);
return;	
}
/*===========================================================================*/
static void usage(char *eigenname)
{
printf("%s %s (C) %s ZeroBeat\n"
	"usage: %s <options>\n"
	"\n"
	"options:\n"
	"-d       : download http://standards-oui.ieee.org/oui.txt\n"
	"         : and save to ~.hcxtools/oui.txt\n"
	"         : internet connection required\n"
	"-m <oui> : oui (fist three bytes of mac addr)\n"
	"-v       : vendor name\n"
	"-h       : this help screen\n"
	"\n", eigenname, VERSION, VERSION_JAHR, eigenname);
exit(EXIT_SUCCESS);
}
/*===========================================================================*/
int main(int argc, char *argv[])
{
int auswahl;
int mode = 0;
unsigned long long int oui = 0;

uid_t uid;
struct passwd *pwd;
struct stat statinfo;
char *eigenpfadname, *eigenname;
char *vendorname = NULL;
const char confdirname[] = ".hcxtools";
const char ouiname[] = ".hcxtools/oui.txt";

eigenpfadname = strdupa(argv[0]);
eigenname = basename(eigenpfadname);


while ((auswahl = getopt(argc, argv, "m:v:dh")) != -1)
	{
	switch (auswahl)
		{
		case 'd':
		mode = 'd';
		break;
		
		case 'm':
		if(strlen(optarg) != 6)
			{
			fprintf(stderr, "error wrong oui size %s (need 1122aa)\n", optarg);
			exit(EXIT_FAILURE);
			}
		oui = strtoul(optarg, NULL, 16);
		mode = 'm';
		break;

		case 'v':
		vendorname = optarg;;
		mode = 'v';
		break;

		case 'h':
		usage(eigenname);
		break;

		default:
		usage(eigenname);
		break;
		}
	}


uid = getuid();
pwd = getpwuid(uid);
if (pwd == NULL)
	{
	fprintf(stdout, "failed to get home dir\n");
	exit(EXIT_FAILURE);
	}
chdir(pwd->pw_dir);

if(stat(confdirname, &statinfo) == -1)
	{
	if (mkdir(confdirname,0755) == -1)
		{
		fprintf(stdout, "failed to create conf dir\n");
		exit(EXIT_FAILURE);
		}
	}


if(mode == 'd')
	downloadoui(ouiname);

if(stat(ouiname, &statinfo) != 0)
	{
	fprintf(stderr, "can't stat %s\n"
			"use download option -d to download it\n"
			"or download file http://standards-oui.ieee.org/oui.txt\n"
			"and save it to ~./hcxtools/oui.txt\n", ouiname);
	exit(EXIT_FAILURE);
	}

if(mode == 'm')
	getoui(ouiname, oui);

if(mode == 'v')
	getvendor(ouiname, vendorname);


return EXIT_SUCCESS;
}