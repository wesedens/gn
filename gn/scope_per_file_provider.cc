// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/scope_per_file_provider.h"

#include "tools/gn/filesystem_utils.h"
#include "tools/gn/settings.h"
#include "tools/gn/source_file.h"
#include "tools/gn/value.h"
#include "tools/gn/variables.h"

ScopePerFileProvider::ScopePerFileProvider(Scope* scope)
    : ProgrammaticProvider(scope) {
}

ScopePerFileProvider::~ScopePerFileProvider() {
}

const Value* ScopePerFileProvider::GetProgrammaticValue(
    const base::StringPiece& ident) {
  if (ident == variables::kCurrentToolchain)
    return GetCurrentToolchain();
  if (ident == variables::kDefaultToolchain)
    return GetDefaultToolchain();
  if (ident == variables::kPythonPath)
    return GetPythonPath();

  if (ident == variables::kRootBuildDir)
    return GetRootBuildDir();
  if (ident == variables::kRootGenDir)
    return GetRootGenDir();
  if (ident == variables::kRootOutDir)
    return GetRootOutDir();
  if (ident == variables::kTargetGenDir)
    return GetTargetGenDir();
  if (ident == variables::kTargetOutDir)
    return GetTargetOutDir();
  return NULL;
}

const Value* ScopePerFileProvider::GetCurrentToolchain() {
  if (!current_toolchain_) {
    current_toolchain_.reset(new Value(NULL,
        scope_->settings()->toolchain_label().GetUserVisibleName(false)));
  }
  return current_toolchain_.get();
}

const Value* ScopePerFileProvider::GetDefaultToolchain() {
  if (!default_toolchain_) {
    default_toolchain_.reset(new Value(NULL,
        scope_->settings()->default_toolchain_label().GetUserVisibleName(
            false)));
  }
  return default_toolchain_.get();
}

const Value* ScopePerFileProvider::GetPythonPath() {
  if (!python_path_) {
    python_path_.reset(new Value(NULL,
        FilePathToUTF8(scope_->settings()->build_settings()->python_path())));
  }
  return python_path_.get();
}

const Value* ScopePerFileProvider::GetRootBuildDir() {
  if (!root_build_dir_) {
    root_build_dir_.reset(new Value(NULL, DirectoryWithNoLastSlash(
        scope_->settings()->build_settings()->build_dir())));
  }
  return root_build_dir_.get();
}

const Value* ScopePerFileProvider::GetRootGenDir() {
  if (!root_gen_dir_) {
    root_gen_dir_.reset(new Value(NULL,
        DirectoryWithNoLastSlash(GetToolchainGenDir(scope_->settings()))));
  }
  return root_gen_dir_.get();
}

const Value* ScopePerFileProvider::GetRootOutDir() {
  if (!root_out_dir_) {
    root_out_dir_.reset(new Value(NULL,
        DirectoryWithNoLastSlash(GetToolchainOutputDir(scope_->settings()))));
  }
  return root_out_dir_.get();
}

const Value* ScopePerFileProvider::GetTargetGenDir() {
  if (!target_gen_dir_) {
    target_gen_dir_.reset(new Value(NULL,
        DirectoryWithNoLastSlash(GetCurrentGenDir(scope_))));
  }
  return target_gen_dir_.get();
}

const Value* ScopePerFileProvider::GetTargetOutDir() {
  if (!target_out_dir_) {
    target_out_dir_.reset(new Value(NULL,
        DirectoryWithNoLastSlash(GetCurrentOutputDir(scope_))));
  }
  return target_out_dir_.get();
}
