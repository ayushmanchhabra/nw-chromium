// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_SYNC_SERVICE_FACTORY_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_SYNC_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace webapk {

class WebApkSyncService;

class WebApkSyncServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  WebApkSyncServiceFactory(const WebApkSyncServiceFactory&) = delete;
  WebApkSyncServiceFactory& operator=(const WebApkSyncServiceFactory&) = delete;

  static WebApkSyncServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<WebApkSyncServiceFactory>;
  friend class WebApkSyncService;

  WebApkSyncServiceFactory();
  ~WebApkSyncServiceFactory() override;

  static WebApkSyncService* GetForProfile(Profile* profile);

  // BrowserContextKeyedServiceFactory
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace webapk

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_SYNC_SERVICE_FACTORY_H_
