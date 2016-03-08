# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2015 Canonical Ltd.
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

import logging
import ubuntuuitoolkit as uitk

from autopilot import logging as autopilot_logging

logger = logging.getLogger(__name__)


class MainView(uitk.MainView):

    def _tap(self, objectName):
        """Find a widget and tap on it."""
        item = self.wait_select_single(objectName=objectName)
        if item.enabled:
            self.pointing_device.click_object(item)

    def _type_text(self, objectName, text):
        """Find a text widget and enter some text in it."""
        item = self.wait_select_single(
            uitk.TextField, objectName=objectName)
        item.write(text)

    @autopilot_logging.log_action(logger.info)
    def cancel(self):
        """Tap on the 'Cancel' button."""
        self._tap('cancelButton')

    @autopilot_logging.log_action(logger.info)
    def buy(self):
        """Tap on the 'Buy Now' button."""
        self._tap('buyButton')

    @autopilot_logging.log_action(logger.info)
    def enter_password(self, password):
        """Type the password into the entry field."""
        self._type_text('passwordField', password)

    @autopilot_logging.log_action(logger.info)
    def tap_on_webview(self):
        """Tap in the center of the web view."""
        self._tap('webView')

    @autopilot_logging.log_action(logger.info)
    def open_add_card_page(self):
        """Open the 'Add Credit/Debit Card' page.

        :return the Add Credit/Debit Card page.
        """
        self._tap('addCreditCardLabel')
        return self.wait_select_single(objectName='pageWebkit')

    @autopilot_logging.log_action(logger.info)
    def get_payment_types(self):
        """Get the payment types option selector widget.

        :return the Payment Types option selector widget.
        """
        return self.wait_select_single(objectName='paymentTypes')

    @autopilot_logging.log_action(logger.info)
    def get_checkout_page(self):
        """Get the widget for the Checkout page.

        :return the Checkout page widget."""
        return self.wait_select_single(objectName='pageCheckout')

    @autopilot_logging.log_action(logger.info)
    def tap_dialog_ok_button(self):
        """Tap the 'OK' button in the dialog."""
        self._tap('dialogOkButton')

    @autopilot_logging.log_action(logger.info)
    def tap_dialog_cancel_button(self):
        """Tap the 'Cancel' button in the dialog."""
        self._tap('dialogCancelButton')

    @autopilot_logging.log_action(logger.info)
    def tap_dialog_leave_button(self):
        """Tap the 'Leave' button in the dialog."""
        self._tap('leaveButton')

    @autopilot_logging.log_action(logger.info)
    def tap_dialog_stay_button(self):
        """Tap the 'Stay' button in the dialog."""
        self._tap('stayButton')

    @autopilot_logging.log_action(logger.info)
    def input_dialog_text(self, text):
        """Type the text into the dialog text entry field."""
        self._type_text('dialogInput', text)
