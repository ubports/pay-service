import QtQuick 2.0
import QtTest 1.0
import Ubuntu.Components 0.1
import "../../ui"
import "js/unit_test.js" as UT

// See more details @ http://qt-project.org/doc/qt-5.0/qtquick/qml-testcase.html

// Execute tests with:
//   qmltestrunner

Item {
    id: root

    property string title: "My App"
    property string subtitle: "My App Subtitle"
    property string price: "$ 1.99"
    property string ubuntuid: "mail@mail.com"
    property bool called: false

    // The objects
    CheckoutPage {
        id: checkoutPage
        itemTitle: root.title
        itemSubtitle: root.subtitle
        price: root.price
        ubuntuID: root.ubuntuid

        onCancel: {
            root.called = true;
        }

        onBuy: {
            root.called = true;
        }
    }

    TestCase {
        name: "CheckoutPage"

        function init() {
            console.debug("Cleaning vars");
            root.called = false;
        }

        function test_uiTexts() {
            var titleLabel = UT.findChild(checkoutPage, "titleLabel");
            var subtitleLabel = UT.findChild(checkoutPage, "subtitleLabel");
            var priceLabel = UT.findChild(checkoutPage, "priceLabel");
            var ubuntuIdLabel = UT.findChild(checkoutPage, "ubuntuIdLabel");
            var ubuntuidExpected = "Ubuntu ID: " + root.ubuntuid

            compare(titleLabel.text, root.title);
            compare(subtitleLabel.text, root.subtitle);
            compare(priceLabel.text, root.price);
            compare(ubuntuIdLabel.text, ubuntuidExpected);
        }

        function test_cancelPressed() {
            compare(root.called, false);
            var cancelButton = UT.findChild(checkoutPage, "cancelButton");
            cancelButton.clicked();
            compare(root.called, true);
        }

        function test_buyPressed() {
            compare(root.called, false);
            var buyButton = UT.findChild(checkoutPage, "buyButton");
            buyButton.clicked();
            compare(root.called, true);
        }
    }
}
