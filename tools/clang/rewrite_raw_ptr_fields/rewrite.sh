#!/bin/bash
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# IMPORTANT! Before running this script you have to perform these steps:
# 1. Run `mkdir ~/scratch`
# 2. Run `gn args out/{your_out_dir}` and set the following options:
#   use_remoteexec = false
#   # this can be skipped if you do this in build/config/clang/clang.gni
#   clang_use_chrome_plugins = false
#
# (You can do these steps only once, as long as you don't delete the directories
# between rewrites.)
#
# For more fine-grained instructions, see:
# https://docs.google.com/document/d/1chTvr3fSofQNV_PDPEHRyUgcJCQBgTDOOBriW9gIm9M/edit?ts=5e9549a2#heading=h.fjdnrdg1gcty

set -e  # makes the script quit on any command failure
set -u  # unset variables are quit-worthy errors

OUT_DIR="out/rewrite"
if [ "$1" != "" ]
then
  OUT_DIR="$1"
fi

COMPILE_DIRS=.
EDIT_DIRS=.

# Save llvm-build as it is about to be overwritten.
mv third_party/llvm-build third_party/llvm-build-upstream

# Build and test the rewriter.
echo "*** Building the rewriter ***"
time tools/clang/scripts/build.py \
    --without-android \
    --without-fuchsia \
    --extra-tools rewrite_raw_ptr_fields
tools/clang/rewrite_raw_ptr_fields/tests/run_all_tests.py

# Build generated files that a successful compilation depends on.
echo "*** Preparing targets ***"
gn gen $OUT_DIR
GEN_H_TARGETS=`ninja -C $OUT_DIR -t targets all | grep '^gen/.*\(\.h\|inc\|css_tokenizer_codepoints.cc\)' | cut -d : -f 1`
time ninja -C $OUT_DIR $GEN_H_TARGETS

if grep -qE '^\s*target_os\s*=\s*("win"|win)' $OUT_DIR/args.gn
then
  TARGET_OS_OPTION="--target_os=win"
fi

# A preliminary rewriter run in a special mode that generates a list of fields
# to ignore. These fields would likely lead to compiler errors if rewritten.
echo "*** Generating the ignore list ***"
time tools/clang/scripts/run_tool.py \
    $TARGET_OS_OPTION \
    --tool rewrite_raw_ptr_fields \
    --generate-compdb \
    -p $OUT_DIR \
    $COMPILE_DIRS > ~/scratch/rewriter.out
cat ~/scratch/rewriter.out \
    | sed '/^==== BEGIN FIELD FILTERS ====$/,/^==== END FIELD FILTERS ====$/{//!b};d' \
    | sort | uniq > ~/scratch/automated-fields-to-ignore.txt
cat ~/scratch/automated-fields-to-ignore.txt \
    tools/clang/rewrite_raw_ptr_fields/manual-fields-to-ignore.txt \
    | grep -v "base::FileDescriptorWatcher::Controller::watcher_" \
    > ~/scratch/combined-fields-to-ignore.txt

# Main rewrite.
echo "*** Running the main rewrite phase ***"
time tools/clang/scripts/run_tool.py \
    $TARGET_OS_OPTION \
    --tool rewrite_raw_ptr_fields \
    --tool-arg=--exclude-fields=$HOME/scratch/combined-fields-to-ignore.txt \
    -p $OUT_DIR \
    $COMPILE_DIRS > ~/scratch/rewriter.main.out

# Apply edits generated by the main rewrite.
echo "*** Applying edits ***"
cat ~/scratch/rewriter.main.out | \
    tools/clang/scripts/extract_edits.py | \
    tools/clang/scripts/apply_edits.py -p $OUT_DIR $EDIT_DIRS

# Format sources, as many lines are likely over 80 chars now.
echo "*** Formatting ***"
time git cl format

# Restore llvm-build. Without this, your future builds will be painfully slow.
rm -r -f third_party/llvm-build
mv third_party/llvm-build-upstream third_party/llvm-build
