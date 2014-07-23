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

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

int
main (int argc, char * argv[])
{
	const gchar * mir_socket = g_getenv("PAY_SERVICE_MIR_SOCKET");
	if (mir_socket == NULL || mir_socket[0] == '\0') {
		g_error("Unable to find Mir connection from Pay Service");
		return -1;
	}

	g_print("Mir Connection Path: %s\n", mir_socket);

	GError * error = NULL;
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (error != NULL) {
		g_error("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return -1;
	}

	GVariant * retval;
	GUnixFDList * fdlist;

	retval = g_dbus_connection_call_with_unix_fd_list_sync(
		bus,
		"com.canonical.pay",
		mir_socket,
		"com.canonical.pay.payui",
		"GetMirSocket",
		NULL,
		G_VARIANT_TYPE("(h)"),
		G_DBUS_CALL_FLAGS_NO_AUTO_START,
		-1, /* timeout */
		NULL, /* fd list in */
		&fdlist,
		NULL, /* cancelable */
		&error);

	if (error != NULL) {
		g_error("Unable to get Mir socket over dbus: %s", error->message);
		g_error_free(error);
		return -1;
	}

	GVariant * outhandle = g_variant_get_child_value(retval, 0);

	if (outhandle == NULL) {
		g_error("Unable to get data from function");
		return -1;
	}

	gint32 fd = g_unix_fd_list_get(fdlist, g_variant_get_handle(outhandle), &error);
	if (error != NULL) {
		g_error("Unable to Unix FD: %s", error->message);
		g_error_free(error);
		return -1;
	}

	gchar * mirsocketbuf = g_strdup_printf("fd://%d", fd);
	setenv("MIR_SOCKET", mirsocketbuf, 1);
	g_print("Setting MIR_SOCKET to: '%s'\n", mirsocketbuf);

	g_free(mirsocketbuf);
	g_variant_unref(outhandle);
	g_variant_unref(retval);
	g_object_unref(bus);

	/* Thought, is argv NULL terminated? */
	return execvp(argv[1], argv + 1);
}
