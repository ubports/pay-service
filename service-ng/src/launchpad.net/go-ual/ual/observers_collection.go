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
	"fmt"
	"sync"
	"unsafe"
)

type Observer func(string, string, string)
type ObserverId int

// ObserversCollection is responsible for tracking observers for a given event.
// It exists for two reasons. First of all, we can't pass Go functions into C.
// Second, if we packed the function into the UAL `user_data` Go would lose
// track of it and it'd be garbage-collected. ObserversCollection can safely be
// accessed simultaneously from multiple threads.
type ObserversCollection struct {
	mutex      sync.RWMutex
	observers  map[ObserverId]observerAndHelperType
	observerId ObserverId
}

// observerAndHelperType associates an observer and a helper type.
type observerAndHelperType struct {
	observerPointer unsafe.Pointer
	helperType      string
}

// AddObserver adds an observer to the collection and returns a unique ID for
// it. This ID is used for all further ObserversCollection communication for
// that observer (e.g. calling the observer, removing it, etc.). It also
// returns an unsafe.Pointer for the observer which can be passed into C.
func (collection *ObserversCollection) AddObserver(helperType string,
	observer Observer) ObserverId {
	collection.mutex.Lock()
	defer collection.mutex.Unlock()

	if collection.observers == nil {
		collection.observers = make(map[ObserverId]observerAndHelperType)
	}

	collection.observerId += 1

	collection.observers[collection.observerId] = observerAndHelperType{
		observerPointer: unsafe.Pointer(&observer),
		helperType:      helperType,
	}

	return collection.observerId
}

// RemoveObserver removes the observer represented by the given ID from the
// collection.
func (collection *ObserversCollection) RemoveObserver(id ObserverId) bool {
	collection.mutex.Lock()
	defer collection.mutex.Unlock()

	// Reading from a potentially nil map is okay-- it will behave as if it was
	// empty.
	_, ok := collection.observers[id]
	if ok {
		delete(collection.observers, id)
		return true
	}

	return false
}

// Observer returns the unsafe.Pointer for a given observer.
func (collection *ObserversCollection) Observer(id ObserverId) (unsafe.Pointer, error) {
	collection.mutex.RLock()
	defer collection.mutex.RUnlock()

	// Reading from a potentially nil map is okay-- it will behave as if it was
	// empty.
	observerAndHelper, ok := collection.observers[id]
	if ok {
		return observerAndHelper.observerPointer, nil
	}

	return nil, fmt.Errorf("No observer with ID %d", id)
}

// ObserverHelperType returns the helper type for a given observer.
func (collection *ObserversCollection) ObserverHelperType(id ObserverId) (string, error) {
	collection.mutex.RLock()
	defer collection.mutex.RUnlock()

	// Reading from a potentially nil map is okay-- it will behave as if it was
	// empty.
	observerAndHelper, ok := collection.observers[id]
	if ok {
		return observerAndHelper.helperType, nil
	}

	return "", fmt.Errorf("No observer with ID %d", id)
}
