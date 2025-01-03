// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_TEST_WITH_CROSAPI_PARAM_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_TEST_WITH_CROSAPI_PARAM_H_

#include <string>

#include "base/test/scoped_feature_list.h"
#include "build/chromeos_buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "base/test/scoped_command_line.h"
#endif

namespace web_app::test {

enum class CrosapiParam {
  kDisabled = 0,
  kEnabled = 1,
};

class WithCrosapiParam : public testing::WithParamInterface<CrosapiParam> {
 public:
  WithCrosapiParam();
  ~WithCrosapiParam() override;

  static std::string ParamToString(testing::TestParamInfo<CrosapiParam> param);

  // Verifies that the Lacros status is same as the param setting.
  static void VerifyLacrosStatus();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
#if BUILDFLAG(IS_CHROMEOS_ASH)
  base::test::ScopedCommandLine scoped_command_line_;
#endif
};

}  // namespace web_app::test

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_TEST_WITH_CROSAPI_PARAM_H_
