// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40284755): Remove this and spanify to fix the errors.
#pragma allow_unsafe_buffers
#endif

#include "base/trace_event/category_registry.h"

#include <string.h>

#include <ostream>
#include <type_traits>

#include "base/check.h"
#include "base/debug/leak_annotations.h"
#include "base/notreached.h"
#include "third_party/abseil-cpp/absl/base/dynamic_annotations.h"

namespace base {
namespace trace_event {

namespace {

// |categories_| might end up causing creating dynamic initializers if not POD.
static_assert(std::is_trivial_v<TraceCategory> &&
                  std::is_standard_layout_v<TraceCategory>,
              "TraceCategory must be POD");

}  // namespace

// static
TraceCategory CategoryRegistry::categories_[kMaxCategories] = {
    INTERNAL_TRACE_LIST_BUILTIN_CATEGORIES(INTERNAL_TRACE_INIT_CATEGORY)};

// static
std::atomic<size_t> CategoryRegistry::category_index_{
    BuiltinCategories::Size()};

// static
TraceCategory* const CategoryRegistry::kCategoryAlreadyShutdown =
    &categories_[1];
TraceCategory* const CategoryRegistry::kCategoryMetadata = &categories_[2];

// static
void CategoryRegistry::Initialize() {
  // Trace is enabled or disabled on one thread while other threads are
  // accessing the enabled flag. We don't care whether edge-case events are
  // traced or not, so we allow races on the enabled flag to keep the trace
  // macros fast.
  for (size_t i = 0; i < kMaxCategories; ++i) {
    ABSL_ANNOTATE_BENIGN_RACE(categories_[i].state_ptr(),
                              "trace_event category enabled");
    // If this DCHECK is hit in a test it means that ResetForTesting() is not
    // called and the categories state leaks between test fixtures.
    DCHECK(!categories_[i].is_enabled());
  }
}

// static
void CategoryRegistry::ResetForTesting() {
  // reset_for_testing clears up only the enabled state and filters. The
  // categories themselves cannot be cleared up because the static pointers
  // injected by the macros still point to them and cannot be reset.
  for (size_t i = 0; i < kMaxCategories; ++i)
    categories_[i].reset_for_testing();
}

// static
TraceCategory* CategoryRegistry::GetCategoryByName(const char* category_name) {
  DCHECK(!strchr(category_name, '"'))
      << "Category names may not contain double quote";

  // The categories_ is append only, avoid using a lock for the fast path.
  size_t category_index = category_index_.load(std::memory_order_acquire);

  // Search for pre-existing category group.
  for (size_t i = 0; i < category_index; ++i) {
    if (strcmp(categories_[i].name(), category_name) == 0) {
      return &categories_[i];
    }
  }
  return nullptr;
}

bool CategoryRegistry::GetOrCreateCategoryLocked(
    const char* category_name,
    CategoryInitializerFn category_initializer_fn,
    TraceCategory** category) {
  // This is the slow path: the lock is not held in the fastpath
  // (GetCategoryByName), so more than one thread could have reached here trying
  // to add the same category.
  *category = GetCategoryByName(category_name);
  if (*category)
    return false;

  // Create a new category.
  size_t category_index = category_index_.load(std::memory_order_acquire);
  if (category_index >= kMaxCategories) {
    NOTREACHED() << "must increase kMaxCategories";
  }

  // TODO(primiano): this strdup should be removed. The only documented reason
  // for it was TraceWatchEvent, which is gone. However, something might have
  // ended up relying on this. Needs some auditing before removal.
  const char* category_name_copy = strdup(category_name);
  ANNOTATE_LEAKING_OBJECT_PTR(category_name_copy);

  *category = &categories_[category_index];
  DCHECK(!(*category)->is_valid());
  DCHECK(!(*category)->is_enabled());
  (*category)->set_name(category_name_copy);
  category_initializer_fn(*category);

  // Update the max index now.
  category_index_.store(category_index + 1, std::memory_order_release);
  return true;
}

// static
const TraceCategory* CategoryRegistry::GetCategoryByStatePtr(
    const uint8_t* category_state) {
  const TraceCategory* category = TraceCategory::FromStatePtr(category_state);
  DCHECK(IsValidCategoryPtr(category));
  return category;
}

// static
bool CategoryRegistry::IsMetaCategory(const TraceCategory* category) {
  DCHECK(IsValidCategoryPtr(category));
  return category <= kCategoryMetadata;
}

// static
span<TraceCategory> CategoryRegistry::GetAllCategories() {
  // The |categories_| array is append only. We have to only guarantee to
  // not return an index to a category which is being initialized by
  // GetOrCreateCategoryByName().
  size_t category_index = category_index_.load(std::memory_order_acquire);
  return span(categories_).first(category_index);
}

// static
bool CategoryRegistry::IsValidCategoryPtr(const TraceCategory* category) {
  // If any of these are hit, something has cached a corrupt category pointer.
  uintptr_t ptr = reinterpret_cast<uintptr_t>(category);
  return ptr % sizeof(void*) == 0 &&
         ptr >= reinterpret_cast<uintptr_t>(&categories_[0]) &&
         ptr <= reinterpret_cast<uintptr_t>(&categories_[kMaxCategories - 1]);
}

}  // namespace trace_event
}  // namespace base
