// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

#import <memory>
#import <utility>

#import "base/check_op.h"
#import "base/files/file_path.h"
#import "base/task/sequenced_task_runner.h"
#import "components/sync_preferences/pref_service_syncable.h"
#import "components/variations/net/variations_http_headers.h"
#import "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#import "ios/components/webui/web_ui_url_constants.h"
#import "ios/web/public/thread/web_thread.h"
#import "ios/web/public/web_state.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/webui/url_data_manager_ios_backend.h"
#import "net/url_request/url_request_context_getter.h"
#import "net/url_request/url_request_interceptor.h"

namespace {
// All ChromeBrowserState will store a dummy base::SupportsUserData::Data
// object with this key. It can be used to check that a web::BrowserState
// is effectively a ChromeBrowserState when converting.
const char kBrowserStateIsChromeBrowserState[] = "IsChromeBrowserState";
}  // namespace

ChromeBrowserState::ChromeBrowserState(
    const base::FilePath& state_path,
    std::string_view profile_name,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : state_path_(state_path),
      profile_name_(profile_name),
      io_task_runner_(std::move(io_task_runner)) {
  DCHECK(io_task_runner_);
  DCHECK(!state_path_.empty());
  SetUserData(kBrowserStateIsChromeBrowserState,
              std::make_unique<base::SupportsUserData::Data>());
}

ChromeBrowserState::~ChromeBrowserState() {}

// static
ChromeBrowserState* ChromeBrowserState::FromBrowserState(
    web::BrowserState* browser_state) {
  if (!browser_state) {
    return nullptr;
  }

  // Check that the BrowserState is a ChromeBrowserState. It should always
  // be true in production and during tests as the only BrowserState that
  // should be used in ios/chrome inherits from ChromeBrowserState.
  DCHECK(browser_state->GetUserData(kBrowserStateIsChromeBrowserState));
  return static_cast<ChromeBrowserState*>(browser_state);
}

// static
ChromeBrowserState* ChromeBrowserState::FromWebUIIOS(web::WebUIIOS* web_ui) {
  return FromBrowserState(web_ui->GetWebState()->GetBrowserState());
}

const std::string& ChromeBrowserState::GetProfileName() const {
  return profile_name_;
}

scoped_refptr<base::SequencedTaskRunner> ChromeBrowserState::GetIOTaskRunner() {
  return io_task_runner_;
}

PrefService* ChromeBrowserState::GetPrefs() {
  return GetSyncablePrefs();
}

base::FilePath ChromeBrowserState::GetOffTheRecordStatePath() const {
  if (IsOffTheRecord()) {
    return state_path_;
  }

  return state_path_.Append(FILE_PATH_LITERAL("OTR"));
}

base::FilePath ChromeBrowserState::GetStatePath() const {
  return state_path_;
}

net::URLRequestContextGetter* ChromeBrowserState::GetRequestContext() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if (!request_context_getter_) {
    ProtocolHandlerMap protocol_handlers;
    protocol_handlers[kChromeUIScheme] =
        web::URLDataManagerIOSBackend::CreateProtocolHandler(this);
    request_context_getter_ =
        base::WrapRefCounted(CreateRequestContext(&protocol_handlers));
  }
  return request_context_getter_.get();
}

void ChromeBrowserState::UpdateCorsExemptHeader(
    network::mojom::NetworkContextParams* params) {
  variations::UpdateCorsExemptHeaderForVariations(params);
}
