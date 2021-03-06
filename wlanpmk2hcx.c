#define _GNU_SOURCE
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio_ext.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <math.h>
#include "common.c"

/*===========================================================================*/
void base64(const unsigned char* buffer, size_t length, char** b64text)
{
BIO *bio, *b64;
BUF_MEM *bufferPtr;

b64 = BIO_new(BIO_f_base64());
bio = BIO_new(BIO_s_mem());
bio = BIO_push(b64, bio);

BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
BIO_write(bio, buffer, length);
BIO_flush(bio);
BIO_get_mem_ptr(bio, &bufferPtr);
BIO_set_close(bio, BIO_NOCLOSE);
BIO_free_all(bio);
*b64text=(*bufferPtr).data;
return;
}
/*===========================================================================*/
bool hex2bin(const char *str, uint8_t *bytes, size_t blen)
{
size_t c;
uint8_t pos;
uint8_t idx0;
uint8_t idx1;

const uint8_t hashmap[] =
{
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // PQRSTUVW
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // XYZ[\]^_
0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // `abcdefg
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // hijklmno
};

for(c = 0; c < blen; c++)
	{
	if(str[c] < '0')
		return false;
	if(str[c] > 'f')
		return false;
	if((str[c] > '9') && (str[c] < 'A'))
		return false;
	if((str[c] > 'F') && (str[c] < 'a'))
		return false;
	}

bzero(bytes, blen);
for (pos = 0; ((pos < (blen*2)) && (pos < strlen(str))); pos += 2)
	{
	idx0 = ((uint8_t)str[pos+0] & 0x1F) ^ 0x10;
	idx1 = ((uint8_t)str[pos+1] & 0x1F) ^ 0x10;
	bytes[pos/2] = (uint8_t)(hashmap[idx0] << 4) | hashmap[idx1];
	};
return true;
}
/*===========================================================================*/
size_t chop(char *buffer, size_t len)
{
char *ptr = buffer +len -1;

while(len)
	{
	if (*ptr != '\n')
		break;
	*ptr-- = 0;
	len--;
	}

while(len)
	{
	if (*ptr != '\r')
		break;
	*ptr-- = 0;
	len--;
	}
return len;
}
/*---------------------------------------------------------------------------*/
int fgetline(FILE *inputstream, size_t size, char *buffer)
{
if(feof(inputstream))
	return -1;
char *buffptr = fgets (buffer, size, inputstream);

if(buffptr == NULL)
	return -1;

size_t len = strlen(buffptr);
len = chop(buffptr, len);
return len;
}

/*===========================================================================*/
void outputhashlist(FILE *fhcombi, FILE *fhhash)
{
int combilen;
int essidlen;
long int hashcount = 0;
long int skippedcount = 0;
char *essidname = NULL;
char *hashrecord = NULL;
char combiline[100];
unsigned char pmkstr[64];
unsigned char essidstr[64];

while((combilen = fgetline(fhcombi, 100, combiline)) != -1)
	{
	if(combilen < 66)
		{
		skippedcount++;
		continue;
		}
	if(combiline[64] != ':')
		{
		skippedcount++;
		continue;
		}

	if(hex2bin(combiline, pmkstr, 64) != true)
		{
		skippedcount++;
		continue;
		}
	essidname = strchr(combiline, ':') +1;
	if(essidname == NULL)
		{
		skippedcount++;
		continue;
		}
	essidlen = strlen(essidname);
	if((essidlen < 1) || (essidlen > 32))
		{
		skippedcount++;
		continue;
		}

	memset(&essidstr, 0, 64);
	memcpy(&essidstr, essidname, essidlen);
	base64(essidstr, essidlen, &hashrecord);
	fprintf(fhhash, "sha1:4096:%s:", hashrecord);
	free(hashrecord);
	base64(pmkstr, 32, &hashrecord);
	fprintf(fhhash, "%s\n\n", hashrecord);
	free(hashrecord);
	hashcount++;
	}
printf("\r%ld hashrecords generated, %ld password(s) skipped\n", hashcount, skippedcount);
return;
}
/*===========================================================================*/
void outputsinglehash(char *pmkname, char *essidname, int essidlen)
{
char *hashrecord = NULL;
unsigned char essidstr[64];
unsigned char pmkstr[64];

if(hex2bin(pmkname, pmkstr, 64) != true)
	{
	fprintf(stderr, "error wrong plainmasterkey\n");
	return;
	}

printf("\nuse hashcat hash-mode -m 12000 to get password\n");
memset(&essidstr, 0, 64);
memcpy(&essidstr, essidname, essidlen);
base64(essidstr, essidlen, &hashrecord);
printf("sha1:4096:%s:", hashrecord);
free(hashrecord);
base64(pmkstr, 32, &hashrecord);
printf("%s\n\n", hashrecord);
free(hashrecord);
return;
}
/*===========================================================================*/
static void usage(char *eigenname)
{
printf("%s %s (C) %s ZeroBeat\n"
	"usage: %s <options>\n"
	"\n"
	"options:\n"
	"-i <file>  : input combilist (pmk:essid)\n"
 	"-o <file>  : output hashfile (use hashcat -m 12000)\n"
	"-e <essid> : input single essid (networkname: 1 .. 32 characters)\n"
	"-p <pmk>   : input plainmasterkey (64 xdigits)\n"
	"-h         : this help\n"
	"\n", eigenname, VERSION, VERSION_JAHR, eigenname);
exit(EXIT_FAILURE);
}
/*===========================================================================*/
int main(int argc, char *argv[])
{
int auswahl;
int p;
int essidlen = 0;
int pmklen = 0;
char *eigenname = NULL;
char *eigenpfadname = NULL;
char *pmkname = NULL;
char *essidname = NULL;

FILE *fhcombi = NULL;
FILE *fhhash = NULL;


eigenpfadname = strdupa(argv[0]);
eigenname = basename(eigenpfadname);

setbuf(stdout, NULL);
while ((auswahl = getopt(argc, argv, "i:o:e:p:h")) != -1)
	{
	switch (auswahl)
		{
		case 'i':
		if((fhcombi = fopen(optarg, "r")) == NULL)
			{
			fprintf(stderr, "error opening %s\n", optarg);
			exit(EXIT_FAILURE);
			}
		break;

		case 'o':
		if((fhhash = fopen(optarg, "a")) == NULL)
			{
			fprintf(stderr, "error opening %s\n", optarg);
			exit(EXIT_FAILURE);
			}
		break;

		case 'e':
		essidname = optarg;
		essidlen = strlen(essidname);
		if((essidlen < 1) || (essidlen > 32))
			{
			fprintf(stderr, "error wrong essid len (allowed: 1 .. 32 characters)\n");
			exit(EXIT_FAILURE);
			}
		break;

		case 'p':
		pmkname = optarg;
		pmklen = strlen(pmkname);
		if(pmklen != 64)
			{
			fprintf(stderr, "error wrong plainmasterkey len (allowed: 64 xdigits)\n");
			exit(EXIT_FAILURE);
			}
		for(p = 0; p < 64; p++)
			{
			if(!(isxdigit(pmkname[p])))
				{
				fprintf(stderr, "error wrong plainmasterkey len (allowed: 64 xdigits)\n");
				exit(EXIT_FAILURE);
				}
			}
 		break;

		case 'h':
		usage(eigenname);
		break;

		default:
		usage(eigenname);
		break;
		}
	}

if((essidname != NULL) && (pmkname != NULL))
	outputsinglehash(pmkname, essidname, essidlen);

else if((fhcombi != NULL) && (fhhash != NULL))
	outputhashlist(fhcombi, fhhash);

if(fhcombi != NULL)
	fclose(fhcombi);

if(fhhash != NULL)
	fclose(fhhash);


return EXIT_SUCCESS;
}
