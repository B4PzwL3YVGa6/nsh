/* From: $OpenBSD: ifconfig.c,v 1.61 2002/04/10 18:52:27 millert Exp $ */
/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_ieee80211.h>
#include "externs.h"

const char *
get_string(const char *val, const char *sep, u_int8_t *buf, int *lenp)
{
	int len;
	int hexstr;
	u_int8_t *p;

	len = *lenp;
	p = buf;
	hexstr = (val[0] == '0' && tolower((u_char) val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char) val[0]) ||
			    !isxdigit((u_char) val[1])) {
				printf("%% get_string: bad hexadecimal digits\n");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				printf("%% get_string: hexadecimal digits too long\n");
			else
				printf("%% get_string: strings too long\n");
			return NULL;
		}
		if (hexstr) {
#define tohex(x)        (isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char) val[0]) << 4) |
				tohex((u_char) val[1]);
#undef tohex
			val += 2;
		} else {
			if (*val == '\\' &&
			    sep != NULL && strchr(sep, *(val + 1)) != NULL)
				val++;
			*p++ = *val++;
		}
	}
	len = p - buf;
	if (len < *lenp)
		memset(p, 0, *lenp - len);
	*lenp = len;
	return val;
}

/* was print_string() */
void
make_string(char *str, int str_len, const u_int8_t *buf, int buf_len)
{
	int i;
	int hasspc;
	char *tmp;

	str_len--;
	i = 0;
	hasspc = 0;
	if (buf_len < 2 || buf[0] != '0' || tolower(buf[1]) != 'x') {
		for (; i < buf_len; i++) {
			if (!isprint(buf[i]))
				break;
			if (isspace(buf[i]))
				hasspc++;
		}
	}
	if (i == buf_len) {
		if (hasspc || buf_len == 0)
			snprintf(str, str_len, "\"%.*s\"", buf_len, buf);
		else
			snprintf(str, str_len, "%.*s", buf_len, buf);
	} else {
		snprintf(str, str_len, "0x");
		for (i = 0; i < buf_len; i++) {
			snprintf(tmp, sizeof(tmp), "%02x", buf[i]);
			strlcat(str, tmp, str_len);
		}
	}
}

/* was setifnwkey() */
int
intnwkey(const char *ifname, int argc, char **argv)
{
	int i, len, ifs, set;
	char *cp = NULL, *val;
	struct ieee80211_nwkey nwkey;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];

	if (strncasecmp(argv[0], "no", strlen("no")) == 0) {
		set = 0;
		argc--;
		argv++;
	} else
		set = 1;
	argc--;
	argv++;

	if((!set && argc != 0) || (set && argc != 1)) {
		printf("%% nwkey <key>\n");
		printf("%% nwkey persist\n");
		printf("%% nwkey persist:<key>\n");
		printf("%% nwkey <n>:<k1>,<k2>,<k3>,<k4>\n");
		printf("%% no nwkey\n");
		return(0);
	}
	if(set)
		val = argv[0];

	nwkey.i_wepon = IEEE80211_NWKEY_WEP;
	nwkey.i_defkid = 1;
	if (!set) {
		/* disable WEP encryption */
		nwkey.i_wepon = 0;
		i = 0;
	} else if (strcasecmp("persist", val) == 0) {
		/* use all values from persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		nwkey.i_defkid = 0;
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			nwkey.i_key[i].i_keylen = -1;
	} else if (strncasecmp("persist:", val, 8) == 0) {
		val += 8;
		/* program keys in persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		goto set_nwkey;
	} else {
set_nwkey:
		if (isdigit(val[0]) && val[1] == ':') {
			/* specifying a full set of four keys */
			nwkey.i_defkid = val[0] - '0';
			val += 2;
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				len = sizeof(keybuf[i]);
				val = (char *)get_string(val, ",", keybuf[i], &len);
				if (val == NULL)
					return;
				nwkey.i_key[i].i_keylen = len;
				nwkey.i_key[i].i_keydat = keybuf[i];
			}
			if (cp != NULL) {
				printf("%% intnwkey: too many keys\n");
				return;
			}
		} else {
			len = sizeof(keybuf[i]);
			val = (char *)get_string(val, NULL, keybuf[0], &len);
			if (val == NULL)
				return;
			nwkey.i_key[0].i_keylen = len;
			nwkey.i_key[0].i_keydat = keybuf[0];
			i = 1;
		}
	}
	/* zero out any unset keys */
	for (; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keylen = 0;
		nwkey.i_key[i].i_keydat = NULL;
	}
	(void) strlcpy(nwkey.i_name, ifname, sizeof(nwkey.i_name));
	ifs = socket(AF_INET, SOCK_DGRAM, 0);
	if (ifs < 0) {
		perror("% intnwkey: socket");
		return(1);
	}
	if (ioctl(ifs, SIOCS80211NWKEY, (caddr_t)&nwkey) == -1)
		perror("% intnwkey: SIOCS80211NWKEY");
	close(ifs);
	return(0);
}

/*
 * mangled ieee80211_status()
 */
int
get_nwinfo(const char *ifname, char *str, int str_len, int type)
{
	char *tmp;
	int ifs, len, i, nwkey_verbose;

	ifs = socket(AF_INET, SOCK_DGRAM, 0);
	if (ifs < 0) {
		perror("% get_nwinfo: socket");
		return(NULL);
        }

	memset(str, 0, str_len);

	if (type == NWID) {
		struct ieee80211_nwid nwid;
		struct ifreq ifr;

		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_data = (caddr_t) &nwid;
		(void) strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		if (ioctl(ifs, SIOCG80211NWID, (caddr_t) & ifr) == 0) {
			/* nwid.i_nwid is not NUL terminated. */
			len = nwid.i_len;
			if (len > IEEE80211_NWID_LEN)
				len = IEEE80211_NWID_LEN;
			make_string(str, str_len, nwid.i_nwid, len);
		}
	} else if (type == NWKEY) {
		struct ieee80211_nwkey nwkey;
		u_int8_t keybuf[IEEE80211_WEP_NKID][16];

		memset(&nwkey, 0, sizeof(nwkey));
		(void) strlcpy(nwkey.i_name, ifname, sizeof(nwkey.i_name));
		if (ioctl(ifs, SIOCG80211NWKEY, (caddr_t) & nwkey) == 0 &&
		    nwkey.i_wepon > 0) {
			/* try to retrieve WEP keys */
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				nwkey.i_key[i].i_keydat = keybuf[i];
				nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
			}
			if (ioctl(ifs, SIOCG80211NWKEY, (caddr_t) & nwkey)
			    == -1) {
				strlcat(str, str_len, "*****");
			} else {
				nwkey_verbose = 0;
				/*
				 * check to see non default key or multiple keys
				 * defined
				 */
				if (nwkey.i_defkid != 1) {
					nwkey_verbose = 1;
				} else {
					for (i = 1; i < IEEE80211_WEP_NKID; i++)
					{
						if (nwkey.i_key[i].i_keylen !=
						    0) {
							nwkey_verbose = 1;
							break;
						}
					}
				}
				/* check extra ambiguity with keywords */
				if (!nwkey_verbose) {
					if (nwkey.i_key[0].i_keylen >= 2 &&
					    isdigit(nwkey.i_key[0].i_keydat[0])
					    && nwkey.i_key[0].i_keydat[1] ==
					    ':')
						nwkey_verbose = 1;
					else if (nwkey.i_key[0].i_keylen >= 7 &&
						    strncasecmp("persist",
						    nwkey.i_key[0].i_keydat, 7)
						    == 0)
						nwkey_verbose = 1;
				}
				if (nwkey_verbose) {
					snprintf(tmp, sizeof(tmp), "%d:",
					    nwkey.i_defkid);
					strlcat(str, tmp, str_len);
				}
				for (i = 0; i < IEEE80211_WEP_NKID; i++) {
					if (i > 0)
						strlcat(str, ",", str_len);
					if (nwkey.i_key[i].i_keylen < 0) {
						strlcat(str, "persist",
						    str_len);
					} else {
						/*
						 * XXX - sanity check
						 * nwkey.i_key[i].i_keylen
						 */
						make_string(str, str_len,
						    nwkey.i_key[i].i_keydat,
						    nwkey.i_key[i].i_keylen);
					}
					if (!nwkey_verbose)
						break;
				}
			}
		}
	}

	close(ifs);
	return(strlen(str));
}
