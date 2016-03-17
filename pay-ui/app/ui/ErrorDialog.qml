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
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

Item {
    id: dialogErrorItem
    property string title: ""
    property string message: ""
    property bool open: false

    signal retry
    signal close

    onOpenChanged: {
        if (dialogErrorItem.open) {
            PopupUtils.open(errorDialogContainer);
        }
    }

    Component {
         id: errorDialogContainer

         Dialog {
             id: errorDialog
             title: dialogErrorItem.title
             text: dialogErrorItem.message

             Button {
                 text: i18n.tr("Retry")
                 objectName: "retryErrorButton"
                 color: UbuntuColors.orange
                 onClicked: {
                     dialogErrorItem.retry();
                     dialogErrorItem.open = false;
                     PopupUtils.close(errorDialog);
                 }
             }
             Button {
                 text: i18n.tr("Close")
                 objectName: "closeErrorButton"
                 color: UbuntuColors.coolGrey
                 onClicked: {
                     dialogErrorItem.close();
                     dialogErrorItem.open = false;
                     PopupUtils.close(errorDialog);
                 }
             }
         }
    }

}
