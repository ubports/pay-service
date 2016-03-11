/*
 * Copyright 2014 Canonical Ltd.
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

import QtQuick 2.0
import Ubuntu.Components 1.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.Components.Popups 0.1
import payui 0.1 as Oxide
import "../components"

Page {
    id: pageCheckout

    title: i18n.tr("Payment")

    property int keyboardSize: Qt.inputMethod.visible ? Qt.inputMethod.keyboardRectangle.height : 0
    property alias selectedItem: paymentTypes.selectedIndex

    property alias itemIcon: iconImage.source
    property alias itemTitle: titleLabel.text
    property alias itemSubtitle: subtitleLabel.text
    property alias price: priceLabel.text
    property alias account: accountView.model
    property alias model: paymentTypes.model
    property alias password: passwordField.text
    property alias otp: twoFactorField.text
    property alias certificate: otherSecurityStatus.certificate
    property alias securityStatus: otherSecurityStatus

    property bool hasPayments: false
    property bool hasPreferredPayment: true
    property bool hasStoredPayment: false
    property bool beforeTimeout: false

    property bool hasSelectedPayment: false
    property string backendId: ""
    property string paymentId: ""

    signal cancel
    signal buy(string email, string password, string otp, string paymentId, string backendId)
    signal addCreditCard

    function launchPurchase() {
        var pay = paymentTypes.model[pageCheckout.selectedItem];
        var email = accountView.currentItem.email;
        pageCheckout.buy(email, password, otp, pay.paymentId, pay.backendId);
    }

    function showErrorMessage(message) {
        errorLabel.text = message;
        errorLabel.visible = true;
    }

    function showTwoFactor() {
        errorLabel.visible = false;
        twoFactorUI.visible = true;
    }

    function setSelectedItem() {
        for (var i=0; i < pageCheckout.model.length; i++) {
            if (pageCheckout.model[i].preferred) {
                selectedItem = i;
            }
        }
    }

    QtObject {
        id: otherSecurityStatus
        property int securityLevel: certificate == null ? Oxide.SecurityStatus.SecurityLevelNone : Oxide.SecurityStatus.SecurityLevelSecure
        property var certificate: null
    }

    head.actions:[
        Action {
            id: lockAction
            iconName: pageCheckout.securityStatus.securityLevel == Oxide.SecurityStatus.SecurityLevelSecure ? "lock" : "security-alert"
            onTriggered: {
                PopupUtils.open(popoverComponent, lockIconPlace, {"securityStatus": pageCheckout.securityStatus})
            }
        }
    ]

    Component {
        id: popoverComponent

        SecurityCertificatePopover {
            id: certPopover
            securityStatus: null
        }
    }

    Flickable {
        id: checkoutFlickable
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        contentHeight: contentItem.childrenRect.height + pageCheckout.keyboardSize

        Item {
            id: header
            height: units.gu(8)
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                topMargin: units.gu(1)
            }

            UbuntuShape {
                id: iconShape
                objectName: "iconShape"
                anchors {
                    top: parent.top
                    left: parent.left
                    margins: units.gu(1)
                }
                image: Image {
                    id: iconImage
                    objectName: "iconImage"
                }
                width: units.gu(6)
                height: units.gu(6)
            }

            Column {
                id: col
                spacing: units.gu(0.5)
                anchors {
                    left: iconShape.right
                    top: parent.top
                    right: priceLabel.left
                    bottom: parent.bottom
                    margins: units.gu(1)
                }

                Label {
                    id: titleLabel
                    objectName: "titleLabel"
                    fontSize: "medium"
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    elide: Text.ElideRight
                }
                Label {
                    id: subtitleLabel
                    objectName: "subtitleLabel"
                    fontSize: "small"
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    elide: Text.ElideRight
                }
            }

            Label {
                id: priceLabel
                objectName: "priceLabel"
                font.bold: true
                fontSize: "large"
                verticalAlignment: Text.AlignVCenter

                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: units.gu(2)
                }
            }
        }

        Rectangle {
            id: separator
            height: units.dp(1)
            color: "#d5d5d5"
            anchors {
                left: parent.left
                right: parent.right
                top: header.bottom
                rightMargin: units.gu(2)
                leftMargin: units.gu(2)
                topMargin: units.gu(1)
            }
        }

        ListView {
            id: accountView
            anchors {
                left: parent.left
                right: parent.right
                top: separator.bottom
                leftMargin: units.gu(2)
                rightMargin: units.gu(2)
                topMargin: units.gu(2)
            }
            height: units.gu(2)
            enabled: false
            delegate: Label {
                id: ubuntuIdLabel
                objectName: "ubuntuIdLabel"
                text: model.displayName
                elide: Text.ElideRight
                property string email: model.displayName
            }
        }

        TextField {
            id: passwordField
            objectName: "passwordField"
            placeholderText: i18n.tr("Enter your Ubuntu One password")
            echoMode: TextInput.Password
            visible: !pageCheckout.beforeTimeout
            anchors {
                left: parent.left
                right: parent.right
                top: accountView.bottom
                margins: units.gu(2)
            }

            Keys.onReturnPressed: launchPurchase();
        }

        Label {
            id: errorLabel
            objectName: "errorLabel"
            color: "red"
            text: ""
            wrapMode: Text.WordWrap
            visible: false
            anchors {
                left: parent.left
                right: parent.right
                top: passwordField.bottom
                margins: units.gu(2)
            }
        }

        Column {
            id: twoFactorUI
            spacing: units.gu(2)
            anchors {
                left: parent.left
                right: parent.right
                top: errorLabel.visible ? errorLabel.bottom : passwordField.bottom
                margins: units.gu(2)
            }
            visible: false

            Label {
                id: twoFactorLabel
                objectName: "twoFactorLabel"
                color: "black"
                text: i18n.tr("Type your verification code:")
                wrapMode: Text.WordWrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            TextField {
                id: twoFactorField
                objectName: "twoFactorField"
                placeholderText: i18n.tr("2-factor device code")
                inputMethodHints: Qt.ImhDigitsOnly
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Keys.onReturnPressed: launchPurchase();
            }
        }

        Rectangle {
            id: paymentSep
            height: units.dp(1)
            color: "#d5d5d5"
            anchors {
                left: parent.left
                right: parent.right
                top: twoFactorUI.visible ? twoFactorUI.bottom : (errorLabel.visible ? errorLabel.bottom : (passwordField.visible ? passwordField.bottom : accountView.bottom))
                rightMargin: units.gu(2)
                leftMargin: units.gu(2)
                topMargin: units.gu(2)
            }
        }

        OptionSelector {
            id: paymentTypes
            objectName: "paymentTypes"
            anchors {
                left: parent.left
                right: parent.right
                top: paymentSep.bottom
                margins: units.gu(2)
            }
            containerHeight: units.gu(24)
            expanded: false
            delegate: OptionSelectorDelegate {
                Item {
                    anchors.fill: parent
                    Column {
                        anchors {
                            fill: parent
                            leftMargin: units.gu(2)
                            topMargin: units.gu(2)
                            rightMargin: units.gu(5)
                        }
                        spacing: units.gu(0.25)

                        Label {
                            text: modelData.name
                            elide: Text.ElideLeft
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            fontSize: "small"
                        }
                        Label {
                            text: modelData.description
                            elide: Text.ElideLeft
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            fontSize: "x-small"
                        }
                    }
                }

                height: units.gu(8)
            }
        }

        Row {
            id: rowButtons
            anchors {
                left: parent.left
                right: parent.right
                top: paymentTypes.bottom
                margins: units.gu(4)
            }

            spacing: units.gu(2)
            property int buttonsWidth: (width / 2) - (spacing / 2)

            Button {
                id: cancelButton
                objectName: "cancelButton"
                text: i18n.tr("Cancel")
                width: parent.buttonsWidth
                color: "#797979"

                onClicked: pageCheckout.cancel();
            }
            Button {
                id: buyButton
                objectName: "buyButton"
                text: i18n.tr("Buy Now")
                color: UbuntuColors.orange
                width: parent.buttonsWidth

                onClicked: {
                    launchPurchase();
                }
            }
        }

        Label {
            id: addCreditCardLabel
            objectName: "addCreditCardLabel"
            textFormat: Text.RichText
            text: '<a href="#"><span style="color: #797979;">%1</span></a>'.arg(i18n.tr("Add credit/debit card"))
            anchors {
                left: parent.left
                right: parent.right
                top: rowButtons.bottom
                topMargin: units.gu(6)
            }
            horizontalAlignment: Text.AlignHCenter
            onLinkActivated: pageCheckout.addCreditCard();
        }
    }
}
