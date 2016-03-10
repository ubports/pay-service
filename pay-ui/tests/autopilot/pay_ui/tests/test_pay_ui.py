# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2014-2016 Canonical Ltd.
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
import testtools

from testtools.matchers import Equals, NotEquals
from autopilot.matchers import Eventually

from pay_ui import tests
from pay_ui.tests import mock_server


class PayUITestCase(tests.BasePayUITestCase):

    def setUp(self):
        self.clean_config_dir()
        self.mock_server = mock_server.MockServer()
        self.addCleanup(self.mock_server.shutdown)
        self.useFixture(fixtures.EnvironmentVariable(
            'PAY_BASE_URL',
            self.mock_server.url() + '/' + self.id().split('.')[-1]))
        self.useFixture(fixtures.EnvironmentVariable(
            'U1_SEARCH_BASE_URL', self.mock_server.url('iteminfo/')))
        self.useFixture(fixtures.EnvironmentVariable(
            'SSO_AUTH_BASE_URL', self.mock_server.url('login/')))
        self.useFixture(fixtures.EnvironmentVariable('GET_CREDENTIALS', '0'))
        self.create_config_dir()
        self.addCleanup(self.clean_config_dir)
        super().setUp()

    def app_returncode(self):
        return self.app.process.wait(timeout=30)

    def test_ui_initialized(self):
        main = self.main_view
        self.assertThat(main, NotEquals(None))

    def test_cancel_purchase(self):
        self.main_view.cancel()
        self.assertThat(self.app_returncode(), Equals(1))

    def test_basic_purchase(self):
        self.skipTest('Mouse clicks on buyButton not registering.')
        self.main_view.enter_password('password123')
        self.main_view.buy()
        self.assertThat(self.app_returncode(), Equals(0))

    def test_add_credit_card_completed(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_on_webview()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    def test_add_credit_card_returns_on_cancel(self):
        self.mock_server.set_interaction_result_cancelled()
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_on_webview()
        checkout_page = self.main_view.get_checkout_page()
        self.assertThat(checkout_page.get_properties()["active"],
                        Eventually(Equals(True)))

    def test_add_credit_card_cancelled(self):
        self.mock_server.set_interaction_result_cancelled()
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_on_webview()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(3)))

    @testtools.skip('JS Alert dialog seems to not work.')
    def test_add_credit_card_js_alert(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_ok_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    def test_add_credit_card_js_beforeunload(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_stay_button()
        self.main_view.tap_on_webview()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    @testtools.skip('Clicking add card link second seems to not work.')
    def test_add_credit_card_js_beforeunload_twice(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_leave_button()
        self.take_screenshot('beforeunload_left')
        self.main_view.open_add_card_page()
        self.take_screenshot('beforeunload_back')
        self.main_view.tap_dialog_stay_button()
        self.main_view.tap_on_web_view()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    def test_add_credit_card_js_beforeunload_cancelled(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_leave_button()
        self.take_screenshot('beforeunload_cancelled')
        self.assertThat(payment_types.get_option_count, Eventually(Equals(3)))

    def test_add_credit_card_js_confirm_ok(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_ok_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    def test_add_credit_card_js_confirm_cancel(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_cancel_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(3)))

    def test_add_credit_card_js_prompt_ok(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.input_dialog_text('friend')
        self.main_view.tap_dialog_ok_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(4)))

    def test_add_credit_card_js_prompt_ok_wrong_text(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.input_dialog_text('amigo')
        self.main_view.tap_dialog_ok_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(3)))

    def test_add_credit_card_js_prompt_cancel(self):
        payment_types = self.main_view.get_payment_types()
        self.assertThat(payment_types.get_option_count(), Equals(3))
        self.main_view.open_add_card_page()
        self.take_screenshot('add_card_page')
        self.main_view.tap_dialog_cancel_button()
        self.assertThat(payment_types.get_option_count, Eventually(Equals(3)))

    def test_purchase_with_web_interaction_completed(self):
        self.skipTest('Mouse clicks on buyButton not registering.')
        self.mock_server.set_purchase_needs_cc_interaction()
        self.main_view.buy()
        self.main_view.tap_on_webview()
        self.assertThat(self.app_returncode(), Equals(0))

    def test_purchase_with_web_interaction_cancelled(self):
        self.skipTest('Mouse clicks on buyButton not registering.')
        self.mock_server.set_purchase_needs_cc_interaction()
        self.mock_server.set_interaction_result_cancelled()
        self.main_view.buy()
        self.main_view.tap_on_webview()
        self.assertThat(self.app_returncode(), Equals(1))
