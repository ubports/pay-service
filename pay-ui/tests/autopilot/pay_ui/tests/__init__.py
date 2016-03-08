# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2014-2015 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import fixtures
import json
import os
import shutil
import subprocess
import tempfile
import ubuntuuitoolkit as uitk

import pay_ui


class BasePayUITestCase(uitk.base.UbuntuUIToolkitAppTestCase):
    """Base Autopilot test case for the Pay UI project."""

    dirpath = None

    def setUp(self):
        super().setUp()
        build_dir = os.environ.get('BUILD_DIR', None)
        source_dir = os.environ.get('SOURCE_DIR', None)
        self.app = self.launch_application(build_dir, source_dir)

    def create_config_dir(self):
        self.dirpath = tempfile.mkdtemp()
        self.useFixture(fixtures.EnvironmentVariable(
            'XDG_DATA_HOME', self.dirpath))

    def clean_config_dir(self):
        if self.dirpath:
            shutil.rmtree(self.dirpath)

    def launch_application(self, build_dir=None, source_dir=None):
        if build_dir is None:
            return self.launch_click_package('com.canonical.payui')
        else:
            return self.launch_built_application(build_dir, source_dir)

    def launch_installed_app(self):
        raise NotImplementedError()

    def launch_built_application(self, build_dir, source_dir):
        built_import_path = os.path.join(build_dir, 'backend')
        self.useFixture(
            fixtures.EnvironmentVariable(
                'QML2_IMPORT_PATH', newvalue=built_import_path))
        main_qml_path = os.path.join(source_dir, 'app', 'payui.qml')
        desktop_file_path = os.path.join(build_dir, 'payui_payui.desktop')
        return self.launch_test_application(
            uitk.base.get_qmlscene_launch_command(),
            main_qml_path,
            '--desktop_file_hint={0}'.format(desktop_file_path),
            '--transparent',
            'purchase://com.example.testapp',
            app_type='qt',
            emulator_base=uitk.UbuntuUIToolkitCustomProxyObjectBase)

    def launch_click_package(self, package):
        """Launch the payui from the installed click package."""
        installed_directory = self.get_installed_directory(package)
        qml_import_path = os.path.join(
            installed_directory,
            'lib', self.get_arch_triplet())
        self.useFixture(
            fixtures.EnvironmentVariable(
                'QML2_IMPORT_PATH', newvalue=qml_import_path))
        qml_file_path = os.path.join(
            installed_directory, 'qml', 'payui.qml')
        desktop_file_path = os.path.join(
            installed_directory, 'payui_payui.desktop')
        return self.launch_test_application(
            'ubuntu-app-test',
            uitk.base.get_qmlscene_launch_command(),
            '-testability',
            qml_file_path,
            '--desktop_file_hint=%s' % desktop_file_path,
            '--transparent',
            'purchase://com.example.testapp',
            app_type='qt',
            emulator_base=uitk.UbuntuUIToolkitCustomProxyObjectBase)

    def get_arch_triplet(self):
        """Return the dpkg-architecture triplet for shared objects."""
        triplet = subprocess.Popen(
            ['dpkg-architecture', '-qDEB_HOST_MULTIARCH'],
            stdout=subprocess.PIPE).communicate()[0].decode('utf8')
        return triplet.strip()

    def get_installed_directory(self, package):
        """Return the installed path for %package."""
        manifest = json.loads(
            subprocess.Popen(
                ['click', 'info', package],
                stdout=subprocess.PIPE).communicate()[0].decode('utf8'))
        return manifest['_directory']

    @property
    def main_view(self):
        """Return main view"""
        return self.app.select_single(pay_ui.MainView)
