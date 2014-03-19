// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/target_generator.h"

#include "tools/gn/binary_target_generator.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/config.h"
#include "tools/gn/copy_target_generator.h"
#include "tools/gn/err.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/functions.h"
#include "tools/gn/group_target_generator.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/scope.h"
#include "tools/gn/script_target_generator.h"
#include "tools/gn/token.h"
#include "tools/gn/value.h"
#include "tools/gn/value_extractors.h"
#include "tools/gn/variables.h"

TargetGenerator::TargetGenerator(Target* target,
                                 Scope* scope,
                                 const FunctionCallNode* function_call,
                                 Err* err)
    : target_(target),
      scope_(scope),
      function_call_(function_call),
      err_(err) {
}

TargetGenerator::~TargetGenerator() {
}

void TargetGenerator::Run() {
  // All target types use these.
  FillDependentConfigs();
  FillData();
  FillDependencies();
  FillGypFile();

  // Do type-specific generation.
  DoRun();
}

// static
void TargetGenerator::GenerateTarget(Scope* scope,
                                     const FunctionCallNode* function_call,
                                     const std::vector<Value>& args,
                                     const std::string& output_type,
                                     Err* err) {
  // Name is the argument to the function.
  if (args.size() != 1u || args[0].type() != Value::STRING) {
    *err = Err(function_call,
        "Target generator requires one string argument.",
        "Otherwise I'm not sure what to call this target.");
    return;
  }

  // The location of the target is the directory name with no slash at the end.
  // FIXME(brettw) validate name.
  const Label& toolchain_label = ToolchainLabelForScope(scope);
  Label label(scope->GetSourceDir(), args[0].string_value(),
              toolchain_label.dir(), toolchain_label.name());

  if (g_scheduler->verbose_logging())
    g_scheduler->Log("Defining target", label.GetUserVisibleName(true));

  scoped_ptr<Target> target(new Target(scope->settings(), label));
  target->set_defined_from(function_call);

  // Create and call out to the proper generator.
  if (output_type == functions::kCopy) {
    CopyTargetGenerator generator(target.get(), scope, function_call, err);
    generator.Run();
  } else if (output_type == functions::kCustom) {
    ScriptTargetGenerator generator(target.get(), scope, function_call, err);
    generator.Run();
  } else if (output_type == functions::kExecutable) {
    BinaryTargetGenerator generator(target.get(), scope, function_call,
                                    Target::EXECUTABLE, err);
    generator.Run();
  } else if (output_type == functions::kGroup) {
    GroupTargetGenerator generator(target.get(), scope, function_call, err);
    generator.Run();
  } else if (output_type == functions::kSharedLibrary) {
    BinaryTargetGenerator generator(target.get(), scope, function_call,
                                    Target::SHARED_LIBRARY, err);
    generator.Run();
  } else if (output_type == functions::kSourceSet) {
    BinaryTargetGenerator generator(target.get(), scope, function_call,
                                    Target::SOURCE_SET, err);
    generator.Run();
  } else if (output_type == functions::kStaticLibrary) {
    BinaryTargetGenerator generator(target.get(), scope, function_call,
                                    Target::STATIC_LIBRARY, err);
    generator.Run();
  } else {
    *err = Err(function_call, "Not a known output type",
               "I am very confused.");
  }

  if (!err->has_error())
    scope->settings()->build_settings()->ItemDefined(target.PassAs<Item>());
}

const BuildSettings* TargetGenerator::GetBuildSettings() const {
  return scope_->settings()->build_settings();
}

void TargetGenerator::FillSources() {
  const Value* value = scope_->GetValue(variables::kSources, true);
  if (!value)
    return;

  Target::FileList dest_sources;
  if (!ExtractListOfRelativeFiles(scope_->settings()->build_settings(), *value,
                                  scope_->GetSourceDir(), &dest_sources, err_))
    return;
  target_->sources().swap(dest_sources);
}

void TargetGenerator::FillSourcePrereqs() {
  const Value* value = scope_->GetValue(variables::kSourcePrereqs, true);
  if (!value)
    return;

  Target::FileList dest_reqs;
  if (!ExtractListOfRelativeFiles(scope_->settings()->build_settings(), *value,
                                  scope_->GetSourceDir(), &dest_reqs, err_))
    return;
  target_->source_prereqs().swap(dest_reqs);
}

void TargetGenerator::FillConfigs() {
  FillGenericConfigs(variables::kConfigs, &target_->configs());
}

void TargetGenerator::FillDependentConfigs() {
  FillGenericConfigs(variables::kAllDependentConfigs,
                     &target_->all_dependent_configs());
  FillGenericConfigs(variables::kDirectDependentConfigs,
                     &target_->direct_dependent_configs());
}

void TargetGenerator::FillData() {
  const Value* value = scope_->GetValue(variables::kData, true);
  if (!value)
    return;

  Target::FileList dest_data;
  if (!ExtractListOfRelativeFiles(scope_->settings()->build_settings(), *value,
                                  scope_->GetSourceDir(), &dest_data, err_))
    return;
  target_->data().swap(dest_data);
}

void TargetGenerator::FillDependencies() {
  FillGenericDeps(variables::kDeps, &target_->deps());
  FillGenericDeps(variables::kDatadeps, &target_->datadeps());

  // This is a list of dependent targets to have their configs fowarded, so
  // it goes here rather than in FillConfigs.
  FillForwardDependentConfigs();

  FillHardDep();
}

void TargetGenerator::FillGypFile() {
  const Value* gyp_file_value = scope_->GetValue(variables::kGypFile, true);
  if (!gyp_file_value)
    return;
  if (!gyp_file_value->VerifyTypeIs(Value::STRING, err_))
    return;

  target_->set_gyp_file(scope_->GetSourceDir().ResolveRelativeFile(
      gyp_file_value->string_value()));
}

void TargetGenerator::FillHardDep() {
  const Value* hard_dep_value = scope_->GetValue(variables::kHardDep, true);
  if (!hard_dep_value)
    return;
  if (!hard_dep_value->VerifyTypeIs(Value::BOOLEAN, err_))
    return;
  target_->set_hard_dep(hard_dep_value->boolean_value());
}

void TargetGenerator::FillExternal() {
  const Value* value = scope_->GetValue(variables::kExternal, true);
  if (!value)
    return;
  if (!value->VerifyTypeIs(Value::BOOLEAN, err_))
    return;
  target_->set_external(value->boolean_value());
}

void TargetGenerator::FillOutputs() {
  const Value* value = scope_->GetValue(variables::kOutputs, true);
  if (!value)
    return;

  Target::FileList outputs;
  if (!ExtractListOfRelativeFiles(scope_->settings()->build_settings(), *value,
                                  scope_->GetSourceDir(), &outputs, err_))
    return;

  // Validate that outputs are in the output dir.
  CHECK(outputs.size() == value->list_value().size());
  for (size_t i = 0; i < outputs.size(); i++) {
    if (!EnsureStringIsInOutputDir(
            GetBuildSettings()->build_dir(),
            outputs[i].value(), value->list_value()[i], err_))
      return;
  }
  target_->script_values().outputs().swap(outputs);
}

void TargetGenerator::FillGenericConfigs(const char* var_name,
                                         LabelConfigVector* dest) {
  const Value* value = scope_->GetValue(var_name, true);
  if (value) {
    ExtractListOfLabels(*value, scope_->GetSourceDir(),
                        ToolchainLabelForScope(scope_), dest, err_);
  }
}

void TargetGenerator::FillGenericDeps(const char* var_name,
                                      LabelTargetVector* dest) {
  const Value* value = scope_->GetValue(var_name, true);
  if (value) {
    ExtractListOfLabels(*value, scope_->GetSourceDir(),
                        ToolchainLabelForScope(scope_), dest, err_);
  }
}

void TargetGenerator::FillForwardDependentConfigs() {
  const Value* value = scope_->GetValue(
      variables::kForwardDependentConfigsFrom, true);
  if (value) {
    ExtractListOfLabels(*value, scope_->GetSourceDir(),
                        ToolchainLabelForScope(scope_),
                        &target_->forward_dependent_configs(), err_);
  }
}
