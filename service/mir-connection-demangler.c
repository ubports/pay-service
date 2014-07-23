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
#include "proxy-payui.h"

int
main (int argc, char * argv[])
{
	const gchar * mir_socket = g_getenv("PAY_SERVICE_MIR_SOCKET");
	if (mir_socket == NULL || mir_socket[0] == '\0') {
		g_error("Unable to find Mir connection from Pay Service");
		return -1;
	}

	g_print("Mir Connection Path: %s\n", mir_socket);

	proxyPayPayui * proxy = proxy_pay_payui_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
		"com.canonical.pay",
		mir_socket,
		NULL, NULL);
	
	if (proxy == NULL) {
		g_error("Unable to connect to proxy");
		return -1;
	}

	GVariant * outhandle = NULL;
	proxy_pay_payui_call_get_mir_socket_sync(proxy, &outhandle, NULL, NULL);

	if (outhandle == NULL) {
		g_error("Unable to get data from function");
		return -1;
	}

	gint32 fd = g_variant_get_handle(outhandle);
	gchar * mirsocketbuf = g_strdup_printf("fd://%d", fd);
	setenv("MIR_SOCKET", mirsocketbuf, 1);

	g_free(mirsocketbuf);
	g_variant_unref(outhandle);
	g_object_unref(proxy);

	g_print("Setting MIR_SOCKET to: '%s'\n", mirsocketbuf);

	/* Thought, is argv NULL terminated? */
	return execv(argv[1], argv + 1);
}
