/* Copyright (C) 2016 Canonical Ltd.
 *
 * This file is part of go-mir.
 *
 * go-mir is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-mir is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-mir. If not, see <http://www.gnu.org/licenses/>.
 */

#include "_cgo_export.h"
#include "mir_prompt_session_helper.h"

static void _fd_getter (MirPromptSession * session, size_t count,
                 int const * fdsin, void * pfds)
{
    if (count != 1) {
        g_warning("Didn't get the right number of FDs");
        return;
    }

    int * fds = (int *)pfds;
    fds[0] = fdsin[0];
}

int mir_prompt_session_get_fd(MirPromptSession * session)
{
    int fd = 0;
    MirWaitHandle * wait = mir_prompt_session_new_fds_for_prompt_providers(session, 1, _fd_getter, &fd);

    mir_wait_for(wait);

    return fd;
}
