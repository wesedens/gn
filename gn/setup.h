// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SETUP_H_
#define TOOLS_GN_SETUP_H_

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/builder.h"
#include "tools/gn/loader.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/scope.h"
#include "tools/gn/settings.h"
#include "tools/gn/token.h"
#include "tools/gn/toolchain.h"

class InputFile;
class ParseNode;

namespace base {
class CommandLine;
}

extern const char kDotfile_Help[];

// Base class for code shared between Setup and DependentSetup.
class CommonSetup {
 public:
  virtual ~CommonSetup();

  // Returns the scheduler. This is virtual since only the main setup has a
  // scheduler and the derived ones just store pointers.
  virtual Scheduler* GetScheduler() = 0;

  // When true (the default), Run() will check for unresolved dependencies and
  // cycles upon completion. When false, such errors will be ignored.
  void set_check_for_bad_items(bool s) { check_for_bad_items_ = s; }

  // When true (the default), RunPostMessageLoop will check for overrides that
  // were specified but not used. When false, such errors will be ignored.
  void set_check_for_unused_overrides(bool s) {
    check_for_unused_overrides_ = s;
  }

  BuildSettings& build_settings() { return build_settings_; }
  Builder* builder() { return builder_.get(); }
  LoaderImpl* loader() { return loader_.get(); }

 protected:
  CommonSetup();
  CommonSetup(const CommonSetup& other);

  // Performs the two sets of operations to run the generation before and after
  // the message loop is run.
  void RunPreMessageLoop();
  bool RunPostMessageLoop();

  BuildSettings build_settings_;
  scoped_refptr<LoaderImpl> loader_;
  scoped_refptr<Builder> builder_;

  bool check_for_bad_items_;
  bool check_for_unused_overrides_;

 private:
  CommonSetup& operator=(const CommonSetup& other);  // Disallow.
};

// Helper class to setup the build settings and environment for the various
// commands to run.
class Setup : public CommonSetup {
 public:
  Setup();
  virtual ~Setup();

  // Configures the build for the current command line. On success returns
  // true. On failure, prints the error and returns false.
  //
  // The parameter is the string the user specified for the build directory. We
  // will try to interpret this as a SourceDir if possible, and will fail if is
  // is malformed.
  bool DoSetup(const std::string& build_dir);

  // Runs the load, returning true on success. On failure, prints the error
  // and returns false. This includes both RunPreMessageLoop() and
  // RunPostMessageLoop().
  bool Run();

  Scheduler& scheduler() { return scheduler_; }

  virtual Scheduler* GetScheduler() OVERRIDE;

 private:
  // Fills build arguments. Returns true on success.
  bool FillArguments(const base::CommandLine& cmdline);

  // Fills the root directory into the settings. Returns true on success.
  bool FillSourceDir(const base::CommandLine& cmdline);

  // Fills the build directory given the value the user has specified.
  // Must happen after FillSourceDir so we can resolve source-relative
  // paths.
  bool FillBuildDir(const std::string& build_dir);

  // Fills the python path portion of the command line. On failure, sets
  // it to just "python".
  void FillPythonPath();

  // Run config file.
  bool RunConfigFile();

  bool FillOtherConfig(const base::CommandLine& cmdline);

  Scheduler scheduler_;

  // These empty settings and toolchain are used to interpret the command line
  // and dot file.
  BuildSettings empty_build_settings_;
  Settings empty_settings_;
  Scope dotfile_scope_;

  // State for invoking the dotfile.
  base::FilePath dotfile_name_;
  scoped_ptr<InputFile> dotfile_input_file_;
  std::vector<Token> dotfile_tokens_;
  scoped_ptr<ParseNode> dotfile_root_;

  // State for invoking the command line args. We specifically want to keep
  // this around for the entire run so that Values can blame to the command
  // line when we issue errors about them.
  scoped_ptr<InputFile> args_input_file_;
  std::vector<Token> args_tokens_;
  scoped_ptr<ParseNode> args_root_;

  DISALLOW_COPY_AND_ASSIGN(Setup);
};

// A dependent setup allows one to do more than one build at a time. You would
// make a dependent setup which clones the state of the main one, make any
// necessary changes, and then run it.
//
// The way to run both at the same time is:
//   dependent_setup.RunPreMessageLoop();
//   main_setup.Run();
//   dependent_setup.RunPostMessageLoop();
// so that the main setup executes the message loop, but both are run.
class DependentSetup : public CommonSetup {
 public:
  // Note: this could be one function that takes a CommonSetup*, but then
  // the compiler can get confused what to call, since it also matches the
  // default copy constructor.
  DependentSetup(Setup* derive_from);
  DependentSetup(DependentSetup* derive_from);
  virtual ~DependentSetup();

  // These are the two parts of Run() in the regular setup, not including the
  // call to actually run the message loop.
  void RunPreMessageLoop();
  bool RunPostMessageLoop();

  virtual Scheduler* GetScheduler() OVERRIDE;

 private:
  Scheduler* scheduler_;
};

#endif  // TOOLS_GN_SETUP_H_
