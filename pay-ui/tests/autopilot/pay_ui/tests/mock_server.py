# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2014 Canonical Ltd.
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

import json
import socket
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer


html_success = """
<html>
    <body bgcolor="green" onClick="window.location.assign('/paymentmethods/completeadd')">
        <h1>Placeholder for web interaction</h1>
        <p>Click anywhere to proceed</p>
    </body>
</html>"
"""

html_cancel = """
<html>
    <body bgcolor="red" onClick="window.location.assign('/api/2.0/click/cancelled')">
        <h1>Placeholder for web interaction</h1>
        <p>Click anywhere to cancel</p>
    </body>
</html>"
"""

html_completed = """
<html>
    <body onLoad="window.location.assign('/click/succeeded')">
    </body>
</html>
"""

html_beforeunload = """
<html>
    <script language="javascript">
        window.onbeforeunload = function() {
            return 'Really want to add your card?'
        }
        window.onload = function() {
            window.location.assign('/click/cancelled')
        }
        window.onclick = function() {
            window.onbeforeunload = null;
            window.location.assign('/paymentmethods/completeadd')
        }
    </script>
    <body bgcolor="yellow">
        <h1>Placeholder for web interaction</h1>
        <p>Click 'Stay' and then anywhere to proceed.</p>
    </body>
</html>
"""

html_alert = """
<html>
    <body onClick="window.location.assign('/paymentmethods/completeadd')"
          onLoad="alert('Click OK to add your card.')">
        <h1>Placeholder for web interaction</h1>
        <p>Click anywhere to proceed</p>
     </body>
</html>
"""

html_confirm = """
<html>
    <script language="javascript">
        window.onload = function() {
            if (window.confirm('Do you want to add your card?')) {
                window.location.assign('/paymentmethods/completeadd')
            } else {
                window.location.assign('/click/cancelled')
            }
        }
    </script>
    <body bgcolor="pink">
        <h1>Placeholder for web interaction</h1>
        <p>Click ok to add card, or cancel to not.</p>
    </body>
</html>
"""

html_prompt = """
<html>
    <script language="javascript">
        window.onload = function() {
            if (window.prompt('Speak friend and enter') == 'friend') {
                window.location.assign('/paymentmethods/completeadd')
            } else {
                window.location.assign('/click/cancelled')
            }
        }
    </script>
    <body bgcolor="pink">
        <h1>Placeholder for web interaction</h1>
        <p>Type friend and ok to add card.</p>
    </body>
</html>
"""


class MyHandler(BaseHTTPRequestHandler):

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("X-Click-Token", "X-Click-Token")
        self.end_headers()

    def response_payment_types(self, fail=False):
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(self.server.payment_types), 'UTF-8'))

    def interaction_html(self):
        return html_cancel if self.server.interaction_cancel else html_success

    def response_cc_interaction(self, fail=False):
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(bytes(self.interaction_html(), 'UTF-8'))

    def response_payment_add(self, fail=False):
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        test = self.path.split('/')[1]
        if test.find('js_alert') != -1:
            self.wrile.write(bytes(html_alert, 'UTF-8'))
        elif test.find('js_beforeunload') != -1:
            self.wfile.write(bytes(html_beforeunload, 'UTF-8'))
        elif test.find('js_confirm') != -1:
            self.wfile.write(bytes(html_confirm, 'UTF-8'))
        elif test.find('js_prompt') != -1:
            self.wfile.write(bytes(html_prompt, 'UTF-8'))
        else:
            self.wfile.write(bytes(self.interaction_html(), 'UTF-8'))

    def response_payment_add_complete(self):
        """Add the new payment info to the list."""
        self.server.payment_types[1]["choices"].append({
            "currencies": ["USD"],
            "id": 1999,
            "requires_interaction": False,
            "preferred": False,
            "description": "Yet another payment method"
        })
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(bytes(html_completed, 'UTF-8'))

    def response_buy_item(self):
        response = {
            "state": "Complete",
        }
        if self.server.buy_cc_interaction:
            response["state"] = "InProgress"
            response["redirect_to"] = self.server.buy_cc_interaction
        if self.server.fail:
            response = {}
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        try:
            self.wfile.write(bytes(json.dumps(response), 'UTF-8'))
        except socket.error:
            pass

    def response_update_credentials(self, fail=False):
        response = {
            "token_key": "token_key",
            "token_secret": "token_secret",
            "consumer_key": "consumer_key",
            "consumer_secret": "consumer_secret",
        }
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(response), 'UTF-8'))

    def response_item_info(self, fail=False):
        response = {
            "title": "title",
            "publisher": "publisher",
            "icon_url": "icon_url",
            "prices": {
                "USD": 2.99,
                "EUR": 2.49,
                "GBP": 1.99,
            },
        }
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(response), 'UTF-8'))

    def response_auth_error(self):
        self.send_response(401)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(dict()), 'UTF-8'))

    def do_POST(self):
        """Respond to a POST request."""
        #content = self.rfile.read(int(self.headers.get('content-length')))
        #structure = json.loads(content.decode('utf-8'))
        if self.path.find("purchases/") != -1:
            self.response_buy_item()

    def do_GET(self):
        """Respond to a GET request."""
        fail = self.path.find("/fail/") != -1
        if self.path.find("paymentmethods/add/") != -1:
            self.response_payment_add()
        elif self.path.find("paymentmethods/completeadd") != -1:
            self.response_payment_add_complete()
        elif self.path.find("paymentmethods/") != -1:
            self.response_payment_types(fail)
        elif self.path.find("creditcard_interaction/") != -1:
            self.response_cc_interaction()
        elif self.path.find("purchases/") != -1:
            self.response_buy_item()
        elif self.path.find("creds/") != -1 or self.path.find("wallet/") != -1:
            self.response_update_credentials(fail)
        elif self.path.find("iteminfo/") != -1:
            self.response_item_info(fail)
        elif self.path.find("/authError/") != -1:
            self.response_auth_error()


def initial_payment_types():
    return [
        {
            "description": "PayPal",
            "id": "paypal",
            "preferred": False,
            "choices": [
                {
                    "currencies": [
                        "USD",
                        "GBP",
                        "EUR"
                    ],
                    "id": 532,
                    "requires_interaction": False,
                    "preferred": False,
                    "description": ("PayPal Preapproved Payment "
                                    "(exp. 2014-04-12)")
                }
            ]
        },
        {
            "description": "Credit or Debit Card",
            "id": "credit_card",
            "preferred": True,
            "choices": [
                {
                    "currencies": [
                        "USD"
                    ],
                    "id": 1767,
                    "requires_interaction": False,
                    "preferred": False,
                    "description": ("**** **** **** 1111 "
                                    "(Visa, exp. 02/2015)")
                },
                {
                    "currencies": [
                        "USD"
                    ],
                    "id": 1726,
                    "requires_interaction": False,
                    "preferred": True,
                    "description": ("**** **** **** 1111 "
                                    "(Visa, exp. 03/2015)")
                }
            ]
        }
    ]


class MockServer:
    def __init__(self):
        server_address = ('', 0)
        self.server = HTTPServer(server_address, MyHandler)
        tcp_port = self.server.socket.getsockname()[1]
        self.base_url = "http://127.0.0.1:%d/" % tcp_port
        server_thread = threading.Thread(target=self.server.serve_forever)
        server_thread.start()
        self.server.payment_types = initial_payment_types()
        self.server.fail = False
        self.server.buy_cc_interaction = None
        self.server.interaction_cancel = False

    def set_purchase_needs_cc_interaction(self):
        # Real server returns path starting with / here, not full URL.
        self.server.buy_cc_interaction = "/creditcard_interaction"

    def set_interaction_result_cancelled(self):
        self.server.interaction_cancel = True

    def url(self, tail=""):
        return self.base_url + tail

    def shutdown(self):
        self.server.shutdown()
