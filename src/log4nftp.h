// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//
// This is a Customized File Transfer Protocol nftp.
//

#ifndef NANO_FTP_DEBUG_H
#define NANO_FTP_DEBUG_H

#include <stdio.h>

#ifdef _WIN32
#ifndef DEBUG
#define nftp_fatal(format, ...)                                                 \
	do {                                                                  \
		fprintf(stderr, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
	} while (0)
#else //DEBUG
#ifdef LOGTOFILE
#define nftp_fatal(format, ...)                                                 \
	do {                                                                  \
		FILE *fd = fopen(LOGTOFILE, "w"); \
		fprintf(fd, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
		fclose(fd); \
	} while (0)
#else // LOGTOFILE
	do {                                                                  \
		fprintf(stderr, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
	} while (0)
#endif // LOGTOFILE
#endif // DEBUG
#else // LINUX
#ifndef DEBUG
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
	} while (0)
#else // DEBUG
#ifdef LOGTOFILE
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		FILE *fd = fopen(LOGTOFILE, "w"); \
		fprintf(fd, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
		fclose(fd); \
	} while (0)
#else // LOGTOFILE
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "ERR %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
	} while (0)
#endif // LOGTOFILE
#endif // DEBUG
#endif // WIN32

#ifdef _WIN32
#ifdef DEBUG
#ifdef LOGTOFILE
#define nftp_log(format, ...)                                           \
	do { \
		FILE *fd = fopen(LOGTOFILE, "w"); \
		fprintf(fd, "INFO %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
		fclose(fd); \
	} while (0)
#else // LOGTOFILE
#define nftp_log(format, ...)                                           \
	do { \
		fprintf(stderr, "INFO %s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    	__FUNCTION__, __VA_ARGS__); \
	} while (0)
#endif // LOGTOFILE

#else // DEBUG

#define nftp_log(format, ...)                                           \
	do { \
	} while (0)
#endif // DEBUG

#else // Linux

#ifdef DEBUG
#ifdef LOGTOFILE
#define nftp_log(format, arg...)                                           \
	do { \
		FILE *fd = fopen(LOGTOFILE, "w"); \
		fprintf(fd, "INFO %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
		fclose(fd); \
	} while (0)
#else // LOGTOFILE
#define nftp_log(format, arg...)                                           \
	do { \
		fprintf(stderr, "INFO %s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg); \
	} while (0)
#endif // LOGTOFILE

#else // DEBUG

#define nftp_log(format, arg...)                                           \
	do { \
	} while (0)
#endif // DEBUG
#endif // WIN32

#endif
