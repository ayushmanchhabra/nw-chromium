// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_database_factory.h"

#include "chrome/browser/sync/data_type_store_service_factory.h"
#include "components/sync/model/data_type_store.h"
#include "components/sync/model/data_type_store_service.h"

namespace webapk {
WebApkDatabaseFactory::WebApkDatabaseFactory(Profile* profile)
    : profile_(profile) {}

WebApkDatabaseFactory::~WebApkDatabaseFactory() = default;

syncer::OnceDataTypeStoreFactory WebApkDatabaseFactory::GetStoreFactory() {
  return DataTypeStoreServiceFactory::GetForProfile(profile_)
      ->GetStoreFactory();
}

}  // namespace webapk
