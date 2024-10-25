// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/strings/escape.h"
#import "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#import "components/feature_engagement/public/feature_constants.h"
#import "components/plus_addresses/features.h"
#import "components/plus_addresses/metrics/plus_address_metrics.h"
#import "components/plus_addresses/plus_address_test_utils.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/autofill/ui_bundled/autofill_app_interface.h"
#import "ios/chrome/browser/metrics/model/metrics_app_interface.h"
#import "ios/chrome/browser/plus_addresses/ui/plus_address_bottom_sheet_constants.h"
#import "ios/chrome/browser/signin/model/fake_system_identity.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey_ui_test_util.h"
#import "ios/chrome/browser/ui/settings/settings_table_view_controller_constants.h"
#import "ios/chrome/common/string_util.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_actions_app_interface.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/web_http_server_chrome_test_case.h"
#import "ios/testing/earl_grey/app_launch_manager.h"
#import "ios/testing/earl_grey/earl_grey_test.h"
#import "ios/testing/earl_grey/matchers.h"
#import "net/test/embedded_test_server/default_handlers.h"
#import "net/test/embedded_test_server/request_handler_util.h"
#import "ui/base/l10n/l10n_util_mac.h"

namespace {

constexpr char kEmailFormUrl[] = "/email_signup_form.html";
constexpr char kEmailFieldId[] = "email";
constexpr char kFakeSuggestionLabel[] = "Lorem Ipsum";

// Assert that a given plus address modal event of type `event_type` occurred
// `count` times.
void ExpectModalHistogram(
    plus_addresses::metrics::PlusAddressModalEvent event_type,
    int count) {
  NSError* error =
      [MetricsAppInterface expectCount:count
                             forBucket:static_cast<int>(event_type)
                          forHistogram:@"PlusAddresses.Modal.Events"];
  GREYAssertNil(error, @"Failed to record modal event histogram");
}

// Assert that the bottom sheet shown duration metrics is recorded.
// Actual duration is not assessed to avoid unnecessary clock mocking.
void ExpectModalTimeSample(
    plus_addresses::metrics::PlusAddressModalCompletionStatus status,
    int count) {
  NSString* modalStatus = [NSString
      stringWithUTF8String:plus_addresses::metrics::
                               PlusAddressModalCompletionStatusToString(status)
                                   .c_str()];
  NSString* name = [NSString
      stringWithFormat:@"PlusAddresses.Modal.%@.ShownDuration", modalStatus];

  NSError* error = [MetricsAppInterface expectTotalCount:count
                                            forHistogram:name];
  GREYAssertNil(error, @"Failed to record modal shown duration histogram");
}

}  // namespace

// Test suite that tests plus addresses functionality.
@interface PlusAddressesTestCase : WebHttpServerChromeTestCase
@end

@implementation PlusAddressesTestCase {
  FakeSystemIdentity* _fakeIdentity;
}

- (void)setUp {
  [super setUp];
  net::test_server::RegisterDefaultHandlers(self.testServer);
  self.testServer->RegisterRequestHandler(base::BindRepeating(
      &net::test_server::HandlePrefixedRequest, "/v1/profiles",
      base::BindRepeating(
          &plus_addresses::test::HandleRequestToPlusAddressWithSuccess)));
  GREYAssertTrue(self.testServer->Start(), @"Server did not start.");

  if ([self isRunningTest:@selector
            (DISABLED_testConfirmPlusAddressUsingBottomSheet)] ||
      [self isRunningTest:@selector(DISABLED_testRefresh)] ||
      [self isRunningTest:@selector(DISABLED_testCreatePlusAddressIPH)]) {
    [self relaunchAppAndSetConfiguration];
  }

  GREYAssertNil([MetricsAppInterface setupHistogramTester],
                @"Failed to set up histogram tester.");

  // Ensure a fake identity is available, as this is required by the
  // plus_addresses feature.
  _fakeIdentity = [FakeSystemIdentity fakeIdentity1];
  [SigninEarlGrey signinWithFakeIdentity:_fakeIdentity];

  [self loadPlusAddressEligiblePage];
}

- (void)tearDown {
  [super tearDown];
  GREYAssertNil([MetricsAppInterface releaseHistogramTester],
                @"Cannot reset histogram tester.");
}

- (void)relaunchAppAndSetConfiguration {
  AppLaunchConfiguration config = self.appConfigurationForTestCase;

  config.features_enabled_and_params.clear();
  config.features_enabled_and_params.push_back(
      {plus_addresses::features::kPlusAddressesEnabled,
       {{{"server-url", {self.testServer->base_url().spec()}},
         {"oauth-scope", {plus_addresses::test::kFakeOauthScope}},
         {"manage-url", {plus_addresses::test::kFakeManagementUrl}},
         {"error-report-url", {plus_addresses::test::kFakeErrorReportUrl}}}}});

  if ([self isRunningTest:@selector(DISABLED_testCreatePlusAddressIPH)]) {
    config.iph_feature_enabled =
        feature_engagement::kIPHPlusAddressCreateSuggestionFeature.name;
  }

  // Relaunch the app to take the configuration into account.
  [[AppLaunchManager sharedManager] ensureAppLaunchedWithConfiguration:config];
}

- (AppLaunchConfiguration)appConfigurationForTestCase {
  AppLaunchConfiguration config;
  // Ensure the feature is enabled, including a required param.
  // TODO(crbug.com/40276862): Set up fake responses via `self.testServer`, or
  // use an app interface to force different states without a backend
  // dependency. The `chrome://version` part in the `server-url` param is just
  // to force an invalid response, and must not be used long-term.
  std::string fakeLocalUrl =
      base::EscapeQueryParamValue("chrome://version", /*use_plus=*/false);
  config.features_enabled_and_params.push_back(
      {plus_addresses::features::kPlusAddressesEnabled,
       {{
           {"server-url", {fakeLocalUrl}},
           {"manage-url", {fakeLocalUrl}},
       }}});
  return config;
}

#pragma mark - Helper methods

// Loads simple page on localhost, ensuring that it is eligible for the
// plus_addresses feature.
- (void)loadPlusAddressEligiblePage {
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kEmailFormUrl)];
  [ChromeEarlGrey waitForWebStateContainingText:"Signup form"];
}

// Taps on the create plus address suggestion to open the bottom sheet.
- (void)openCreatePlusAddressBottomSheet {
  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];

  NSString* suggestionLabel = base::SysUTF8ToNSString(kFakeSuggestionLabel);
  id<GREYMatcher> userChip =
      [AutofillAppInterface isKeyboardAccessoryUpgradeEnabled]
          ? grey_accessibilityLabel([NSString
                stringWithFormat:@"%@, %@", suggestionLabel, suggestionLabel])
          : grey_text(suggestionLabel);

  // Ensure the plus_address suggestion appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:userChip];

  // Tapping it will trigger the UI.
  [[EarlGrey selectElementWithMatcher:userChip] performAction:grey_tap()];
}

id<GREYMatcher> GetMatcherForErrorReportLink() {
  return grey_allOf(
      // The link is within
      // kPlusAddressModalErrorMessageAccessibilityIdentifier.
      grey_ancestor(grey_accessibilityID(
          kPlusAddressSheetErrorMessageAccessibilityIdentifier)),
      // UIKit instantiates a `UIAccessibilityLinkSubelement` for the link
      // element in the label with attributed string.
      grey_kindOfClassName(@"UIAccessibilityLinkSubelement"),
      grey_accessibilityTrait(UIAccessibilityTraitLink), nil);
}

// Returns a matcher for the email description.
id<GREYMatcher> GetMatcherForEmailDescription(NSString* email) {
  NSString* message =
      l10n_util::GetNSStringF(IDS_PLUS_ADDRESS_BOTTOMSHEET_DESCRIPTION_IOS,
                              base::SysNSStringToUTF16(email));
  return grey_allOf(
      // The link is within
      // kPlusAddressSheetDescriptionAccessibilityIdentifier.
      grey_text(message),
      grey_accessibilityID(kPlusAddressSheetDescriptionAccessibilityIdentifier),
      nil);
}

// Returns a matcher for the plus address field.
id<GREYMatcher> GetMatcherForPlusAddressLabel(NSString* labelText) {
  return grey_allOf(
      // The link is within
      // kPlusAddressLabelAccessibilityIdentifier.
      grey_accessibilityID(kPlusAddressLabelAccessibilityIdentifier),
      grey_text(labelText),
      grey_accessibilityTrait(UIAccessibilityTraitStaticText), nil);
}

// Verifies that field with the html `id` has been filled with `value`.
- (void)verifyFieldWithIdHasBeenFilled:(std::string)id value:(NSString*)value {
  NSString* condition = [NSString
      stringWithFormat:@"window.document.getElementById('%s').value === '%@'",
                       id.c_str(), value];
  [ChromeEarlGrey waitForJavaScriptCondition:condition];
}

#pragma mark - Tests

// TODO(crbug.com/364105204) Fix test failures.
// Tests showing up a bottom sheet to confirm a plus address. Once, the plus
// address is confirmed checks if it is filled in the file.d
- (void)DISABLED_testConfirmPlusAddressUsingBottomSheet {
  [self openCreatePlusAddressBottomSheet];

  id<GREYMatcher> plusAddressLabelMatcher = GetMatcherForPlusAddressLabel(
      base::SysUTF8ToNSString(plus_addresses::test::kFakePlusAddress));
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:plusAddressLabelMatcher];

  id<GREYMatcher> confirmButton =
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_PLUS_ADDRESS_BOTTOMSHEET_OK_TEXT_IOS);

  // Click the okay button, confirming the plus address.
  [[EarlGrey selectElementWithMatcher:confirmButton] performAction:grey_tap()];

  [self verifyFieldWithIdHasBeenFilled:kEmailFieldId
                                 value:base::SysUTF8ToNSString(
                                           plus_addresses::test::
                                               kFakePlusAddress)];

  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalShown, 1);
  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalConfirmed, 1);

  ExpectModalTimeSample(plus_addresses::metrics::
                            PlusAddressModalCompletionStatus::kModalConfirmed,
                        1);
}

// A basic test that simply opens the bottom sheet with an error and then
// dismisses the bottom sheet.
- (void)DISABLED_testShowPlusAddressBottomSheetWithError {
  [self openCreatePlusAddressBottomSheet];

  // The primary email address should be shown.
  [ChromeEarlGrey
      waitForUIElementToAppearWithMatcher:GetMatcherForEmailDescription(
                                              _fakeIdentity.userEmail)];

  // The request to reserve a plus address is hitting the test server, and
  // should fail immediately.
  NSString* error_message = l10n_util::GetNSString(
      IDS_PLUS_ADDRESS_BOTTOMSHEET_REPORT_ERROR_INSTRUCTION_IOS);
  id<GREYMatcher> parsed_error_message =
      grey_text(ParseStringWithLinks(error_message).string);
  // Ensure error message with link is shown and correctly parsed.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:parsed_error_message];

  // Ensure the cancel button is shown.
  id<GREYMatcher> cancelButton =
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_PLUS_ADDRESS_MODAL_CANCEL_TEXT);

  // Click the cancel button, dismissing the bottom sheet.
  [[EarlGrey selectElementWithMatcher:cancelButton] performAction:grey_tap()];

  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalShown, 1);
  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalCanceled, 1);
  // The test server currently only response with reserve error. Thus, closing
  // status is recorded as `kReservePlusAddressError`.
  // TODO(b/321072266) Expand coverage to other responses.
  ExpectModalTimeSample(
      plus_addresses::metrics::PlusAddressModalCompletionStatus::
          kReservePlusAddressError,
      1);
}

- (void)DISABLED_testPlusAddressBottomSheetErrorReportLink {
  [self openCreatePlusAddressBottomSheet];

  id<GREYMatcher> link_text = GetMatcherForErrorReportLink();

  // Take note of how many tabs are open before clicking the link.
  NSUInteger oldRegularTabCount = [ChromeEarlGrey mainTabCount];
  NSUInteger oldIncognitoTabCount = [ChromeEarlGrey incognitoTabCount];

  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:link_text];
  [[EarlGrey selectElementWithMatcher:link_text] performAction:grey_tap()];

  // A new tab should open after tapping the link.
  [ChromeEarlGrey waitForMainTabCount:oldRegularTabCount + 1];
  [ChromeEarlGrey waitForIncognitoTabCount:oldIncognitoTabCount];

  // The bottom sheet should be dismissed.
  [[EarlGrey selectElementWithMatcher:link_text]
      assertWithMatcher:grey_notVisible()];
}

- (void)DISABLED_testSwipeToDismiss {
  // TODO(crbug.com/40949085): Test fails on iPad.
  if ([ChromeEarlGrey isIPadIdiom]) {
    EARL_GREY_TEST_DISABLED(@"Fails on iPad.");
  }

  [self openCreatePlusAddressBottomSheet];

  id<GREYMatcher> emailDescription =
      GetMatcherForEmailDescription(_fakeIdentity.userEmail);
  // The primary email address should be shown.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:emailDescription];

  // Then, swipe down on the bottom sheet.
  [[EarlGrey selectElementWithMatcher:emailDescription]
      performAction:grey_swipeSlowInDirection(kGREYDirectionDown)];
  // It should no longer be shown.
  [[EarlGrey selectElementWithMatcher:emailDescription]
      assertWithMatcher:grey_notVisible()];

  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalShown, 1);
  // TODO(crbug.com/40276862): separate out the cancel click from other exit
  // patterns, on all platforms.
  ExpectModalHistogram(
      plus_addresses::metrics::PlusAddressModalEvent::kModalCanceled, 1);
  ExpectModalTimeSample(
      plus_addresses::metrics::PlusAddressModalCompletionStatus::
          kReservePlusAddressError,
      1);
}

// A test to ensure that a row in the settings view shows up for
// plus_addresses, and that tapping it opens a new tab for its settings, which
// are managed externally.
- (void)testSettings {
  [ChromeEarlGreyUI openSettingsMenu];
  // Take note of how many tabs are open before clicking the link in settings,
  // which should simply open a new tab.
  NSUInteger oldRegularTabCount = [ChromeEarlGrey mainTabCount];
  NSUInteger oldIncognitoTabCount = [ChromeEarlGrey incognitoTabCount];
  [ChromeEarlGreyUI
      tapSettingsMenuButton:grey_accessibilityID(kSettingsPlusAddressesId)];

  // A new tab should open after tapping the link.
  [ChromeEarlGrey waitForMainTabCount:oldRegularTabCount + 1];
  [ChromeEarlGrey waitForIncognitoTabCount:oldIncognitoTabCount];
}

// A test to check the refresh plus address functionality.
- (void)DISABLED_testRefresh {
  [self openCreatePlusAddressBottomSheet];

  id<GREYMatcher> plusAddressLabelMatcher = GetMatcherForPlusAddressLabel(
      base::SysUTF8ToNSString(plus_addresses::test::kFakePlusAddress));
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:plusAddressLabelMatcher];

  id<GREYMatcher> refreshButton = grey_allOf(
      grey_accessibilityID(kPlusAddressRefreshButtonAccessibilityIdentifier),
      grey_accessibilityTrait(UIAccessibilityTraitButton), nil);

  // Tap on the refresh button
  [[EarlGrey selectElementWithMatcher:refreshButton] performAction:grey_tap()];

  id<GREYMatcher> refreshed_plus_address = GetMatcherForPlusAddressLabel(
      base::SysUTF8ToNSString(plus_addresses::test::kFakePlusAddressRefresh));
  // Test that the plus address has been refreshed.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:refreshed_plus_address];

  // Ensure the cancel button is shown.
  id<GREYMatcher> cancelButton =
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_PLUS_ADDRESS_MODAL_CANCEL_TEXT);

  // Click the cancel button, dismissing the bottom sheet.
  [[EarlGrey selectElementWithMatcher:cancelButton] performAction:grey_tap()];
}

// A test to check the plus address create suggestion IPH feature.
- (void)DISABLED_testCreatePlusAddressIPH {
  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];
  id<GREYMatcher> iph_chip = grey_text(
      l10n_util::GetNSString(IDS_PLUS_ADDRESS_CREATE_SUGGESTION_IPH_IOS));

  // Ensure the plus_address suggestion IPH appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:iph_chip];
}

@end
