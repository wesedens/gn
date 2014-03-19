// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/build_settings.h"

#include "base/file_util.h"
#include "tools/gn/filesystem_utils.h"

BuildSettings::BuildSettings() {
}

BuildSettings::BuildSettings(const BuildSettings& other)
    : root_path_(other.root_path_),
      root_path_utf8_(other.root_path_utf8_),
      secondary_source_path_(other.secondary_source_path_),
      python_path_(other.python_path_),
      build_config_file_(other.build_config_file_),
      build_dir_(other.build_dir_),
      build_to_source_dir_string_(other.build_to_source_dir_string_),
      build_args_(other.build_args_) {
}

BuildSettings::~BuildSettings() {
}

void BuildSettings::SetRootPath(const base::FilePath& r) {
  DCHECK(r.value()[r.value().size() - 1] != base::FilePath::kSeparators[0]);
  root_path_ = r;
  root_path_utf8_ = FilePathToUTF8(root_path_);
}

void BuildSettings::SetSecondarySourcePath(const SourceDir& d) {
  secondary_source_path_ = GetFullPath(d);
}

void BuildSettings::SetBuildDir(const SourceDir& d) {
  build_dir_ = d;
  build_to_source_dir_string_ = InvertDir(d);
}

base::FilePath BuildSettings::GetFullPath(const SourceFile& file) const {
  return file.Resolve(root_path_);
}

base::FilePath BuildSettings::GetFullPath(const SourceDir& dir) const {
  return dir.Resolve(root_path_);
}

base::FilePath BuildSettings::GetFullPathSecondary(
    const SourceFile& file) const {
  return file.Resolve(secondary_source_path_);
}

base::FilePath BuildSettings::GetFullPathSecondary(
    const SourceDir& dir) const {
  return dir.Resolve(secondary_source_path_);
}

void BuildSettings::ItemDefined(scoped_ptr<Item> item) const {
  DCHECK(item);
  if (!item_defined_callback_.is_null())
    item_defined_callback_.Run(item.Pass());
}
