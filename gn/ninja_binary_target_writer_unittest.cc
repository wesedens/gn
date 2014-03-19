// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/ninja_binary_target_writer.h"
#include "tools/gn/test_with_scope.h"

TEST(NinjaBinaryTargetWriter, SourceSet) {
  TestWithScope setup;
  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  setup.settings()->set_target_os(Settings::WIN);

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  target.OnResolved();

  // Source set itself.
  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, setup.toolchain(), out);
    writer.Run();

    // TODO(brettw) I think we'll need to worry about backslashes here
    // depending if we're on actual Windows or Linux pretending to be Windows.
    const char expected_win[] =
        "defines =\n"
        "includes =\n"
        "cflags =\n"
        "cflags_c =\n"
        "cflags_cc =\n"
        "cflags_objc =\n"
        "cflags_objcc =\n"
        "\n"
        "build obj/foo/bar.input1.obj: cxx ../../foo/input1.cc\n"
        "build obj/foo/bar.input2.obj: cxx ../../foo/input2.cc\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.obj obj/foo/bar.input2.obj\n";
    std::string out_str = out.str();
#if defined(OS_WIN)
    std::replace(out_str.begin(), out_str.end(), '\\', '/');
#endif
    EXPECT_EQ(expected_win, out_str);
  }

  // A shared library that depends on the source set.
  Target shlib_target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  shlib_target.set_output_type(Target::SHARED_LIBRARY);
  shlib_target.deps().push_back(LabelTargetPair(&target));
  shlib_target.OnResolved();

  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&shlib_target, setup.toolchain(), out);
    writer.Run();

    // TODO(brettw) I think we'll need to worry about backslashes here
    // depending if we're on actual Windows or Linux pretending to be Windows.
    const char expected_win[] =
        "defines =\n"
        "includes =\n"
        "cflags =\n"
        "cflags_c =\n"
        "cflags_cc =\n"
        "cflags_objc =\n"
        "cflags_objcc =\n"
        "\n"
        "\n"
        "manifests = obj/foo/shlib.intermediate.manifest\n"
        "ldflags = /MANIFEST /ManifestFile:obj/foo/shlib.intermediate."
            "manifest\n"
        "libs =\n"
        "build shlib.dll shlib.dll.lib: solink obj/foo/bar.input1.obj "
            "obj/foo/bar.input2.obj\n"
        "  soname = shlib.dll\n"
        "  lib = shlib.dll\n"
        "  dll = shlib.dll\n"
        "  implibflag = /IMPLIB:shlib.dll.lib\n\n";
    std::string out_str = out.str();
#if defined(OS_WIN)
    std::replace(out_str.begin(), out_str.end(), '\\', '/');
#endif
    EXPECT_EQ(expected_win, out_str);
  }
}

TEST(NinjaBinaryTargetWriter, ProductExtension) {
  TestWithScope setup;
  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  setup.settings()->set_target_os(Settings::LINUX);

  // A shared library w/ the product_extension set to a custom value.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_extension(std::string("so.6"));
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  target.OnResolved();

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, setup.toolchain(), out);
  writer.Run();

  // TODO(brettw) I think we'll need to worry about backslashes here
  // depending if we're on actual Windows or Linux pretending to be Windows.
  const char expected[] =
      "defines =\n"
      "includes =\n"
      "cflags =\n"
      "cflags_c =\n"
      "cflags_cc =\n"
      "cflags_objc =\n"
      "cflags_objcc =\n"
      "\n"
      "build obj/foo/shlib.input1.o: cxx ../../foo/input1.cc\n"
      "build obj/foo/shlib.input2.o: cxx ../../foo/input2.cc\n"
      "\n"
      "ldflags =\n"
      "libs =\n"
      "build lib/libshlib.so.6: solink obj/foo/shlib.input1.o "
          "obj/foo/shlib.input2.o\n"
      "  soname = libshlib.so.6\n"
      "  lib = lib/libshlib.so.6\n"
      "\n";

  std::string out_str = out.str();
#if defined(OS_WIN)
  std::replace(out_str.begin(), out_str.end(), '\\', '/');
#endif
  EXPECT_EQ(expected, out_str);
}

TEST(NinjaBinaryTargetWriter, EmptyProductExtension) {
  TestWithScope setup;
  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  setup.settings()->set_target_os(Settings::LINUX);

  // This test is the same as ProductExtension, except that
  // we call set_output_extension("") and ensure that we still get the default.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_extension(std::string());
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, setup.toolchain(), out);
  writer.Run();

  // TODO(brettw) I think we'll need to worry about backslashes here
  // depending if we're on actual Windows or Linux pretending to be Windows.
  const char expected[] =
      "defines =\n"
      "includes =\n"
      "cflags =\n"
      "cflags_c =\n"
      "cflags_cc =\n"
      "cflags_objc =\n"
      "cflags_objcc =\n"
      "\n"
      "build obj/foo/shlib.input1.o: cxx ../../foo/input1.cc\n"
      "build obj/foo/shlib.input2.o: cxx ../../foo/input2.cc\n"
      "\n"
      "ldflags =\n"
      "libs =\n"
      "build lib/libshlib.so: solink obj/foo/shlib.input1.o "
          "obj/foo/shlib.input2.o\n"
      "  soname = libshlib.so\n"
      "  lib = lib/libshlib.so\n"
      "\n";

  std::string out_str = out.str();
#if defined(OS_WIN)
  std::replace(out_str.begin(), out_str.end(), '\\', '/');
#endif
  EXPECT_EQ(expected, out_str);
}
