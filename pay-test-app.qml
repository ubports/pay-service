import QtQuick 2.0
import Ubuntu.Components 1.1
import Ubuntu.Components.ListItems 1.0 as ListItem
import Pay 1.0

MainView {
	id: mainview
	automaticOrientation: true

	Package {
		id: pkg
		pkgname: "pay-test-app"

		onItemStatusChanged: {
			statusBox.text = pkg.itemStatus(idBox.text)
		}
	}

	Page {
		title: i18n.tr("Pay Test")

		Column {

			width: parent.width
			height: parent.height

			ListItem.SingleControl {
				control: TextField {
					id: idBox
					x: units.gu(1)
					y: units.gu(1)
					width: parent.width - units.gu(2)
					text: i18n.tr("Item ID")

					onTextChanged: {
						statusBox.text = pkg.itemStatus(idBox.text)
					}
				}
			}
			ListItem.Divider {
			}
			ListItem.SingleControl {
				control: TextField {
					id: statusBox
					x: units.gu(1)
					y: units.gu(1)
					width: parent.width - units.gu(2)
					text: i18n.tr("Unknown")
					readOnly: true
				}
			}
			ListItem.Divider {
			}
			ListItem.SingleControl {
				control: Button {
					x: units.gu(1)
					y: units.gu(1)
					width: parent.width - units.gu(2)
					text: i18n.tr("Verify")
					onClicked: {
						pkg.verifyItem(idBox.text)
						return false
					}
				}
			}
			ListItem.SingleControl {
				control: Button {
					x: units.gu(1)
					y: units.gu(1)
					width: parent.width - units.gu(2)
					text: i18n.tr("Purchase")
					onClicked: {
						pkg.purchaseItem(idBox.text)
						return false
					}
				}
			}
		}
	}
}
