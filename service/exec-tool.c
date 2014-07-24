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
#include <click.h>
#include <ubuntu-app-launch.h>

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

	if (!g_key_file_has_key(keyfile, "Desktop Entry", "Exec", NULL)) {
		g_error("Desktop file does not have 'Exec' key");
		g_key_file_free(keyfile);
		return NULL;
	}

	gchar * exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);
	g_key_file_free(keyfile);

	gchar * prepend = g_strdup_printf("%s %s", SOCKET_TOOL, exec);
	g_free(exec);
	g_debug("Final Exec line: %s", prepend);

	gchar * envvar = g_strdup_printf("APP_EXEC=%s", prepend);
	g_free(prepend);

	return envvar;
}

gboolean
build_uri_envvar(const gchar * appuris, gchar ** euri, gchar ** esocket)
{
	gint argc;
	gchar ** argv;
	GError * error = NULL;

	g_shell_parse_argv(appuris, &argc, &argv, &error);
	if (error != NULL) {
		g_critical("Unable to parse URIs '%s': %s", appuris, error->message);
		g_error_free(error);
		return FALSE;
	}

	if (argc != 2) {
		g_critical("We should be getting 2 entries from '%s' but got %d", appuris, argc);
		g_strfreev(argv);
		return FALSE;
	}

	*esocket = g_strdup_printf("PAY_SERVICE_MIR_SOCKET=%s", argv[0]);
	gchar * quoted = g_shell_quote(argv[1]);
	*euri = g_strdup_printf("APP_URIS=%s", quoted);
	
	g_free(quoted);
	g_strfreev(argv);

	return TRUE;
}

gchar *
build_dir_envvar (const gchar * appid)
{
	GError * error = NULL;
	gchar * package = NULL;
	/* 'Parse' the App ID */
	if (!ubuntu_app_launch_app_id_parse(appid, &package, NULL, NULL)) {
		g_warning("Unable to parse App ID: '%s'", appid);
		return NULL;
	}

	/* Check click to find out where the files are */
	ClickDB * db = click_db_new();
	/* If TEST_CLICK_DB is unset, this reads the system database. */
	click_db_read(db, g_getenv("TEST_CLICK_DB"), &error);
	if (error != NULL) {
		g_warning("Unable to read Click database: %s", error->message);
		g_error_free(error);
		g_free(package);
		return NULL;
	}
	/* If TEST_CLICK_USER is unset, this uses the current user name. */
	ClickUser * user = click_user_new_for_user(db, g_getenv("TEST_CLICK_USER"), &error);
	if (error != NULL) {
		g_warning("Unable to read Click database: %s", error->message);
		g_error_free(error);
		g_free(package);
		g_object_unref(db);
		return NULL;
	}
	gchar * pkgdir = click_user_get_path(user, package, &error);

	g_object_unref(user);
	g_object_unref(db);
	g_free(package);

	if (error != NULL) {
		g_warning("Unable to get the Click package directory for %s: %s", package, error->message);
		g_error_free(error);
		return NULL;
	}

	gchar * envvar = g_strdup_printf("APP_DIR=%s", pkgdir);
	g_free(pkgdir);
	return envvar;
}

int
main (int argc, char * argv[])
{
	GError * error = NULL;

	/* Build up our exec */
	const gchar * appid = g_getenv("APP_ID");
	if (appid == NULL) {
		g_error("Environment variable 'APP_ID' required");
		return -1;
	}

	gchar * envexec = build_exec_envvar(appid);
	if (envexec == NULL) {
		return -1;
	}

	/* Build up our socket name URL */
	const gchar * appuris = g_getenv("APP_URIS");
	if (appuris == NULL) {
		g_error("Environment variable 'APP_URIS' required");
		g_free(envexec);
		return -1;
	}

	gchar * envuri = NULL;
	gchar * envsocket = NULL;

	if (!build_uri_envvar(appuris, &envuri, &envsocket)) {
		g_free(envexec);
		return -1;
	}

	gchar * envdir = build_dir_envvar(appid);
	/* envdir might be NULL if not a click */

	/* Execute the setting of the variables! */

	gchar * initctlargv[7] = {
		"initctl",
		"set-env",
		envexec,
		envuri,
		envsocket,
		envdir,
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
	g_free(envuri);
	g_free(envsocket);

	if (error == NULL) {
		return 0;
	} else {
		g_error("Unable to spawn 'initctl': %s", error->message);
		g_error_free(error);
		return -1;
	}
}
