'''com.canonical.pay.store mock template

This creates the expected methods of a pay.store and adds
some extra control methods for adding/updating items
and their states.
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Charles Kerr'
__email__ = 'charles.kerr@canonical.com'
__copyright__ = '(c) 2015 Canonical Ltd.'
__license__ = 'LGPL 3+'

import dbus
import time

from dbusmock import MOCK_IFACE, mockobject
import dbusmock

BUS_NAME = 'com.canonical.payments'
MAIN_OBJ = '/com/canonical/pay/store'
MAIN_IFACE = 'com.canonical.pay.storemock'
SYSTEM_BUS = False

STORE_PATH_PREFIX = MAIN_OBJ
STORE_IFACE = 'com.canonical.pay.store'

ERR_PREFIX = 'org.freedesktop.DBus.Error'
ERR_INVAL = ERR_PREFIX + '.InvalidArgs'


##
##  Util
##


def build_store_path(package_name):
	return STORE_PATH_PREFIX + '/' + package_name

##
##  Store
##

class Item:
	__default_bus_properties = {
		'acknowledged': dbus.Boolean(False),
		'acknowledged_time': dbus.UInt64(0.0),
		'description': dbus.String('The is a default item'),
		'id': dbus.String('default_item'),
		'price': dbus.String('$1'),
		'purchased_time': dbus.UInt64(0.0),
		'status': dbus.String('not purchased'),
		'type': dbus.String('unlockable'),
		'title': dbus.String('Default Item')
	}

	def __init__(self, id):
		self.bus_properties = Item.__default_bus_properties.copy()
		self.bus_properties['id'] = id

	def serialize(self):
		return dbus.Dictionary(self.bus_properties)

	def set_property(self, key, value):
		if not key in self.bus_properties:
			raise dbus.exceptions.DBusException(
				ERR_INVAL,
				'invalid item property {0}'.format(key))
		self.bus_properties[key] = value


@dbus.service.method(STORE_IFACE, in_signature='a{sv}')
def StoreAddItem(store, properties):

	if 'id' not in properties:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'item has no id property')

	if properties['id'] in store.items:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} already has item {1}'.format(store.name, properties.id))

	id = properties['id']
	item = Item(id)
	store.items[id] = item
	store.StoreSetItem(id, properties)

@dbus.service.method(STORE_IFACE, in_signature='sa{sv}', out_signature='')
def StoreSetItem(store, id, properties):

	try:
		item = store.items[id]
		for key, value in properties.items():
			item.set_property(key, value)
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} has no such item {1}'.format(store.name, id))

@dbus.service.method(STORE_IFACE, in_signature='s', out_signature='a{sv}')
def GetItem(store, id):
	try:
		return store.items[id].serialize()
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} has no such item {1}'.format(store.name, id))

@dbus.service.method(STORE_IFACE, in_signature='', out_signature='aa{sv}')
def GetPurchasedItems(store):
	items = []
	for id, item in store.items.items():
		if item.bus_properties['status'] == 'purchased':
			items.append(item.serialize())
	return dbus.Array(items, signature='a{sv}', variant_level=1)

@dbus.service.method(STORE_IFACE, in_signature='s', out_signature='a{sv}')
def PurchaseItem(store, id):
	try:
		item = store.items[id]
		item.set_property('status', 'purchased')
		item.set_property('purchased_time', dbus.UInt64(time.time()))
		return item.serialize()
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} has no such item {1}'.format(store.name, id))

@dbus.service.method(STORE_IFACE, in_signature='s', out_signature='a{sv}')
def RefundItem(store, id):
	try:
		item = store.items[id]
		item.set_property('status', 'not purchased')
		item.set_property('purchased_time', dbus.UInt64(0))
		return item.serialize()
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} has no such item {1}'.format(store.name, id))

@dbus.service.method(STORE_IFACE, in_signature='s', out_signature='a{sv}')
def AcknowledgeItem(store, id):
	try:
		item = store.items[id]
		item.set_property('acknowledged', dbus.Boolean(True))
		item.set_property('acknowledged_time', dbus.UInt64(time.time()))
		return item.serialize()
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'store {0} has no such item {1}'.format(store.name, id))

##
##  Main 'Storemock' Obj
##

@dbus.service.method(MAIN_IFACE, in_signature='saa{sv}', out_signature='')
def AddStore(mock, package_name, items):

	path = build_store_path(package_name)
	#mock.log('store {0} being added'.format(path))
	mock.AddObject(path, STORE_IFACE, {}, [])

	store = mockobject.objects[path]
	store.store_name = package_name
	store.items = { }
	for item in items:
		mock.AddItem(package_name, item)

@dbus.service.method(MAIN_IFACE, in_signature='', out_signature='as')
def GetStores(mock):
	names = []
	for key, val in mockobject.objects.items():
		try:
			names.append(val.store_name)
		except AttributeError:
			pass
	return dbus.Array(names, signature='s', variant_level=1)


@dbus.service.method(MAIN_IFACE, in_signature='sa{sv}', out_signature='')
def AddItem(mock, package_name, item):
	try:
		path = build_store_path(package_name)
		#mock.log('store {0} adding item {1}'.format(path, item))
		mockobject.objects[path].StoreAddItem(item)
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'no such package {0}'.format(package_name))

@dbus.service.method(MAIN_IFACE, in_signature='sa{sv}', out_signature='')
def SetItem(mock, package_name, item):
	try:
		path = build_store_path(package_name)
		mockobject.objects[path].SetItem(item)
	except KeyError:
		raise dbus.exceptions.DBusException(
			ERR_INVAL,
			'no such package {0}'.format(package_name))
		
def load(mock, parameters):
	pass



