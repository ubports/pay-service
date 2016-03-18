/*
 * Copyright 2013-2014 Canonical Ltd.
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
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Ubuntu.Web 0.2
import payui 0.1 as Oxide
import "../components"


Page {
    id: pageWebkit
    objectName: "pageWebkit"

    signal purchaseSucceeded()
    signal purchaseFailed()
    signal purchaseCancelled()
    signal loading(bool value)

    property int keyboardSize: Qt.inputMethod.visible ? Qt.inputMethod.keyboardRectangle.height : 0
    property alias url: webView.url
    property alias title: pageHeader.title
    property var securityStatus: webView.securityStatus

    function parseQuery(url) {
        var argsParsed = {};
        var vars = url.split('?');
        if(vars.length == 1) {
            return argsParsed;
        }

        var args = vars[1].split('&');
        for (var i=0; i < args.length; i++) {
            var arg = decodeURI(args[i]);
            if (arg.indexOf('=') == -1) {
                argsParsed[arg.trim()] = true;
            } else {
                var keyvalue = arg.split('=');
                argsParsed[keyvalue[0].trim()] = keyvalue[1].trim();
            }
        }

        return argsParsed;
    }

    header: PageHeader {
        id: pageHeader
        title: ""
        trailingActionBar.actions: [
            Action {
                id: lockAction
                iconName: pageWebkit.securityStatus.securityLevel == Oxide.SecurityStatus.SecurityLevelSecure ? "lock" : "security-alert"
                onTriggered: {
                    PopupUtils.open(popoverComponent, lockIconPlace, {"securityStatus": pageCheckout.securityStatus})
                }
            }
        ]
    }

    Component {
        id: popoverComponent

        SecurityCertificatePopover {
            id: certPopover
            securityStatus: null
        }
    }

    WebView {
        id: webView
        objectName: "webView"
        anchors.fill: parent
        anchors.topMargin: pageHeader.height
        anchors.bottomMargin: pageWebkit.keyboardSize

        // We need to specify the dialogs to use for JS dialogs here.
        alertDialog: AlertDialog {}
        confirmDialog: ConfirmDialog {}
        promptDialog: PromptDialog {}
        beforeUnloadDialog: BeforeUnloadDialog {}

        onLoadingStateChanged: {
            pageWebkit.loading(webView.loadProgress != 100);
        }

        onUrlChanged: {
            var re_succeeded = new RegExp("/click/succeeded");
            var re_failed = new RegExp("/click/failed");
            var re_cancelled = new RegExp("/click/cancelled");

            if (re_succeeded.test(webView.url)) {
                pageWebkit.purchaseSucceeded();
            } else if (re_failed.test(webView.url)) {
                pageWebkit.purchaseFailed();
            } else if (re_cancelled.test(webView.url)) {
                pageWebkit.purchaseCancelled();
            }
        }
    }
}
