/* $nsh $ */
/*
 * Copyright (c) 2004
 *      Chris Cappuccio.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/pfvar.h>
#include <net/if_pfsync.h>
#include "externs.h"

int
intsyncif(char *ifname, int ifs, int argc, char **argv)
{
	struct ifreq ifr;
	struct pfsyncreq preq;
	int set;

	if (NO_ARG(argv[0])) {
		set = 0;
		argc--;
		argv++;
	} else
		set = 1;

	argc--;
	argv++;

	if ((!set && argc > 1) || (set && argc != 1)) {
		printf("%% syncif <if>\n");
		printf("%% no syncif [if]\n");
		return (0);
	}
	bzero((char *) &preq, sizeof(struct pfsyncreq));
	ifr.ifr_data = (caddr_t) & preq;
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(ifs, SIOCGETPFSYNC, (caddr_t) & ifr) == -1) {
		printf("%% intsyncif: SIOCGETPFSYNC: %s\n", strerror(errno));
		return (0);
	}

	if (argv[0])
		if (!is_valid_ifname(argv[0])) {
			printf("%% Interface not found: %s\n", argv[0]);
			return (0);
		}

	if (set)
		strlcpy(preq.pfsyncr_syncif, argv[0],
			sizeof(preq.pfsyncr_syncif));
	else
		bzero((char *) &preq.pfsyncr_syncif,
		      sizeof(preq.pfsyncr_syncif));

	if (ioctl(ifs, SIOCSETPFSYNC, (caddr_t) & ifr) == -1) {
		if (errno == ENOBUFS)
			printf("%% Invalid synchronization interface: %s\n",
			    argv[0]);
		else
			printf("%% intsyncif: SIOCSETPFSYNC: %s\n",
			    strerror(errno));
	}
	return (0);
}

int
intmaxupd(char *ifname, int ifs, int argc, char **argv)
{
	struct ifreq ifr;
	struct pfsyncreq preq;
	int set;
	char *ep;

	if (NO_ARG(argv[0])) {
		set = 0;
		argc--;
		argv++;
	} else
		set = 1;

	argc--;
	argv++;

	if ((!set && argc > 1) || (set && argc != 1)) {
		printf("%% maxupd <max pfsync updates>\n");
		printf("%% no maxupd [max pfsync updates]\n");
		return (0);
	}
	bzero((char *) &preq, sizeof(struct pfsyncreq));
	ifr.ifr_data = (caddr_t) & preq;
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(ifs, SIOCGETPFSYNC, (caddr_t) & ifr) == -1) {
		printf("%% intmaxupd: SIOCGETPFSYNC: %s\n", strerror(errno));
		return (0);
	}
	if (set) {
		preq.pfsyncr_maxupdates = strtoul(argv[0], &ep, 10);
		if (!ep || *ep) {
			printf("%% maxupd value out of range\n");
			return (0);
		}
	} else
		preq.pfsyncr_maxupdates = PFSYNC_MAXUPDATES;

	if (ioctl(ifs, SIOCSETPFSYNC, (caddr_t) & ifr) == -1) {
		if (errno == EINVAL)
			printf("%% maxupd value out of range\n");
		else
			printf("%% intmaxupd: SIOCSETPFSYNC: %s\n",
			    strerror(errno));
	}
	return (0);
}

int
conf_pfsync(FILE *output, int s, char *ifname)
{
	struct ifreq ifr;
	struct pfsyncreq preq;

	bzero((char *) &preq, sizeof(struct pfsyncreq));
	ifr.ifr_data = (caddr_t) & preq;
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGETPFSYNC, (caddr_t) & ifr) == -1)
		return (0);

	if (preq.pfsyncr_syncif[0] != '\0') {
		fprintf(output, " syncif %s\n", preq.pfsyncr_syncif);
		if (preq.pfsyncr_maxupdates != PFSYNC_MAXUPDATES)
			fprintf(output, " maxupd %i\n",
			    preq.pfsyncr_maxupdates);
	}
	return (0);
}
