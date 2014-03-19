// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TARGET_H_
#define TOOLS_GN_TARGET_H_

#include <set>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/synchronization/lock.h"
#include "tools/gn/config_values.h"
#include "tools/gn/item.h"
#include "tools/gn/label_ptr.h"
#include "tools/gn/ordered_set.h"
#include "tools/gn/script_values.h"
#include "tools/gn/source_file.h"

class InputFile;
class Settings;
class Token;

class Target : public Item {
 public:
  enum OutputType {
    UNKNOWN,
    GROUP,
    EXECUTABLE,
    SHARED_LIBRARY,
    STATIC_LIBRARY,
    SOURCE_SET,
    COPY_FILES,
    CUSTOM,
  };
  typedef std::vector<SourceFile> FileList;
  typedef std::vector<std::string> StringVector;

  Target(const Settings* settings, const Label& label);
  virtual ~Target();

  // Returns a string naming the output type.
  static const char* GetStringForOutputType(OutputType type);

  // Item overrides.
  virtual Target* AsTarget() OVERRIDE;
  virtual const Target* AsTarget() const OVERRIDE;
  virtual void OnResolved() OVERRIDE;

  OutputType output_type() const { return output_type_; }
  void set_output_type(OutputType t) { output_type_ = t; }

  bool IsLinkable() const;

  // Will be the empty string to use the target label as the output name.
  const std::string& output_name() const { return output_name_; }
  void set_output_name(const std::string& name) { output_name_ = name; }

  const std::string& output_extension() const { return output_extension_; }
  void set_output_extension(const std::string& extension) {
    output_extension_ = extension;
  }

  const FileList& sources() const { return sources_; }
  FileList& sources() { return sources_; }

  // Compile-time extra dependencies.
  const FileList& source_prereqs() const { return source_prereqs_; }
  FileList& source_prereqs() { return source_prereqs_; }

  // Runtime dependencies.
  const FileList& data() const { return data_; }
  FileList& data() { return data_; }

  // Targets depending on this one should have an order dependency.
  bool hard_dep() const { return hard_dep_; }
  void set_hard_dep(bool hd) { hard_dep_ = hd; }

  // Linked dependencies.
  const LabelTargetVector& deps() const { return deps_; }
  LabelTargetVector& deps() { return deps_; }

  // Non-linked dependencies.
  const LabelTargetVector& datadeps() const { return datadeps_; }
  LabelTargetVector& datadeps() { return datadeps_; }

  // List of configs that this class inherits settings from.
  const LabelConfigVector& configs() const { return configs_; }
  LabelConfigVector& configs() { return configs_; }

  // List of configs that all dependencies (direct and indirect) of this
  // target get. These configs are not added to this target. Note that due
  // to the way this is computed, there may be duplicates in this list.
  const LabelConfigVector& all_dependent_configs() const {
    return all_dependent_configs_;
  }
  LabelConfigVector& all_dependent_configs() {
    return all_dependent_configs_;
  }

  // List of configs that targets depending directly on this one get. These
  // configs are not added to this target.
  const LabelConfigVector& direct_dependent_configs() const {
    return direct_dependent_configs_;
  }
  LabelConfigVector& direct_dependent_configs() {
    return direct_dependent_configs_;
  }

  // A list of a subset of deps where we'll re-export direct_dependent_configs
  // as direct_dependent_configs of this target.
  const LabelTargetVector& forward_dependent_configs() const {
    return forward_dependent_configs_;
  }
  LabelTargetVector& forward_dependent_configs() {
    return forward_dependent_configs_;
  }

  bool external() const { return external_; }
  void set_external(bool e) { external_ = e; }

  const std::set<const Target*>& inherited_libraries() const {
    return inherited_libraries_;
  }

  // This config represents the configuration set directly on this target.
  ConfigValues& config_values() { return config_values_; }
  const ConfigValues& config_values() const { return config_values_; }

  ScriptValues& script_values() { return script_values_; }
  const ScriptValues& script_values() const { return script_values_; }

  const OrderedSet<SourceDir>& all_lib_dirs() const { return all_lib_dirs_; }
  const OrderedSet<std::string>& all_libs() const { return all_libs_; }

  const SourceFile& gyp_file() const { return gyp_file_; }
  void set_gyp_file(const SourceFile& gf) { gyp_file_ = gf; }

 private:
  // Pulls necessary information from dependents to this one when all
  // dependencies have been resolved.
  void PullDependentTargetInfo(std::set<const Config*>* unique_configs);

  OutputType output_type_;
  std::string output_name_;
  std::string output_extension_;

  FileList sources_;
  FileList source_prereqs_;
  FileList data_;

  bool hard_dep_;

  // Note that if there are any groups in the deps, once the target is resolved
  // these vectors will list *both* the groups as well as the groups' deps.
  //
  // This is because, in general, groups should be "transparent" ways to add
  // groups of dependencies, so adding the groups deps make this happen with
  // no additional complexity when iterating over a target's deps.
  //
  // However, a group may also have specific settings and configs added to it,
  // so we also need the group in the list so we find these things. But you
  // shouldn't need to look inside the deps of the group since those will
  // already be added.
  LabelTargetVector deps_;
  LabelTargetVector datadeps_;

  LabelConfigVector configs_;
  LabelConfigVector all_dependent_configs_;
  LabelConfigVector direct_dependent_configs_;
  LabelTargetVector forward_dependent_configs_;

  bool external_;

  // Static libraries and source sets from transitive deps. These things need
  // to be linked only with the end target (executable, shared library). These
  // do not get pushed beyond shared library boundaries.
  std::set<const Target*> inherited_libraries_;

  // These libs and dirs are inherited from statically linked deps and all
  // configs applying to this target.
  OrderedSet<SourceDir> all_lib_dirs_;
  OrderedSet<std::string> all_libs_;

  ConfigValues config_values_;  // Used for all binary targets.
  ScriptValues script_values_;  // Used for script (CUSTOM) targets.

  SourceFile gyp_file_;

  DISALLOW_COPY_AND_ASSIGN(Target);
};

#endif  // TOOLS_GN_TARGET_H_
