// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_DETECTION_CORE_BACKGROUND_FILE_H_
#define COMPONENTS_LANGUAGE_DETECTION_CORE_BACKGROUND_FILE_H_

#include "base/files/file.h"
#include "base/functional/callback_forward.h"
#include "base/task/sequenced_task_runner.h"

namespace language_detection {

// Holds a `base::File`, and provides and API that pushes the user to perform
// opens and closes on a background task runner.
class BackgroundFile {
 public:
  using ReplacedCallback = base::OnceCallback<void()>;
  using FileOpener = base::OnceCallback<base::File()>;
  explicit BackgroundFile(
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);
  ~BackgroundFile();

  // Replaces the current file with the invalid `base::File()`. It closes the
  // previous file on the background task runner.
  void InvalidateFile();
  // Replaces the current file with the return value of running `file_opener` on
  // the background thread then calls `callback` on the default task runner. It
  // closes the previous file on the background task runner.
  void ReplaceFile(FileOpener file_opener, ReplacedCallback replaced_callback);
  base::File& GetFile() { return file_; }

 private:
  void SwapFile(ReplacedCallback callback, base::File new_file);

  base::File file_;

  // Used for blocking IO calls.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // All calls that mutate state in the object should be on the same sequence
  // that created it.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<BackgroundFile> weak_ptr_factory_{this};
};

}  //  namespace language_detection

#endif  // COMPONENTS_LANGUAGE_DETECTION_CORE_BACKGROUND_FILE_H_
