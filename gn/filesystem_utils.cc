// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/filesystem_utils.h"

#include <algorithm>

#include "base/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "tools/gn/location.h"
#include "tools/gn/settings.h"
#include "tools/gn/source_dir.h"

namespace {

enum DotDisposition {
  // The given dot is just part of a filename and is not special.
  NOT_A_DIRECTORY,

  // The given dot is the current directory.
  DIRECTORY_CUR,

  // The given dot is the first of a double dot that should take us up one.
  DIRECTORY_UP
};

// When we find a dot, this function is called with the character following
// that dot to see what it is. The return value indicates what type this dot is
// (see above). This code handles the case where the dot is at the end of the
// input.
//
// |*consumed_len| will contain the number of characters in the input that
// express what we found.
DotDisposition ClassifyAfterDot(const std::string& path,
                                size_t after_dot,
                                size_t* consumed_len) {
  if (after_dot == path.size()) {
    // Single dot at the end.
    *consumed_len = 1;
    return DIRECTORY_CUR;
  }
  if (IsSlash(path[after_dot])) {
    // Single dot followed by a slash.
    *consumed_len = 2;  // Consume the slash
    return DIRECTORY_CUR;
  }

  if (path[after_dot] == '.') {
    // Two dots.
    if (after_dot + 1 == path.size()) {
      // Double dot at the end.
      *consumed_len = 2;
      return DIRECTORY_UP;
    }
    if (IsSlash(path[after_dot + 1])) {
      // Double dot folowed by a slash.
      *consumed_len = 3;
      return DIRECTORY_UP;
    }
  }

  // The dots are followed by something else, not a directory.
  *consumed_len = 1;
  return NOT_A_DIRECTORY;
}

#if defined(OS_WIN)
inline char NormalizeWindowsPathChar(char c) {
  if (c == '/')
    return '\\';
  return base::ToLowerASCII(c);
}

// Attempts to do a case and slash-insensitive comparison of two 8-bit Windows
// paths.
bool AreAbsoluteWindowsPathsEqual(const base::StringPiece& a,
                                  const base::StringPiece& b) {
  if (a.size() != b.size())
    return false;

  // For now, just do a case-insensitive ASCII comparison. We could convert to
  // UTF-16 and use ICU if necessary. Or maybe base::strcasecmp is good enough?
  for (size_t i = 0; i < a.size(); i++) {
    if (NormalizeWindowsPathChar(a[i]) != NormalizeWindowsPathChar(b[i]))
      return false;
  }
  return true;
}

bool DoesBeginWindowsDriveLetter(const base::StringPiece& path) {
  if (path.size() < 3)
    return false;

  // Check colon first, this will generally fail fastest.
  if (path[1] != ':')
    return false;

  // Check drive letter
  if (!((path[0] >= 'A' && path[0] <= 'Z') ||
         path[0] >= 'a' && path[0] <= 'z'))
    return false;

  if (!IsSlash(path[2]))
    return false;
  return true;
}
#endif

// A wrapper around FilePath.GetComponents that works the way we need. This is
// not super efficient since it does some O(n) transformations on the path. If
// this is called a lot, we might want to optimize.
std::vector<base::FilePath::StringType> GetPathComponents(
    const base::FilePath& path) {
  std::vector<base::FilePath::StringType> result;
  path.GetComponents(&result);

  if (result.empty())
    return result;

  // GetComponents will preserve the "/" at the beginning, which confuses us.
  // We don't expect to have relative paths in this function.
  // Don't use IsSeparator since we always want to allow backslashes.
  if (result[0] == FILE_PATH_LITERAL("/") ||
      result[0] == FILE_PATH_LITERAL("\\"))
    result.erase(result.begin());

#if defined(OS_WIN)
  // On Windows, GetComponents will give us [ "C:", "/", "foo" ], and we
  // don't want the slash in there. This doesn't support input like "C:foo"
  // which means foo relative to the current directory of the C drive but
  // that's basically legacy DOS behavior we don't need to support.
  if (result.size() >= 2 && result[1].size() == 1 && IsSlash(result[1][0]))
    result.erase(result.begin() + 1);
#endif

  return result;
}

// Provides the equivalent of == for filesystem strings, trying to do
// approximately the right thing with case.
bool FilesystemStringsEqual(const base::FilePath::StringType& a,
                            const base::FilePath::StringType& b) {
#if defined(OS_WIN)
  // Assume case-insensitive filesystems on Windows. We use the CompareString
  // function to do a case-insensitive comparison based on the current locale
  // (we don't want GN to depend on ICU which is large and requires data
  // files). This isn't perfect, but getting this perfectly right is very
  // difficult and requires I/O, and this comparison should cover 99.9999% of
  // all cases.
  //
  // Note: The documentation for CompareString says it runs fastest on
  // null-terminated strings with -1 passed for the length, so we do that here.
  // There should not be embedded nulls in filesystem strings.
  return ::CompareString(LOCALE_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                         a.c_str(), -1, b.c_str(), -1) == CSTR_EQUAL;
#else
  // Assume case-sensitive filesystems on non-Windows.
  return a == b;
#endif
}

}  // namespace

SourceFileType GetSourceFileType(const SourceFile& file,
                                 Settings::TargetOS os) {
  base::StringPiece extension = FindExtension(&file.value());
  if (extension == "cc" || extension == "cpp" || extension == "cxx")
    return SOURCE_CC;
  if (extension == "h")
    return SOURCE_H;
  if (extension == "c")
    return SOURCE_C;

  switch (os) {
    case Settings::MAC:
      if (extension == "m")
        return SOURCE_M;
      if (extension == "mm")
        return SOURCE_MM;
      break;

    case Settings::WIN:
      if (extension == "rc")
        return SOURCE_RC;
      // TODO(brettw) asm files.
      break;

    default:
      break;
  }

  if (os != Settings::WIN) {
    if (extension == "S")
      return SOURCE_S;
  }

  return SOURCE_UNKNOWN;
}

const char* GetExtensionForOutputType(Target::OutputType type,
                                      Settings::TargetOS os) {
  switch (os) {
    case Settings::MAC:
      switch (type) {
        case Target::EXECUTABLE:
          return "";
        case Target::SHARED_LIBRARY:
          return "dylib";
        case Target::STATIC_LIBRARY:
          return "a";
        default:
          NOTREACHED();
      }
      break;

    case Settings::WIN:
      switch (type) {
        case Target::EXECUTABLE:
          return "exe";
        case Target::SHARED_LIBRARY:
          return "dll.lib";  // Extension of import library.
        case Target::STATIC_LIBRARY:
          return "lib";
        default:
          NOTREACHED();
      }
      break;

    case Settings::LINUX:
      switch (type) {
        case Target::EXECUTABLE:
          return "";
        case Target::SHARED_LIBRARY:
          return "so";
        case Target::STATIC_LIBRARY:
          return "a";
        default:
          NOTREACHED();
      }
      break;

    default:
      NOTREACHED();
  }
  return "";
}

std::string FilePathToUTF8(const base::FilePath::StringType& str) {
#if defined(OS_WIN)
  return base::WideToUTF8(str);
#else
  return str;
#endif
}

base::FilePath UTF8ToFilePath(const base::StringPiece& sp) {
#if defined(OS_WIN)
  return base::FilePath(base::UTF8ToWide(sp));
#else
  return base::FilePath(sp.as_string());
#endif
}

size_t FindExtensionOffset(const std::string& path) {
  for (int i = static_cast<int>(path.size()); i >= 0; i--) {
    if (IsSlash(path[i]))
      break;
    if (path[i] == '.')
      return i + 1;
  }
  return std::string::npos;
}

base::StringPiece FindExtension(const std::string* path) {
  size_t extension_offset = FindExtensionOffset(*path);
  if (extension_offset == std::string::npos)
    return base::StringPiece();
  return base::StringPiece(&path->data()[extension_offset],
                           path->size() - extension_offset);
}

size_t FindFilenameOffset(const std::string& path) {
  for (int i = static_cast<int>(path.size()) - 1; i >= 0; i--) {
    if (IsSlash(path[i]))
      return i + 1;
  }
  return 0;  // No filename found means everything was the filename.
}

base::StringPiece FindFilename(const std::string* path) {
  size_t filename_offset = FindFilenameOffset(*path);
  if (filename_offset == 0)
    return base::StringPiece(*path);  // Everything is the file name.
  return base::StringPiece(&(*path).data()[filename_offset],
                           path->size() - filename_offset);
}

base::StringPiece FindFilenameNoExtension(const std::string* path) {
  if (path->empty())
    return base::StringPiece();
  size_t filename_offset = FindFilenameOffset(*path);
  size_t extension_offset = FindExtensionOffset(*path);

  size_t name_len;
  if (extension_offset == std::string::npos)
    name_len = path->size() - filename_offset;
  else
    name_len = extension_offset - filename_offset - 1;

  return base::StringPiece(&(*path).data()[filename_offset], name_len);
}

void RemoveFilename(std::string* path) {
  path->resize(FindFilenameOffset(*path));
}

bool EndsWithSlash(const std::string& s) {
  return !s.empty() && IsSlash(s[s.size() - 1]);
}

base::StringPiece FindDir(const std::string* path) {
  size_t filename_offset = FindFilenameOffset(*path);
  if (filename_offset == 0u)
    return base::StringPiece();
  return base::StringPiece(path->data(), filename_offset);
}

bool EnsureStringIsInOutputDir(const SourceDir& dir,
                               const std::string& str,
                               const Value& originating,
                               Err* err) {
  // The last char of the dir will be a slash. We don't care if the input ends
  // in a slash or not, so just compare up until there.
  //
  // This check will be wrong for all proper prefixes "e.g. "/output" will
  // match "/out" but we don't really care since this is just a sanity check.
  const std::string& dir_str = dir.value();
  if (str.compare(0, dir_str.length() - 1, dir_str, 0, dir_str.length() - 1)
      != 0) {
    *err = Err(originating, "File is not inside output directory.",
        "The given file should be in the output directory. Normally you would "
        "specify\n\"$target_out_dir/foo\" or "
        "\"$target_gen_dir/foo\". I interpreted this as\n\""
        + str + "\".");
    return false;
  }
  return true;
}

bool IsPathAbsolute(const base::StringPiece& path) {
  if (path.empty())
    return false;

  if (!IsSlash(path[0])) {
#if defined(OS_WIN)
    // Check for Windows system paths like "C:\foo".
    if (path.size() > 2 && path[1] == ':' && IsSlash(path[2]))
      return true;
#endif
    return false;  // Doesn't begin with a slash, is relative.
  }

  // Double forward slash at the beginning means source-relative (we don't
  // allow backslashes for denoting this).
  if (path.size() > 1 && path[1] == '/')
    return false;

  return true;
}

bool MakeAbsolutePathRelativeIfPossible(const base::StringPiece& source_root,
                                        const base::StringPiece& path,
                                        std::string* dest) {
  DCHECK(IsPathAbsolute(source_root));
  DCHECK(IsPathAbsolute(path));

  dest->clear();

  if (source_root.size() > path.size())
    return false;  // The source root is longer: the path can never be inside.

#if defined(OS_WIN)
  // Source root should be canonical on Windows. Note that the initial slash
  // must be forward slash, but that the other ones can be either forward or
  // backward.
  DCHECK(source_root.size() > 2 && source_root[0] != '/' &&
         source_root[1] == ':' && IsSlash(source_root[2]));

  size_t after_common_index = std::string::npos;
  if (DoesBeginWindowsDriveLetter(path)) {
    // Handle "C:\foo"
    if (AreAbsoluteWindowsPathsEqual(source_root,
                                     path.substr(0, source_root.size())))
      after_common_index = source_root.size();
    else
      return false;
  } else if (path[0] == '/' && source_root.size() <= path.size() - 1 &&
             DoesBeginWindowsDriveLetter(path.substr(1))) {
    // Handle "/C:/foo"
    if (AreAbsoluteWindowsPathsEqual(source_root,
                                     path.substr(1, source_root.size())))
      after_common_index = source_root.size() + 1;
    else
      return false;
  } else {
    return false;
  }

  // If we get here, there's a match and after_common_index identifies the
  // part after it.

  // The base may or may not have a trailing slash, so skip all slashes from
  // the path after our prefix match.
  size_t first_after_slash = after_common_index;
  while (first_after_slash < path.size() && IsSlash(path[first_after_slash]))
    first_after_slash++;

  dest->assign("//");  // Result is source root relative.
  dest->append(&path.data()[first_after_slash],
               path.size() - first_after_slash);
  return true;

#else

  // On non-Windows this is easy. Since we know both are absolute, just do a
  // prefix check.
  if (path.substr(0, source_root.size()) == source_root) {
    // The base may or may not have a trailing slash, so skip all slashes from
    // the path after our prefix match.
    size_t first_after_slash = source_root.size();
    while (first_after_slash < path.size() && IsSlash(path[first_after_slash]))
      first_after_slash++;

    dest->assign("//");  // Result is source root relative.
    dest->append(&path.data()[first_after_slash],
                 path.size() - first_after_slash);
    return true;
  }
  return false;
#endif
}

std::string InvertDir(const SourceDir& path) {
  const std::string value = path.value();
  if (value.empty())
    return std::string();

  DCHECK(value[0] == '/');
  size_t begin_index = 1;

  // If the input begins with two slashes, skip over both (this is a
  // source-relative dir). These must be forward slashes only.
  if (value.size() > 1 && value[1] == '/')
    begin_index = 2;

  std::string ret;
  for (size_t i = begin_index; i < value.size(); i++) {
    if (IsSlash(value[i]))
      ret.append("../");
  }
  return ret;
}

void NormalizePath(std::string* path) {
  char* pathbuf = path->empty() ? NULL : &(*path)[0];

  // top_index is the first character we can modify in the path. Anything
  // before this indicates where the path is relative to.
  size_t top_index = 0;
  bool is_relative = true;
  if (!path->empty() && pathbuf[0] == '/') {
    is_relative = false;

    if (path->size() > 1 && pathbuf[1] == '/') {
      // Two leading slashes, this is a path into the source dir.
      top_index = 2;
    } else {
      // One leading slash, this is a system-absolute path.
      top_index = 1;
    }
  }

  size_t dest_i = top_index;
  for (size_t src_i = top_index; src_i < path->size(); /* nothing */) {
    if (pathbuf[src_i] == '.') {
      if (src_i == 0 || IsSlash(pathbuf[src_i - 1])) {
        // Slash followed by a dot, see if it's something special.
        size_t consumed_len;
        switch (ClassifyAfterDot(*path, src_i + 1, &consumed_len)) {
          case NOT_A_DIRECTORY:
            // Copy the dot to the output, it means nothing special.
            pathbuf[dest_i++] = pathbuf[src_i++];
            break;
          case DIRECTORY_CUR:
            // Current directory, just skip the input.
            src_i += consumed_len;
            break;
          case DIRECTORY_UP:
            // Back up over previous directory component. If we're already
            // at the top, preserve the "..".
            if (dest_i > top_index) {
              // The previous char was a slash, remove it.
              dest_i--;
            }

            if (dest_i == top_index) {
              if (is_relative) {
                // We're already at the beginning of a relative input, copy the
                // ".." and continue. We need the trailing slash if there was
                // one before (otherwise we're at the end of the input).
                pathbuf[dest_i++] = '.';
                pathbuf[dest_i++] = '.';
                if (consumed_len == 3)
                  pathbuf[dest_i++] = '/';

                // This also makes a new "root" that we can't delete by going
                // up more levels.  Otherwise "../.." would collapse to
                // nothing.
                top_index = dest_i;
              }
              // Otherwise we're at the beginning of an absolute path. Don't
              // allow ".." to go up another level and just eat it.
            } else {
              // Just find the previous slash or the beginning of input.
              while (dest_i > 0 && !IsSlash(pathbuf[dest_i - 1]))
                dest_i--;
            }
            src_i += consumed_len;
        }
      } else {
        // Dot not preceeded by a slash, copy it literally.
        pathbuf[dest_i++] = pathbuf[src_i++];
      }
    } else if (IsSlash(pathbuf[src_i])) {
      if (src_i > 0 && IsSlash(pathbuf[src_i - 1])) {
        // Two slashes in a row, skip over it.
        src_i++;
      } else {
        // Just one slash, copy it, normalizing to foward slash.
        pathbuf[dest_i] = '/';
        dest_i++;
        src_i++;
      }
    } else {
      // Input nothing special, just copy it.
      pathbuf[dest_i++] = pathbuf[src_i++];
    }
  }
  path->resize(dest_i);
}

void ConvertPathToSystem(std::string* path) {
#if defined(OS_WIN)
  for (size_t i = 0; i < path->size(); i++) {
    if ((*path)[i] == '/')
      (*path)[i] = '\\';
  }
#endif
}

std::string PathToSystem(const std::string& path) {
  std::string ret(path);
  ConvertPathToSystem(&ret);
  return ret;
}

std::string RebaseSourceAbsolutePath(const std::string& input,
                                     const SourceDir& dest_dir) {
  CHECK(input.size() >= 2 && input[0] == '/' && input[1] == '/')
      << "Input to rebase isn't source-absolute: " << input;
  CHECK(dest_dir.is_source_absolute())
      << "Dir to rebase to isn't source-absolute: " << dest_dir.value();

  const std::string& dest = dest_dir.value();

  // Skip the common prefixes of the source and dest as long as they end in
  // a [back]slash.
  size_t common_prefix_len = 2;  // The beginning two "//" are always the same.
  size_t max_common_length = std::min(input.size(), dest.size());
  for (size_t i = common_prefix_len; i < max_common_length; i++) {
    if (IsSlash(input[i]) && IsSlash(dest[i]))
      common_prefix_len = i + 1;
    else if (input[i] != dest[i])
      break;
  }

  // Invert the dest dir starting from the end of the common prefix.
  std::string ret;
  for (size_t i = common_prefix_len; i < dest.size(); i++) {
    if (IsSlash(dest[i]))
      ret.append("../");
  }

  // Append any remaining unique input.
  ret.append(&input[common_prefix_len], input.size() - common_prefix_len);

  // If the result is still empty, the paths are the same.
  if (ret.empty())
    ret.push_back('.');

  return ret;
}

std::string DirectoryWithNoLastSlash(const SourceDir& dir) {
  std::string ret;

  if (dir.value().empty()) {
    // Just keep input the same.
  } else if (dir.value() == "/") {
    ret.assign("/.");
  } else if (dir.value() == "//") {
    ret.assign("//.");
  } else {
    ret.assign(dir.value());
    ret.resize(ret.size() - 1);
  }
  return ret;
}

SourceDir SourceDirForPath(const base::FilePath& source_root,
                           const base::FilePath& path) {
  std::vector<base::FilePath::StringType> source_comp =
      GetPathComponents(source_root);
  std::vector<base::FilePath::StringType> path_comp =
      GetPathComponents(path);

  // See if path is inside the source root by looking for each of source root's
  // components at the beginning of path.
  bool is_inside_source;
  if (path_comp.size() < source_comp.size()) {
    // Too small to fit.
    is_inside_source = false;
  } else {
    is_inside_source = true;
    for (size_t i = 0; i < source_comp.size(); i++) {
      if (!FilesystemStringsEqual(source_comp[i], path_comp[i])) {
        is_inside_source = false;
        break;
      }
    }
  }

  std::string result_str;
  size_t initial_path_comp_to_use;
  if (is_inside_source) {
    // Construct a source-relative path beginning in // and skip all of the
    // shared directories.
    result_str = "//";
    initial_path_comp_to_use = source_comp.size();
  } else {
    // Not inside source code, construct a system-absolute path.
    result_str = "/";
    initial_path_comp_to_use = 0;
  }

  for (size_t i = initial_path_comp_to_use; i < path_comp.size(); i++) {
    result_str.append(FilePathToUTF8(path_comp[i]));
    result_str.push_back('/');
  }
  return SourceDir(result_str);
}

SourceDir SourceDirForCurrentDirectory(const base::FilePath& source_root) {
  base::FilePath cd;
  base::GetCurrentDirectory(&cd);
  return SourceDirForPath(source_root, cd);
}

SourceDir GetToolchainOutputDir(const Settings* settings) {
  const OutputFile& toolchain_subdir = settings->toolchain_output_subdir();

  std::string result = settings->build_settings()->build_dir().value();
  if (!toolchain_subdir.value().empty())
    result.append(toolchain_subdir.value());

  return SourceDir(SourceDir::SWAP_IN, &result);
}

SourceDir GetToolchainGenDir(const Settings* settings) {
  const OutputFile& toolchain_subdir = settings->toolchain_output_subdir();

  std::string result = settings->build_settings()->build_dir().value();
  if (!toolchain_subdir.value().empty())
    result.append(toolchain_subdir.value());

  result.append("gen/");
  return SourceDir(SourceDir::SWAP_IN, &result);
}

SourceDir GetOutputDirForSourceDir(const Settings* settings,
                                   const SourceDir& source_dir) {
  SourceDir toolchain = GetToolchainOutputDir(settings);

  std::string ret;
  toolchain.SwapValue(&ret);
  ret.append("obj/");

  // The source dir should be source-absolute, so we trim off the two leading
  // slashes to append to the toolchain object directory.
  DCHECK(source_dir.is_source_absolute());
  ret.append(&source_dir.value()[2], source_dir.value().size() - 2);

  return SourceDir(SourceDir::SWAP_IN, &ret);
}

SourceDir GetGenDirForSourceDir(const Settings* settings,
                                const SourceDir& source_dir) {
  SourceDir toolchain = GetToolchainGenDir(settings);

  std::string ret;
  toolchain.SwapValue(&ret);

  // The source dir should be source-absolute, so we trim off the two leading
  // slashes to append to the toolchain object directory.
  DCHECK(source_dir.is_source_absolute());
  ret.append(&source_dir.value()[2], source_dir.value().size() - 2);

  return SourceDir(SourceDir::SWAP_IN, &ret);
}

SourceDir GetTargetOutputDir(const Target* target) {
  return GetOutputDirForSourceDir(target->settings(), target->label().dir());
}

SourceDir GetTargetGenDir(const Target* target) {
  return GetGenDirForSourceDir(target->settings(), target->label().dir());
}

SourceDir GetCurrentOutputDir(const Scope* scope) {
  return GetOutputDirForSourceDir(scope->settings(), scope->GetSourceDir());
}

SourceDir GetCurrentGenDir(const Scope* scope) {
  return GetGenDirForSourceDir(scope->settings(), scope->GetSourceDir());
}
