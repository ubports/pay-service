/*
 * Copyright 2014-2016 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.4
import QtQuick.LocalStorage 2.0
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Ubuntu.OnlineAccounts 0.1
import Ubuntu.OnlineAccounts.Client 0.1
import payui 0.1
import "ui"

/*!
    states:
        - add-payment
        - buy-interaction
        - checkout
        - online-accounts
        - error
*/

MainView {
    id: mainView
    // objectName for functional testing purposes (autopilot-qt5)
    objectName: "payui"

    // NOTE: Must match the gettext domain for translations.
    applicationName: "pay-service"

    /*
     This property enables the application to change orientation
     when the device is rotated. The default is false.
    */
    // automaticOrientation: true

    width: units.gu(100)
    height: units.gu(75)

    property bool loading: false
    property bool purchasing: false
    property bool recentLogin: false
    property bool cancellable: true
    property string suggestedCurrency: "USD"

    backgroundColor: "transparent"

    onStateChanged: {
        mainView.backgroundColor = "white";
    }

    AccountServiceModel {
        id: accounts
        provider: "ubuntuone"
    }

    Setup {
        id: setup
        applicationId: "pay-service"
        providerId: "ubuntuone"

        onFinished: {
            mainView.recentLogin = true;
            mainView.showLoading();
            purchase.checkCredentials();
        }
    }

    Purchase {
        id: purchase

        onItemDetailsObtained: {
            suggestedCurrency = currency;
            checkout.itemIcon = icon;
            checkout.itemTitle = title;
            checkout.itemSubtitle = publisher;
            checkout.price = formatted_price;
            purchase.getPaymentTypes(currency);
        }

        onPaymentTypesObtained: {
            mainView.recentLogin = mainView.recentCredentials();
            checkout.beforeTimeout = mainView.recentLogin;

            // Check for selected payment, and keep it selected if so.
            if (checkout.hasSelectedPayment) {
                for (var i=0; i < payments.length; i++) {
                    if (payments[i].paymentId == checkout.paymentId &&
                        payments[i].backendId == checkout.backendId) {
                            payments[i].preferred = true;
                    } else {
                        payments[i].preferred = false;
                    }
                }
            }
            checkout.model = payments;
            checkout.hasPayments = payments.length != 0;
            checkout.setSelectedItem();

            mainView.state = "checkout";
            checkout.visible = true;

            hideLoading();
        }

        onNoPreferredPaymentMethod: {
            checkout.hasPreferredPayment = false;
            var values = mainView.getLastPayment();
            var backendid = values[0];
            var paymentid = values[1];
            if (backendid != "" && paymentid != "") {
                checkout.hasStoredPayment = true;
            }
        }

        onPasswordValid: {
            hideLoading();
            // Reset password and otp to not keep them in memory.
            checkout.password = "";
            checkout.otp = "";
        }

        onBuyItemFailed: {
            hideLoading();
            purchaseErrorDialog.open = true;
        }

        onBuyItemSucceeded: {
            purchase.quitSuccess();
        }

        onBuyInterationRequired: {
            webkit.title = i18n.tr("Finish Purchase");
            webkit.url = url;
            mainView.state = "buy-interaction";
            pageStack.push(webkit);
        }

        onError: {
            hideLoading();
            serverErrorDialog.open = true;
        }

        onAuthenticationError: {
            mainView.recentLogin = false;
            if (pageStack.currentPage == checkout) {
                mainView.hideLoading();
                checkout.showErrorMessage(i18n.tr("Incorrect Password, please try again."));
            } else {
                mainView.state = "online-accounts";
                setup.exec();
            }
        }

        onCredentialsFound: {
            mainView.recentLogin = mainView.recentCredentials();
            checkout.beforeTimeout = mainView.recentLogin;

            if (mainView.state == "online-accounts") {
                purchase.checkItemPurchased();
            } else if (!mainView.purchasing && mainView.state != "buy-interaction") {
                purchase.getItemDetails();
            }
        }

        onItemNotPurchased: {
            purchase.getItemDetails();
        }

        onCredentialsNotFound: {
            mainView.recentLogin = false;
            if (mainView.state == "online-accounts") {
                purchase.quitCancel();
            } else {
                mainView.state = "online-accounts";
                setup.exec();
            }
        }

        onLoginError: {
            mainView.recentLogin = false;
            mainView.hideLoading();
            checkout.showErrorMessage(message);
        }

        onTwoFactorAuthRequired: {
            mainView.hideLoading();
            checkout.showTwoFactor();
        }

        onCertificateFound: {
            checkout.certificate = cert
        }
    }

    function showLoading() {
        if (!mainView.loading) {
            mainView.loading = true;
            PopupUtils.open(loadingDialogContainer);
        }
    }

    function hideLoading() {
        mainView.purchasing = false;
        mainView.loading = false;
        mainView.cancellable = true;
    }

    function createDB() {
        var db = LocalStorage.openDatabaseSync("PayUI", "1.0", "PayUI Credentials Date", 100);
        db.transaction(
            function(tx) {
                // Create the database if it doesn't already exist
                tx.executeSql('CREATE TABLE IF NOT EXISTS PayUIPayment(backendid TEXT, paymentid TEXT)');
            }
        )
    }

    function recentCredentials() {
        var valid = false;
        var date = purchase.getTokenUpdated();
        var currentDate = new Date();
        var msec = currentDate - date;
        var mm = Math.floor(msec / 1000 / 60);
        if (mm < 15) {
            valid = true;
        }
        return valid;
    }

    function getLastPayment() {
        var backendid = "";
        var paymentid = "";
        var db = LocalStorage.openDatabaseSync("PayUI", "1.0", "PayUI Credentials Date", 100);
        db.transaction(
            function(tx) {
                var rs = tx.executeSql('SELECT * FROM PayUIPayment');
                if (rs.rows.length > 0) {
                    backendid = rs.rows.item(0).backendid;
                    paymentid = rs.rows.item(0).paymentid;
                }
            }
        )
        return [backendid, paymentid];
    }

    function updatePayment(backendid, paymentid) {
        var db = LocalStorage.openDatabaseSync("PayUI", "1.0", "PayUI Credentials Date", 100);
        db.transaction(
            function(tx) {
                var rs = tx.executeSql('SELECT * FROM PayUIPayment');
                if (rs.rows.length > 0) {
                    tx.executeSql('UPDATE PayUIPayment SET backendid = "' + backendid + '", paymentid = "' + paymentid + '"');
                } else {
                    tx.executeSql('INSERT INTO PayUIPayment VALUES(?, ?)', [backendid, paymentid]);
                }
            }
        )
    }

    ErrorDialog {
        id: purchaseErrorDialog
        title: i18n.tr("Purchase failed")
        message: i18n.tr("The purchase couldn't be completed.")

        onRetry: {
            mainView.state = "error";
            mainView.showLoading();
            purchase.getItemDetails();
        }

        onClose: {
            purchase.quitCancel();
        }
    }

    ErrorDialog {
        id: serverErrorDialog
        title: i18n.tr("Error contacting the server")
        message: i18n.tr("Do you want to try again?")

        onRetry: {
            mainView.state = "error";
            mainView.showLoading();
            purchase.getItemDetails();
        }

        onClose: {
            purchase.quitCancel();
        }
    }

    ErrorDialog {
        id: creditCardErrorDialog
        title: i18n.tr("Adding Credit Card failed")
        message: i18n.tr("Do you want to try again?")

        onRetry: {
            mainView.state = "error";
            mainView.showLoading();
            purchase.getItemDetails();
        }

        onClose: {
            purchase.quitCancel();
        }
    }

    Component {
         id: loadingDialogContainer

         Dialog {
             id: loadingDialog
             title: mainView.purchasing ? i18n.tr("Processing Purchase") : i18n.tr("Loading")
             text: i18n.tr("Please wait…")
             ActivityIndicator {
                 running: mainView.loading ? true : false
                 width: parent.width

                 onRunningChanged: {
                     if(!running) {
                         PopupUtils.close(loadingDialog);
                     }
                 }
             }

             Button {
                 objectName: "buttonCancelLoading"
                 text: i18n.tr("Cancel")
                 color: UbuntuColors.orange
                 visible: mainView.cancellable
                 onClicked: {
                     PopupUtils.close(loadingDialog);
                     purchase.quitCancel();
                 }
             }
         }
    }

    PageStack {
        id: pageStack
        objectName: "pageStack"

        Component.onCompleted: {
            showLoading();
            mainView.createDB();
            purchase.checkCredentials();
        }

        onCurrentPageChanged: {
            if (pageStack.currentPage == checkout) {
                mainView.state = "checkout";
            }
        }

        CheckoutPage {
            id: checkout
            objectName: "pageCheckout"
            visible: false
            account: accounts

            onCancel: {
                purchase.quitCancel();
            }

            onBuy: {
                mainView.recentLogin = mainView.recentCredentials();
                checkout.beforeTimeout = mainView.recentLogin;

                // Pass it on.
                checkout.hasSelectedPayment = true;
                checkout.backendId = backendId;
                checkout.paymentId = paymentId;

                mainView.purchasing = true;
                mainView.cancellable = false;
                showLoading();
                if (!checkout.hasPreferredPayment) {
                    mainView.updatePayment(backendId, paymentId);
                }

                if (mainView.recentLogin) {
                    purchase.buyItem(email, "", "", suggestedCurrency, paymentId, backendId, mainView.recentLogin);
                } else {
                    purchase.buyItem(email, password, otp, suggestedCurrency, paymentId, backendId, mainView.recentLogin);
                }
            }

            onAddCreditCard: {
                webkit.title = i18n.tr("Add Payment");
                webkit.url = purchase.getAddPaymentUrl(suggestedCurrency);
                mainView.state = "add-payment";
                pageStack.push(webkit);
            }
        }

        UbuntuPurchaseWebkit {
            id: webkit
            visible: false

            onPurchaseFailed: {
                pageStack.pop();
                hideLoading();
                purchaseErrorDialog.open = true;
            }

            onPurchaseCancelled: {
                hideLoading();
                if (mainView.state == "add-payment") {
                    mainView.state = "checkout";
                    pageStack.pop();
                } else {
                    purchase.quitCancel();
                }
            }

            onPurchaseSucceeded: {
                if (mainView.state == "add-payment") {
                    showLoading();
                    pageStack.pop();
                    purchase.getPaymentTypes(suggestedCurrency);
                } else {
                    purchase.quitSuccess();
                }
            }

            onLoading: {
                if (value) {
                    showLoading();
                } else {
                    hideLoading();
                }
            }
        }
    }

    Rectangle {
        id: lockIconPlace
        width: units.gu(7)
        height: units.gu(7)
        anchors {
            right: parent.right
            top: parent.top
        }
        visible: false
    }
}
