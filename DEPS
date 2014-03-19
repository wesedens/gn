# Copyright 2012 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# gn builds on Chromium base, uses GTest, all of which requires
# this build configuration.

vars = {
  "chrome_revision": "256972",
  "gmock_revision": "453",
  "gtest_revision": "677",
  "gyp_revision": "1857",
  "swarming_revision": "4196cfcee5becb6003174661b6af40cb5b1af126",
  "clang_format_revision": "198831",

  "googlecode_url": "http://%s.googlecode.com/svn", 
  "chrome_base": "http://src.chromium.org/svn/trunk",
  "chromium_git": "https://chromium.googlesource.com",

  # mac
  "llvm_url": "http://src.chromium.org/llvm-project",
  "llvm_git": "https://llvm.googlesource.com",
}

deps = {
  "src/base":
    Var("chrome_base") + "/src/base@" + Var("chrome_revision"),

  "src/build":
    Var("chrome_base") + "/src/build@" + Var("chrome_revision"),

  "src/ipc":
    Var("chrome_base") + "/src/ipc@" + Var("chrome_revision"),

  "src/third_party/apple_apsl":
    Var("chrome_base") + "/src/third_party/apple_apsl@" +
        Var("chrome_revision"),
  "src/third_party/clang_format":
    Var("chrome_base") + "/src/third_party/clang_format@" +
        Var("chrome_revision"),
  "src/third_party/clang_format/script":
    Var("llvm_url") + "/cfe/trunk/tools/clang-format@" +
        Var("clang_format_revision"),
  "src/third_party/gold":
    Var("chrome_base") + "/deps/third_party/gold@" +
        Var("chrome_revision"),
  "src/third_party/icu":
    Var("chrome_base") + "/deps/third_party/icu46@" +
        Var("chrome_revision"),
  "src/third_party/libevent":
    Var("chrome_base") + "/src/third_party/libevent@" +
        Var("chrome_revision"),
  "src/third_party/mach_override":
    Var("chrome_base") + "/src/third_party/mach_override@" +
        Var("chrome_revision"),
  "src/third_party/modp_b64":
    Var("chrome_base") + "/src/third_party/modp_b64@" +
        Var("chrome_revision"),

  "src/testing":
    Var("chrome_base") + "/src/testing@" + Var("chrome_revision"),
  "src/testing/gmock":
    "http://googlemock.googlecode.com/svn/trunk@" + Var("gmock_revision"),
  "src/testing/gtest":
    "http://googletest.googlecode.com/svn/trunk@" + Var("gtest_revision"),

  "src/tools":
    File(Var("chrome_base") + "/src/tools/find_depot_tools.py@" +
        Var("chrome_revision")),
  "src/tools/clang":
    Var("chrome_base") + "/src/tools/clang@" + Var("chrome_revision"),
  "src/tools/gn":
    Var("chrome_base") + "/src/tools/gn@" + Var("chrome_revision"),
  "src/tools/gyp":
    (Var("googlecode_url") % "gyp") + "/trunk@" + Var("gyp_revision"),
  "src/tools/swarming_client":
    Var("chromium_git") + "/external/swarming.client.git@" +
        Var("swarming_revision"),
  "src/tools/xdisplaycheck":
    Var("chrome_base") + "/src/tools/xdisplaycheck@" + Var("chrome_revision"),
}


include_rules = [
  # Everybody can use some things.
  "+base",
  "+build",
]

hooks = [
  {
    # Pull clang on mac. If nothing changed, or on non-mac platforms, this takes
    # zero seconds to run. If something changed, it downloads a prebuilt clang,
    # which takes ~20s, but clang speeds up builds by more than 20s.
    "name": "clang",
    "pattern": ".",
    "action": ["python", "src/tools/clang/scripts/update.py", "--mac-only"],
  },
  {
     # Update LASTCHANGE
     "name": "lastchange",
     "pattern": ".",
     "action": ["python", "src/build/util/lastchange.py",
                "--source-dir=src",
                "-o", "src/build/util/LASTCHANGE"],
   },
  # Pull GN binaries. This needs to be before running GYP below.
  {
    "name": "gn_win",
    "pattern": "src/tools/gn/bin/win/gn.exe.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=win32",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", "src/tools/gn/bin/win/gn.exe.sha1",
    ],
  },
  {
    "name": "gn_mac",
    "pattern": "src/tools/gn/bin/mac/gn.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=darwin",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", "src/tools/gn/bin/mac/gn.sha1",
    ],
  },
  {
    "name": "gn_linux",
    "pattern": "src/tools/gn/bin/linux/gn.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=linux*",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", "src/tools/gn/bin/linux/gn.sha1",
    ],
  },
  {
    "name": "gn_linux32",
    "pattern": "src/tools/gn/bin/linux/gn32.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=linux*",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", "src/tools/gn/bin/linux/gn32.sha1",
    ],
  },
  # Pull clang-format binaries using checked-in hashes.
  {
    "name": "clang_format_win",
    "pattern": "src/third_party/clang_format/bin/win/clang-format.exe.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=win32",
                "--no_auth",
                "--bucket", "chromium-clang-format",
                "-s", "src/third_party/clang_format/bin/win/clang-format.exe.sha1",
    ],
  },
  {
    "name": "clang_format_mac",
    "pattern": "src/third_party/clang_format/bin/mac/clang-format.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=darwin",
                "--no_auth",
                "--bucket", "chromium-clang-format",
                "-s", "src/third_party/clang_format/bin/mac/clang-format.sha1",
    ],
  },
  {
    "name": "clang_format_linux",
    "pattern": "src/third_party/clang_format/bin/linux/clang-format.sha1",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=linux*",
                "--no_auth",
                "--bucket", "chromium-clang-format",
                "-s", "src/third_party/clang_format/bin/linux/clang-format.sha1",
    ],
  },
  #{
  #  # run generate ninja upon completion of sync
  #  "pattern": ".",
  #  "action": ["gn", "--root=src", "gen", "out/gn"],
  #},
]
