/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef UNIX_PATH_MAX
/* Bugs me to no end that this isn't a define in sys/un.h */
#define UNIX_PATH_MAX 108
#endif

#define OVERRIDE_ENVIRONMENT 1

struct fdcmsghdr {
	struct cmsghdr hdr;
	int fd;
};

int
main (int argc, char * argv[])
{
	const char * mir_socket = getenv("PAY_SERVICE_MIR_SOCKET");
	if (mir_socket == NULL) {
		fprintf(stderr, "Unable to find Mir connection from Pay Service\n");
		return -1;
	}

	if (strlen(mir_socket) > UNIX_PATH_MAX - 2) { /* One for NULL on front, one on end */
		fprintf(stderr, "Environment variable 'PAY_SERVICE_MIR_SOCKET' is too long to be an abstract socket path: %s\n", mir_socket);
		return -1;
	}

	struct sockaddr_un socketaddr = {0};
	socketaddr.sun_family = AF_UNIX;
	memcpy(socketaddr.sun_path + 1, mir_socket, strlen(mir_socket) + 1);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == 0) {
		fprintf(stderr, "Unable to open a socket, at all.\n");
		return -1;
	}

	int conresult = connect(sock, (struct sockaddr *)&socketaddr, sizeof(struct sockaddr_un));
	if (conresult != 0) {
		close(sock);
		fprintf(stderr, "Unable to connect to address: %s\n", mir_socket);
		perror("Connect error");
		return -1;
	}

	struct fdcmsghdr fdhdr = {0};
	struct msghdr msg = {0};

	msg.msg_control = &fdhdr;
	msg.msg_controllen = sizeof(struct fdcmsghdr);

	int msgsize = recvmsg(sock, &msg, 0);

	close(sock);

	if (msgsize <= 0) {
		fprintf(stderr, "Not expecting %d message size", msgsize);
		return -1;
	}

	char mirsocketbuf[32];
	sprintf(mirsocketbuf, "fd://%d", fdhdr.fd);
	setenv("MIR_SOCKET", mirsocketbuf, OVERRIDE_ENVIRONMENT);

	/* Thought, is argv NULL terminated? */
	return execv(argv[1], argv + 1);
}
