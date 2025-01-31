// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_HID_HID_INPUT_REPORT_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_HID_HID_INPUT_REPORT_EVENT_H_

#include "third_party/blink/renderer/core/typed_arrays/array_buffer_view_helpers.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_data_view.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class HIDDevice;

class HIDInputReportEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  HIDInputReportEvent(const AtomicString& type,
                      HIDDevice* device,
                      uint8_t report_id,
                      const Vector<uint8_t>& data);
  ~HIDInputReportEvent() override;

  HIDDevice* device() const { return device_.Get(); }
  uint8_t reportId() const { return report_id_; }
  NotShared<DOMDataView> data() const { return data_; }

  // Event:
  const AtomicString& InterfaceName() const override;
  void Trace(Visitor*) const override;

 private:
  Member<HIDDevice> device_;
  uint8_t report_id_;
  NotShared<DOMDataView> data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_HID_HID_INPUT_REPORT_EVENT_H_
