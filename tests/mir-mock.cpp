
#include "mir-mock.h"

#include <iostream>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

MirPromptSession *
mir_connection_create_prompt_session_sync(MirConnection * connection, pid_t pid, void (*)(MirPromptSession *, MirPromptSessionState, void*data), void * context) {
	return nullptr;
}

void
mir_prompt_session_release_sync (MirPromptSession * session)
{
}

MirWaitHandle *
mir_prompt_session_new_fds_for_promt_providers (MirPromptSession * session, unsigned int numfds, mir_client_fd_callback cb, void * data) {
	return nullptr;
}

void
mir_wait_for (MirWaitHandle * wait)
{

}

static const char * valid_connection_str = "Valid Mir Connection";
static std::pair<std::string, std::string> last_connection;
static bool valid_connection = true;

void
mir_mock_connect_return_valid (bool valid)
{
	valid_connection = valid;
}

std::pair<std::string, std::string>
mir_mock_connect_last_connect (void)
{
	return last_connection;
}

MirConnection *
mir_connect_sync (char const * server, char const * appname)
{
	last_connection = std::pair<std::string, std::string>(server, appname);

	if (valid_connection) {
		return (MirConnection *)(valid_connection_str);
	} else {
		return nullptr;
	}
}

void
mir_connection_release (MirConnection * con)
{
	if (reinterpret_cast<char *>(con) != valid_connection_str) {
		std::cerr << "Releasing a Mir Connection that isn't valid" << std::endl;
		exit(1);
	}
}
