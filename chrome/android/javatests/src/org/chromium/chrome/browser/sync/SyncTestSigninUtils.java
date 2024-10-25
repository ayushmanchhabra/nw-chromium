// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.sync;

import org.jni_zero.CalledByNative;

import org.chromium.chrome.test.util.browser.signin.LiveSigninTestUtil;
import org.chromium.chrome.test.util.browser.signin.SigninTestRule;

/** Utility class for sign-in functionalities in native Sync browser tests. */
final class SyncTestSigninUtils {
    private static final SigninTestRule sSigninTestRule = new SigninTestRule();

    /** Sets up the test account and signs in, but does not enable Sync. */
    @CalledByNative
    private static void setUpAccountAndSignInForTesting() {
        sSigninTestRule.addTestAccountThenSignin();
    }

    /** Sets up the test account, signs in, and enables Sync-the-feature. */
    @CalledByNative
    private static void setUpAccountAndSignInAndEnableSyncForTesting() {
        sSigninTestRule.addTestAccountThenSigninAndEnableSync();
    }

    /** Signs out from the current test account. */
    @CalledByNative
    private static void signOutForTesting() {
        sSigninTestRule.signOut();
    }

    /** Sets up the test authentication environment with fake services. */
    @CalledByNative
    private static void setUpAuthForTesting() {
        sSigninTestRule.setUpRule();
    }

    /** Tears down the test authentication environment. */
    @CalledByNative
    private static void tearDownAuthForTesting() {
        // The seeded account is removed automatically when user signs out
        sSigninTestRule.tearDownRule();
    }

    /** Add an account to the device and signs in for live testing, but does not enable Sync. */
    @CalledByNative
    private static void setUpLiveAccountAndSignInForTesting(String accountName, String password) {
        LiveSigninTestUtil.getInstance().addAccountWithPasswordThenSignin(accountName, password);
    }

    /** Add an account to the device and signs in for live testing, and enables Sync-the-feature. */
    @CalledByNative
    private static void setUpLiveAccountAndSignInAndEnableSyncForTesting(
            String accountName, String password) {
        LiveSigninTestUtil.getInstance()
                .addAccountWithPasswordThenSigninAndEnableSync(accountName, password);
    }
}
