# Copyright (c) 2021 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

if(NOT TARGET GTest::Main)
  add_library(GTest_Main OBJECT EXCLUDE_FROM_ALL test_main.cpp Path.cpp)
  target_include_directories(GTest_Main PUBLIC include/)
  target_link_libraries(GTest_Main PUBLIC OrbitBase GTest::GTest)
  add_library(GTest::Main ALIAS GTest_Main)
endif()

if(NOT TARGET GTest::QtCoreMain AND TARGET Qt5::Core)
  add_library(GTest_QtCoreMain OBJECT EXCLUDE_FROM_ALL test_qtcore_main.cpp Path.cpp)
  target_include_directories(GTest_QtCoreMain PUBLIC include/)
  target_link_libraries(GTest_QtCoreMain PUBLIC OrbitBase GTest::GTest Qt5::Core)
  add_library(GTest::QtCoreMain ALIAS GTest_QtCoreMain)
endif()

if(NOT TARGET GTest::QtGuiMain AND TARGET Qt5::Widgets)
  add_library(GTest_QtGuiMain OBJECT EXCLUDE_FROM_ALL test_qtgui_main.cpp Path.cpp)
  target_include_directories(GTest_QtGuiMain PUBLIC include/)
  target_link_libraries(GTest_QtGuiMain PUBLIC OrbitBase GTest::GTest Qt5::Widgets CONAN_PKG::abseil)
  add_library(GTest::QtGuiMain ALIAS GTest_QtGuiMain)
endif()