// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <stdio.h>
#include <string.h>

#include "nftp.h"

#ifdef _WIN32
#define F_OK 0

inline int access(const char *pathname, int mode) {
	return _access(pathname, mode);
}
#else
#include <unistd.h>
#endif

int
nftp_file_exist(char *fname)
{
	return (access(fname, F_OK) == 0);
}

int
nftp_file_newname(char *fname, char **newnamep)
{
	char * newname;
	int len = strlen(fname);

	if ((newname = malloc(sizeof(char) * (len+3))) == NULL) {
		return (NFTP_ERR_MEM);
	}

	strcpy(newname, fname);
	newname[len] = '_';
	newname[len+2] = '\0';

	// Retry up to 10 times if filename unavailable
	for (int i = 1; i < 10; ++i) {
		newname[len+1] = '0' + i;
		if (0 == nftp_file_exist(newname)) {
			*newnamep = newname;
			return (0);
		}
	}

	*newnamep = NULL;
	return (NFTP_ERR_FILENAME);
}

int
nftp_file_rename(char *sfname, char *dfname)
{
	return rename(sfname, dfname);
}

int
nftp_file_size(char *fname, size_t *sz)
{
	FILE * fp;
	size_t filesize;

	if ((fp = fopen(fname, "r")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	*sz = filesize;
	return (0);
}

int
nftp_file_blocks(char *fname, size_t *blocks)
{
	int rv;
	size_t sz;

	if (0 != (rv = nftp_file_size(fname, &sz))) {
		return rv;
	}

	*blocks = sz/NFTP_BLOCK_SZ + 1;
	return (0);
}

int
nftp_file_readblk(char *fname, int n, char **strp, size_t *sz)
{
	FILE * fp;
	char * str;
	size_t filesize;
	size_t blksz;

	if ((fp = fopen(fname, "r")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);

	if (n > filesize/NFTP_BLOCK_SZ) {
		return (NFTP_ERR_FILE);
	} else if (n == filesize/NFTP_BLOCK_SZ) {
		blksz = filesize - n*NFTP_BLOCK_SZ;
	} else {
		blksz = NFTP_BLOCK_SZ;
	}

	fseek(fp, n*NFTP_BLOCK_SZ, SEEK_SET);

	str = (char *) malloc(blksz + 1);
	memset(str, '\0', blksz + 1);

	if (blksz != fread(str, 1, blksz, fp)) {
		free(str);
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	*strp = str;
	*sz   = blksz;
	return (0);
}

int
nftp_file_read(char *fname, char **strp, size_t *sz)
{
	FILE * fp;
	char * str;
	char   txt[1000];
	size_t filesize;

	if ((fp = fopen(fname, "r")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	str = (char *) malloc(filesize + 1);
	memset(str, '\0', filesize + 1);

	while ((fgets(txt, 1000, fp)) != NULL) {
		strcat(str, txt);
	}

	fclose(fp);
	*strp = str;
	*sz   = filesize;
	return (0);
}

int
nftp_file_write(char * fname, char * str, size_t sz)
{
	FILE * fp;
	size_t filesize;

	if ((fp = fopen(fname, "w")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	filesize = fwrite(str, 1, sz, fp);
	if (filesize != sz) {
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return (0);
}

int
nftp_file_append(char * fname, char * str, size_t sz)
{
	FILE * fp;
	int    filesize;

	if ((fp = fopen(fname, "a")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	filesize = fwrite(str, 1, sz, fp);
	if (filesize != sz) {
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return 0;
}

int
nftp_file_clear(char * fname)
{
	FILE * fp;

	if ((fp = fopen(fname, "w")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return (0);
}

int
nftp_file_hash(char *fname, uint32_t *hashval)
{
	FILE *   fp;
	size_t   sz = 1000;
	char     txt[1000];
	int      pos;
	uint32_t res = 5381;

	if ((fp = fopen(fname, "r")) == NULL) {
		nftp_fatal("read error");
		return (NFTP_ERR_FILE);
	}

	while ((fgets(txt, sz, fp)) != NULL) {
		for (pos = 0; pos < strlen(txt); ++pos) {
			res = 33 * res ^ (uint8_t) txt[pos];
		}
	}

	fclose(fp);
	*hashval = res;
	return (0);
}

