import json
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer


KEEP_ALIVE = True


class MyHandler(BaseHTTPRequestHandler):

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("X-Click-Token", "X-Click-Token")
        self.end_headers()

    def response_payment_types(self, fail=False):
        types = [
            {
                "description": "PayPal",
                "id": "paypal",
                "preferred": False,
                "choices": [{
                    "currencies": [
                        "USD",
                        "GBP",
                        "EUR"
                    ],
                    "id": 532,
                    "requires_interaction": False,
                    "preferred": False,
                    "description": "PayPal Preapproved (exp. 2014-04-12)"
                }]
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
                        "description": "**** **** **** 1111 (X, exp. 02/2015)"
                    },
                    {
                        "currencies": [
                            "USD"
                        ],
                        "id": 1726,
                        "requires_interaction": False,
                        "preferred": True,
                        "description": "**** **** **** 1111 (X, exp. 03/2015)"
                    }
                ]
            }
        ]
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(types), 'UTF-8'))

    def response_buy_item(self, fail=False, interaction=False):
        state = "Complete" if not interaction else "InProgress"
        response = {
            "state": state,
        }
        if interaction:
            # Real server returns path starting with / here.
            response["redirect_to"] = "/redirect.url?currency=USD"
        if fail:
            response = {}

        if self.path.find("/notpurchased/") != -1:
            self.send_response(404)
        else:
            self.send_response(200)

        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(response), 'UTF-8'))

    def response_item_info(self, fail, eurozone, dotar):
        response = {
            "title": "title",
            "publisher": "publisher",
            "price": 9.99,
            "prices": {
                "USD": 1.99,
                "EUR": 1.69,
                "GBP": 1.29,
                "ARS": 18.05,
                },
            "icon_url": "icon_url",
        }
        if fail:
            self.send_response(404)
        else:
            self.send_response(200)
        self.send_header("Content-type", "application/json")
        if eurozone:
            self.send_header("X-Suggested-Currency", "EUR")
        if dotar:
            self.send_header("X-Suggested-Currency", "ARS")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(response), 'UTF-8'))

    def response_auth_error(self):
        self.send_response(401)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(dict()), 'UTF-8'))

    def do_POST(self):
        """Respond to a POST request."""
        # content = self.rfile.read(int(self.headers.get('content-length')))
        # structure = json.loads(content.decode('utf-8'))
        self.do_GET()

    def do_GET(self):
        """Respond to a GET request."""
        if self.path.find("/authError/") != -1:
            self.response_auth_error()
        elif self.path.find("shutdown") != -1:
            global KEEP_ALIVE
            KEEP_ALIVE = False
        elif self.path.find("paymentmethods/") != -1:
            fail = self.path.find("/fail/") != -1
            self.response_payment_types(fail)
        elif self.path.find("purchases/") != -1:
            fail = self.path.find("/fail/") != -1
            interaction = self.path.find("/interaction/") != -1
            self.response_buy_item(fail=fail, interaction=interaction)
        elif self.path.find("iteminfo/") != -1:
            fail = self.path.find("/fail/") != -1
            eurozone = self.path.find("/eurozone/") != -1
            dotar = self.path.find("/dotar/") != -1
            self.response_item_info(fail, eurozone, dotar)


def run_click_server():
    server_address = ('', 8000)
    httpd = HTTPServer(server_address, MyHandler)
    global KEEP_ALIVE
    print('start')
    while KEEP_ALIVE:
        httpd.handle_request()


def run_mocked_settings():
    t = threading.Thread(target=run_click_server)
    t.start()


run_mocked_settings()
