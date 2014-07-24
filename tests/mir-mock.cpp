
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

MirConnection *
mir_connect_sync (char const * server, char const * appname)
{

}

void
mir_connection_release (MirConnection * con)
{

}
