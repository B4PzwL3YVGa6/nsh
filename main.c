/*
 * Copyright (c) 2002
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
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <histedit.h>
#include "externs.h"
#include "editing.h"

void usage(void);

char *vers = "20020521";
int verbose = 0;		/* verbose mode */
int editing;			/* command line editing enabled ? */

History *histi = NULL;
History *histc = NULL;
EditLine *elc = NULL;
EditLine *eli = NULL;
char *cursor_pos = NULL;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, iflag = 0;

	while ((ch = getopt(argc, argv, "iv")) != -1)
		switch (ch) {
		case 'i':
			iflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			break;
		}

	argc -= optind;
	argv += optind;

	printf("%% NSH v%s\n", vers);

	if (argc > 0)
		usage();

	/*
	 * For future kvm access
	 */
	load_nlist();

	if (iflag) {
		/*
		 * Run initialization and then exit.
		 */
		cmdrc("nshrc");
		exit(0);
	}

	for (;;) {
		command(1);
	}

	return 0;
}

void
usage()
{
	(void)fprintf(stderr, "usage: %s [-i]\n", __progname);
	exit(1);
}
