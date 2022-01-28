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
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
	} while (0)
#else
// Only EXIT in DEBUG MODE
#define nftp_fatal(format, ...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
		exit(0);                                                      \
	} while (0)
#endif
#else
#ifndef DEBUG
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
	} while (0)
#else
// Only EXIT in DEBUG MODE
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
		exit(0);                                                      \
	} while (0)
#endif
#endif

#ifdef _WIN32
#define nftp_log(format, ...)                                           \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, __VA_ARGS__)
#else
#define nftp_log(format, arg...)                                           \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, ##arg)
#endif

#endif
