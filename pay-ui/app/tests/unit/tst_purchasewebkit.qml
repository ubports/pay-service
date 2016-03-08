import QtQuick 2.0
import QtTest 1.0
import Ubuntu.Components 0.1
import "../../ui"

// See more details @ http://qt-project.org/doc/qt-5.0/qtquick/qml-testcase.html

// Execute tests with:
//   qmltestrunner

Item {
    id: root

    property bool succeeded: false
    property bool failed: false

    // The objects
    UbuntuPurchaseWebkit {
        id: purchaseWebkit

        onPurchaseCanceled: {
            root.failed = true;
        }

        onPurchaseSucceeded: {
            root.succeeded = true;
        }
    }

    TestCase {
        name: "UbuntuPurchaseWebkitPage"

        function init() {
            console.debug("Cleaning vars");
            root.succeeded = false;
            root.failed = false;
        }

        function test_normalNavigation() {
            purchaseWebkit.url = "http://fakepage.com";
            compare(root.failed, false);
            compare(root.succeeded, false);
        }

        function test_succeeded() {
            purchaseWebkit.url = "https://sc.staging.ubuntu.com/click/succeeded";
            compare(root.failed, false);
            compare(root.succeeded, true);
        }

        function test_failed() {
            purchaseWebkit.url = "https://sc.staging.ubuntu.com/click/succeeded";
            compare(root.failed, true);
            compare(root.succeeded, false);
        }
    }
}
