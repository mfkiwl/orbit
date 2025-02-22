// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SchedulerTrack.h"

#include <GteVector.h>
#include <absl/strings/str_format.h>
#include <stdint.h>

#include "App.h"
#include "Batcher.h"
#include "ClientData/CaptureData.h"
#include "OrbitBase/Logging.h"
#include "TimeGraph.h"
#include "TimeGraphLayout.h"
#include "Viewport.h"
#include "capture_data.pb.h"

using orbit_client_protos::TimerInfo;

const Color kInactiveColor(100, 100, 100, 255);
const Color kSelectionColor(0, 128, 255, 255);

SchedulerTrack::SchedulerTrack(CaptureViewElement* parent, TimeGraph* time_graph,
                               orbit_gl::Viewport* viewport, TimeGraphLayout* layout, OrbitApp* app,
                               const orbit_client_data::CaptureData* capture_data)
    : TimerTrack(parent, time_graph, viewport, layout, app, capture_data) {
  SetPinned(false);
  num_cores_ = 0;
}

void SchedulerTrack::OnTimer(const orbit_client_protos::TimerInfo& timer_info) {
  TimerTrack::OnTimer(timer_info);
  if (num_cores_ <= static_cast<uint32_t>(timer_info.processor())) {
    num_cores_ = timer_info.processor() + 1;
  }
}

float SchedulerTrack::GetHeight() const {
  uint32_t num_gaps = depth_ > 0 ? depth_ - 1 : 0;
  return GetHeaderHeight() + (depth_ * layout_->GetTextCoresHeight()) +
         (num_gaps * layout_->GetSpaceBetweenCores()) + layout_->GetTrackBottomMargin();
}

bool SchedulerTrack::IsTimerActive(const TimerInfo& timer_info) const {
  bool is_same_tid_as_selected = timer_info.thread_id() == app_->selected_thread_id();
  CHECK(capture_data_ != nullptr);
  int32_t capture_process_id = capture_data_->process_id();
  bool is_same_pid_as_target =
      capture_process_id == 0 || capture_process_id == timer_info.process_id();

  return is_same_tid_as_selected || (app_->selected_thread_id() == -1 && is_same_pid_as_target);
}

Color SchedulerTrack::GetTimerColor(const TimerInfo& timer_info, bool is_selected,
                                    bool is_highlighted) const {
  if (is_highlighted) {
    return TimerTrack::kHighlightColor;
  }
  if (is_selected) {
    return kSelectionColor;
  }
  if (!IsTimerActive(timer_info)) {
    return kInactiveColor;
  }
  return TimeGraph::GetThreadColor(timer_info.thread_id());
}

float SchedulerTrack::GetYFromTimer(const TimerInfo& timer_info) const {
  uint32_t num_gaps = timer_info.depth();
  return pos_[1] - GetHeaderHeight() -
         (layout_->GetTextCoresHeight() * static_cast<float>(timer_info.depth() + 1)) -
         num_gaps * layout_->GetSpaceBetweenCores();
}

std::string SchedulerTrack::GetTooltip() const {
  return "Shows scheduling information for CPU cores";
}

std::string SchedulerTrack::GetBoxTooltip(const Batcher& batcher, PickingId id) const {
  const orbit_client_protos::TimerInfo* timer_info = batcher.GetTimerInfo(id);
  if (!timer_info) {
    return "";
  }

  CHECK(capture_data_ != nullptr);
  return absl::StrFormat(
      "<b>CPU Core activity</b><br/>"
      "<br/>"
      "<b>Core:</b> %d<br/>"
      "<b>Process:</b> %s [%d]<br/>"
      "<b>Thread:</b> %s [%d]<br/>",
      timer_info->processor(), capture_data_->GetThreadName(timer_info->process_id()),
      timer_info->process_id(), capture_data_->GetThreadName(timer_info->thread_id()),
      timer_info->thread_id());
}
