#!/bin/sh
#
# Copyright © 2015 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
set -x
set -e

for upstream in `cat vendor.cfg`; do
    if [ ! -d ${upstream} ]; then
        mkdir -p ${upstream}
    fi
    HOST="`echo ${upstream} | cut -d/ -f1`"
    CODE_TMP=`mktemp -d -u`
    pardir=`dirname ${upstream}`
    dirname=`basename ${upstream}`
    if [ "x${HOST}" = "xgithub.com" ]; then
        echo "Fetching from github..."
        GIT_DIR=`mktemp -d -u`
        git clone https://${upstream}.git ${CODE_TMP}/${dirname} --separate-git-dir=${GIT_DIR}
        rm -rf ${CODE_TMP}/${dirname}/.git
        rm -rf ${GIT_DIR}
    elif [ "x${HOST}" = "xlaunchpad.net" ]; then
        echo "Fetching from launchpad..."
        mkdir -p ${CODE_TMP}
        bzr export -d lp:${dirname} ${CODE_TMP}/${dirname}
    else
        echo "Unknown external source type: ${dirname}"
    fi
    if [ -e $CODE_TMP/${dirname} ]; then
        cp -a ${CODE_TMP}/${dirname} ${pardir}
    fi
    rm -rf ${CODE_TMP}
done
