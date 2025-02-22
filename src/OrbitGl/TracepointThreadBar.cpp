// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TracepointThreadBar.h"

#include <GteVector.h>
#include <absl/strings/str_format.h>

#include <memory>
#include <utility>

#include "App.h"
#include "Batcher.h"
#include "CaptureViewElement.h"
#include "ClientData/CaptureData.h"
#include "ClientServices/TracepointServiceClient.h"
#include "CoreMath.h"
#include "Geometry.h"
#include "GlCanvas.h"
#include "OrbitBase/Logging.h"
#include "OrbitBase/ThreadConstants.h"
#include "TimeGraph.h"
#include "TimeGraphLayout.h"
#include "Viewport.h"
#include "capture_data.pb.h"
#include "tracepoint.pb.h"

namespace orbit_gl {

TracepointThreadBar::TracepointThreadBar(CaptureViewElement* parent, OrbitApp* app,
                                         TimeGraph* time_graph, orbit_gl::Viewport* viewport,
                                         TimeGraphLayout* layout,
                                         const orbit_client_data::CaptureData* capture_data,
                                         int32_t thread_id, const Color& color)
    : ThreadBar(parent, app, time_graph, viewport, layout, capture_data, thread_id, "Tracepoints",
                color) {}

void TracepointThreadBar::Draw(Batcher& batcher, TextRenderer& text_renderer,
                               const DrawContext& draw_context) {
  ThreadBar::Draw(batcher, text_renderer, draw_context);

  if (IsEmpty()) {
    return;
  }

  float event_bar_z = draw_context.picking_mode == PickingMode::kClick
                          ? GlCanvas::kZValueEventBarPicking
                          : GlCanvas::kZValueEventBar;
  event_bar_z += draw_context.z_offset;
  Color color = GetColor();
  Box box(pos_, Vec2(size_[0], -size_[1]), event_bar_z);
  batcher.AddBox(box, color, shared_from_this());
}

void TracepointThreadBar::UpdatePrimitives(Batcher* batcher, uint64_t min_tick, uint64_t max_tick,
                                           PickingMode picking_mode, float z_offset) {
  ThreadBar::UpdatePrimitives(batcher, min_tick, max_tick, picking_mode, z_offset);

  float z = GlCanvas::kZValueEvent + z_offset;
  float track_height = layout_->GetEventTrackHeightFromTid(GetThreadId());
  const bool picking = picking_mode != PickingMode::kNone;

  const Color kWhite(255, 255, 255, 255);
  const Color kWhiteTransparent(255, 255, 255, 190);
  const Color kGrey(128, 128, 128, 255);

  CHECK(capture_data_ != nullptr);

  if (!picking) {
    capture_data_->ForEachTracepointEventOfThreadInTimeRange(
        GetThreadId(), min_tick, max_tick,
        [&](const orbit_client_protos::TracepointEventInfo& tracepoint) {
          uint64_t time = tracepoint.time();
          float radius = track_height / 4;
          Vec2 pos(time_graph_->GetWorldFromTick(time), pos_[1]);
          if (GetThreadId() == orbit_base::kAllThreadsOfAllProcessesTid) {
            const Color color = tracepoint.pid() == capture_data_->process_id() ? kGrey : kWhite;
            batcher->AddVerticalLine(pos, -track_height, z, color);
          } else {
            batcher->AddVerticalLine(pos, -radius, z, kWhiteTransparent);
            batcher->AddVerticalLine(Vec2(pos[0], pos[1] - track_height), radius, z,
                                     kWhiteTransparent);
            batcher->AddCircle(Vec2(pos[0], pos[1] - track_height / 2), radius, z,
                               kWhiteTransparent);
          }
        });

  } else {
    constexpr float kPickingBoxWidth = 9.0f;
    constexpr float kPickingBoxOffset = kPickingBoxWidth / 2.0f;

    capture_data_->ForEachTracepointEventOfThreadInTimeRange(
        GetThreadId(), min_tick, max_tick,
        [&](const orbit_client_protos::TracepointEventInfo& tracepoint) {
          uint64_t time = tracepoint.time();
          Vec2 pos(time_graph_->GetWorldFromTick(time) - kPickingBoxOffset,
                   pos_[1] - track_height + 1);
          Vec2 size(kPickingBoxWidth, track_height);
          auto user_data =
              std::make_unique<PickingUserData>(nullptr, [&, batcher](PickingId id) -> std::string {
                return GetTracepointTooltip(batcher, id);
              });
          user_data->custom_data_ = &tracepoint;
          batcher->AddShadedBox(pos, size, z, kWhite, std::move(user_data));
        });
  }
}

std::string TracepointThreadBar::GetTracepointTooltip(Batcher* batcher, PickingId id) const {
  auto* user_data = batcher->GetUserData(id);
  CHECK(user_data && user_data->custom_data_);

  const auto* tracepoint_event_info =
      static_cast<const orbit_client_protos::TracepointEventInfo*>(user_data->custom_data_);

  uint64_t tracepoint_info_key = tracepoint_event_info->tracepoint_info_key();

  CHECK(capture_data_ != nullptr);

  orbit_grpc_protos::TracepointInfo tracepoint_info =
      capture_data_->GetTracepointInfo(tracepoint_info_key);

  if (GetThreadId() == orbit_base::kAllThreadsOfAllProcessesTid) {
    return absl::StrFormat(
        "<b>%s : %s</b><br/>"
        "<i>Tracepoint event</i><br/>"
        "<br/>"
        "<b>Core:</b> %d<br/>"
        "<b>Process:</b> %s [%d]<br/>"
        "<b>Thread:</b> %s [%d]<br/>",
        tracepoint_info.category(), tracepoint_info.name(), tracepoint_event_info->cpu(),
        capture_data_->GetThreadName(tracepoint_event_info->pid()), tracepoint_event_info->pid(),
        capture_data_->GetThreadName(tracepoint_event_info->tid()), tracepoint_event_info->tid());
  } else {
    return absl::StrFormat(
        "<b>%s : %s</b><br/>"
        "<i>Tracepoint event</i><br/>"
        "<br/>"
        "<b>Core:</b> %d<br/>",
        tracepoint_info.category(), tracepoint_info.name(), tracepoint_event_info->cpu());
  }
}

bool TracepointThreadBar::IsEmpty() const {
  return capture_data_ == nullptr ||
         capture_data_->GetNumTracepointsForThreadId(GetThreadId()) == 0;
}

}  // namespace orbit_gl