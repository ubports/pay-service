/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-ual.
 *
 * go-ual is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-ual is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-ual. If not, see <http://www.gnu.org/licenses/>.
 */

package ual

import (
	"testing"
)

// Test typical observer usage: add, call and delete
func TestObserversCollection(t *testing.T) {
	collection := ObserversCollection{}

	if len(collection.observers) > 0 {
		t.Errorf("Observers collection has %d observers, expected it to be empty",
			len(collection.observers))
	}

	called := false
	id := collection.AddObserver("foo", func(a, b, c string) {
		called = true
	})

	if len(collection.observers) != 1 {
		t.Errorf("Observers collection has %d observers, expected 1",
			len(collection.observers))
	}

	helperType, err := collection.ObserverHelperType(id)
	if helperType != "foo" {
		t.Errorf(`Helper type was "%s", expected "foo"`, helperType)
	}

	observerPointer, err := collection.Observer(id)
	if err != nil {
		t.Errorf("Unexpected error obtaining observer: %s", err)
	}

	(*(*Observer)(observerPointer))("foo", "bar", "baz")

	if !called {
		t.Errorf("Expected observer to be called")
	}

	if !collection.RemoveObserver(id) {
		t.Error("Unexpected error removing observer")
	}

	if len(collection.observers) > 0 {
		t.Errorf("Observers collection has %d observers, expected it to be empty",
			len(collection.observers))
	}

	called = false
	// Should still be able to call the observer
	(*(*Observer)(observerPointer))("foo", "bar", "baz")

	if !called {
		t.Errorf("Expected observer to be called")
	}
}
