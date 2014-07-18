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

#include <glib.h>

gchar *
build_exec_envvar (const gchar * appid)
{
	gchar * appid_desktop = g_strdup_printf("%s.desktop", appid);
	gchar * desktopfilepath = g_build_filename(g_get_user_cache_dir(), "pay-service", "pay-ui", appid_desktop, NULL);
	g_free(appid_desktop);

	if (!g_file_test(desktopfilepath, G_FILE_TEST_EXISTS)) {
		g_error("Can not file desktop file for '%s', expecting: '%s'", appid, desktopfilepath);
		g_free(desktopfilepath);
		return NULL;
	}

	GError * error = NULL;
	GKeyFile * keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, desktopfilepath, G_KEY_FILE_NONE, &error);

	if (error != NULL) {
		g_error("Unable to read pay-ui desktop file '%s': %s", desktopfilepath, error->message);
		g_free(desktopfilepath);
		g_key_file_free(keyfile);
		g_error_free(error);
		return NULL;
	}

	g_free(desktopfilepath);

	if (!g_key_file_has_key(keyfile, "Desktop File", "Exec", NULL)) {
		g_error("Desktop file does not have 'Exec' key");
		g_key_file_free(keyfile);
		return NULL;
	}

	gchar * exec = g_key_file_get_string(keyfile, "Desktop File", "Exec", NULL);
	g_key_file_free(keyfile);

	gchar * prepend = g_strdup_printf("%s %s", SOCKET_TOOL, exec);
	g_free(exec);
	g_debug("Final Exec line: %s", prepend);

	gchar * envvar = g_strdup_printf("APP_EXEC=%s", prepend);
	g_free(prepend);

	return envvar;
}

int
main (int argc, char * argv[])
{
	GError * error = NULL;

	const gchar * appid = g_getenv("APP_ID");
	if (appid == NULL) {
		g_error("Environment variable 'APP_ID' required");
		return -1;
	}

	gchar * envexec = build_exec_envvar(appid);

	gchar * initctlargv[4] = {
		"initctl",
		"set-env",
		envexec,
		NULL
	};

	g_spawn_sync(
		NULL, /* pwd */
		initctlargv,
		NULL, /* env */
		G_SPAWN_SEARCH_PATH,
		NULL, /* child setup */
		NULL, /* user data ^ */
		NULL, /* stdout */
		NULL, /* stderr */
		NULL, /* return code */
		&error
	);

	g_free(envexec);

	if (error == NULL) {
		return 0;
	} else {
		g_error("Unable to spawn 'initctl': %s", error->message);
		g_error_free(error);
		return -1;
	}
}
