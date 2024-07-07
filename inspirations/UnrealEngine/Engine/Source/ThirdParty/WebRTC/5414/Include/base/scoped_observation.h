// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SCOPED_OBSERVATION_H_
#define BASE_SCOPED_OBSERVATION_H_

#include <stddef.h>

#include "base/check.h"
#include "base/check_op.h"
#include "base/memory/raw_ptr.h"

namespace base {

// `ScopedObservation` is used to keep track of a singular observation, i.e.,
// where an observer observes a single source only. Use
// `base::ScopedMultiSourceObservation` for objects that observe multiple
// sources.
//
// When a `ScopedObservation` is destroyed, it unregisters the observer from the
// observable if it was currently observing something. Otherwise it does
// nothing.
//
// Using a `ScopedObservation` instead of manually observing and unobserving has
// the following benefits:
// - The observer cannot accidentally forget to stop observing when it is
//   destroyed.
// - By calling `Reset`, an ongoing observation can be stopped before the
//   `ScopedObservation` is destroyed. If nothing was currently observed, then
//   calling `Reset` does nothing. This can be useful for when the observable is
//   destroyed before the observer is destroyed, because it prevents the
//   observer from accidentally unregistering itself from the destroyed
//   observable a second time when it itself is destroyed. Without
//   `ScopedObservation`, one might need to keep track of whether one has
//   already stopped observing in a separate boolean.
//
// Basic example (as a member variable):
//
//   class MyObserver : public Observable::Observer {
//    public:
//     MyObserver(Observable* observable) {
//       observation_.Observe(observable);
//     }
//     // Note how there is no need to stop observing in the destructor.
//    private:
//     ScopedObservation<Observable, Observable::Observer> observation_{this};
//   };
//
// For cases with methods not named AddObserver/RemoveObserver:
//
//   class MyStateObserver : public Observable::StateObserver {
//     ...
//    private:
//     ScopedObservation<Observable,
//                       Observable::StateObserver,
//                       &Observable::AddStateObserver,
//                       &Observable::RemoveStateObserver>
//       observation_{this};
//   };
template <class Source,
          class Observer,
          void (Source::*AddObsFn)(Observer*) = &Source::AddObserver,
          void (Source::*RemoveObsFn)(Observer*) = &Source::RemoveObserver>
class ScopedObservation {
 public:
  explicit ScopedObservation(Observer* observer) : observer_(observer) {}
  ScopedObservation(const ScopedObservation&) = delete;
  ScopedObservation& operator=(const ScopedObservation&) = delete;
  ~ScopedObservation() { Reset(); }

  // Adds the object passed to the constructor as an observer on |source|.
  // IsObserving() must be false.
  void Observe(Source* source) {
    // TODO(https://crbug.com/1145565): Make this a DCHECK once ScopedObserver
    //     has been fully retired.
    CHECK_EQ(source_, nullptr);
    source_ = source;
    (source_.get()->*AddObsFn)(observer_);
  }

  // Remove the object passed to the constructor as an observer from |source_|
  // if currently observing. Does nothing otherwise.
  void Reset() {
    if (source_) {
      (source_.get()->*RemoveObsFn)(observer_);
      source_ = nullptr;
    }
  }

  // Returns true if any source is being observed.
  bool IsObserving() const { return source_ != nullptr; }

  // Returns true if |source| is being observed.
  bool IsObservingSource(Source* source) const {
    DCHECK(source);
    return source_ == source;
  }

 private:
  const raw_ptr<Observer> observer_;

  // The observed source, if any.
  raw_ptr<Source> source_ = nullptr;
};

}  // namespace base

#endif  // BASE_SCOPED_OBSERVATION_H_