// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/351564777): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "ppapi/tests/test_x509_certificate_private.h"

#include <stdint.h>

#include <cmath>
#include <limits>

#include "ppapi/cpp/private/x509_certificate_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(X509CertificatePrivate);

namespace {

bool FieldMatchesString(
    const pp::X509CertificatePrivate& certificate,
    PP_X509Certificate_Private_Field field,
    const std::string& expected) {
  pp::Var field_value = certificate.GetField(field);
  if (!field_value.is_string())
    return false;
  return field_value.AsString() == expected;
}

bool FieldMatchesDouble(
    const pp::X509CertificatePrivate& certificate,
    PP_X509Certificate_Private_Field field,
    double expected) {
  pp::Var field_value = certificate.GetField(field);
  if (!field_value.is_double())
    return false;
  return std::fabs(field_value.AsDouble() - expected) <=
      std::numeric_limits<double>::epsilon();
}

bool FieldMatchesBuffer(
    const pp::X509CertificatePrivate& certificate,
    PP_X509Certificate_Private_Field field,
    const char* expected,
    uint32_t expected_length) {
  pp::Var field_value = certificate.GetField(field);
  if (!field_value.is_array_buffer())
    return false;
  pp::VarArrayBuffer array_buffer(field_value);
  char* bytes = static_cast<char*>(array_buffer.Map());
  uint32_t length = array_buffer.ByteLength();
  if (length != expected_length)
    return false;
  return std::equal(expected, expected + expected_length, bytes);
}

bool FieldIsNull(
    const pp::X509CertificatePrivate& certificate,
    PP_X509Certificate_Private_Field field) {
  return certificate.GetField(field).is_null();
}

// Google's cert.
const unsigned char kGoogleDer[] = {
  0x30, 0x82, 0x03, 0x21, 0x30, 0x82, 0x02, 0x8a, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x10, 0x01, 0x2a, 0x39, 0x76, 0x0d, 0x3f, 0x4f, 0xc9, 0x0b,
  0xe7, 0xbd, 0x2b, 0xcf, 0x95, 0x2e, 0x7a, 0x30, 0x0d, 0x06, 0x09, 0x2a,
  0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x4c,
  0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x5a,
  0x41, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x1c,
  0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x43, 0x6f, 0x6e, 0x73, 0x75,
  0x6c, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x28, 0x50, 0x74, 0x79, 0x29, 0x20,
  0x4c, 0x74, 0x64, 0x2e, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04,
  0x03, 0x13, 0x0d, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x53, 0x47,
  0x43, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x39, 0x30, 0x33,
  0x32, 0x37, 0x32, 0x32, 0x32, 0x30, 0x30, 0x37, 0x5a, 0x17, 0x0d, 0x31,
  0x30, 0x30, 0x33, 0x32, 0x37, 0x32, 0x32, 0x32, 0x30, 0x30, 0x37, 0x5a,
  0x30, 0x68, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13,
  0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08,
  0x13, 0x0a, 0x43, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61,
  0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x0d, 0x4d,
  0x6f, 0x75, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x20, 0x56, 0x69, 0x65, 0x77,
  0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0a, 0x47,
  0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x20, 0x49, 0x6e, 0x63, 0x31, 0x17, 0x30,
  0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0e, 0x77, 0x77, 0x77, 0x2e,
  0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x81,
  0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
  0x01, 0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89, 0x02,
  0x81, 0x81, 0x00, 0xd6, 0xb9, 0xe1, 0xad, 0xb8, 0x61, 0x0b, 0x1f, 0x4e,
  0xb6, 0x3c, 0x09, 0x3d, 0xab, 0xe8, 0xe3, 0x2b, 0xb6, 0xe8, 0xa4, 0x3a,
  0x78, 0x2f, 0xd3, 0x51, 0x20, 0x22, 0x45, 0x95, 0xd8, 0x00, 0x91, 0x33,
  0x9a, 0xa7, 0xa2, 0x48, 0xea, 0x30, 0x57, 0x26, 0x97, 0x66, 0xc7, 0x5a,
  0xef, 0xf1, 0x9b, 0x0c, 0x3f, 0xe1, 0xb9, 0x7f, 0x7b, 0xc3, 0xc7, 0xcc,
  0xaf, 0x9c, 0xd0, 0x1f, 0x3c, 0x81, 0x15, 0x10, 0x58, 0xfc, 0x06, 0xb3,
  0xbf, 0xbc, 0x9c, 0x02, 0xb9, 0x51, 0xdc, 0xfb, 0xa6, 0xb9, 0x17, 0x42,
  0xe6, 0x46, 0xe7, 0x22, 0xcf, 0x6c, 0x27, 0x10, 0xfe, 0x54, 0xe6, 0x92,
  0x6c, 0x0c, 0x60, 0x76, 0x9a, 0xce, 0xf8, 0x7f, 0xac, 0xb8, 0x5a, 0x08,
  0x4a, 0xdc, 0xb1, 0x64, 0xbd, 0xa0, 0x74, 0x41, 0xb2, 0xac, 0x8f, 0x86,
  0x9d, 0x1a, 0xde, 0x58, 0x09, 0xfd, 0x6c, 0x0a, 0x25, 0xe0, 0x79, 0x02,
  0x03, 0x01, 0x00, 0x01, 0xa3, 0x81, 0xe7, 0x30, 0x81, 0xe4, 0x30, 0x28,
  0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x21, 0x30, 0x1f, 0x06, 0x08, 0x2b,
  0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06, 0x01,
  0x05, 0x05, 0x07, 0x03, 0x02, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x86,
  0xf8, 0x42, 0x04, 0x01, 0x30, 0x36, 0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04,
  0x2f, 0x30, 0x2d, 0x30, 0x2b, 0xa0, 0x29, 0xa0, 0x27, 0x86, 0x25, 0x68,
  0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72, 0x6c, 0x2e, 0x74, 0x68,
  0x61, 0x77, 0x74, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x54, 0x68, 0x61,
  0x77, 0x74, 0x65, 0x53, 0x47, 0x43, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x6c,
  0x30, 0x72, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01,
  0x04, 0x66, 0x30, 0x64, 0x30, 0x22, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x01, 0x86, 0x16, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65,
  0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x3e, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x02, 0x86, 0x32, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x77, 0x77, 0x77, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65, 0x2e,
  0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f,
  0x72, 0x79, 0x2f, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x5f, 0x53, 0x47,
  0x43, 0x5f, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x0c, 0x06, 0x03,
  0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0d,
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05,
  0x00, 0x03, 0x81, 0x81, 0x00, 0x39, 0xb6, 0xfb, 0x11, 0xbc, 0x33, 0x2c,
  0xc3, 0x90, 0x48, 0xe3, 0x6e, 0xc3, 0x9b, 0x38, 0xb1, 0x42, 0xd1, 0x00,
  0x09, 0x58, 0x63, 0xa0, 0xe1, 0x98, 0x1c, 0x85, 0xf2, 0xef, 0x10, 0x1d,
  0x60, 0x4e, 0x51, 0x09, 0x62, 0xf5, 0x05, 0xbd, 0x9d, 0x4f, 0x87, 0x6c,
  0x98, 0x72, 0x07, 0x80, 0xc3, 0x59, 0x48, 0x14, 0xe2, 0xd6, 0xef, 0xd0,
  0x8f, 0x33, 0x6a, 0x68, 0x31, 0xfa, 0xb7, 0xbb, 0x85, 0xcc, 0xf7, 0xc7,
  0x47, 0x7b, 0x67, 0x93, 0x3c, 0xc3, 0x16, 0x51, 0x9b, 0x6f, 0x87, 0x20,
  0xfd, 0x67, 0x4c, 0x2b, 0xea, 0x6a, 0x49, 0xdb, 0x11, 0xd1, 0xbd, 0xd7,
  0x95, 0x22, 0x43, 0x7a, 0x06, 0x7b, 0x4e, 0xf6, 0x37, 0x8e, 0xa2, 0xb9,
  0xcf, 0x1f, 0xa5, 0xd2, 0xbd, 0x3b, 0x04, 0x97, 0x39, 0xb3, 0x0f, 0xfa,
  0x38, 0xb5, 0xaf, 0x55, 0x20, 0x88, 0x60, 0x93, 0xf2, 0xde, 0xdb, 0xff,
  0xdf
};

}  // namespace

TestX509CertificatePrivate::TestX509CertificatePrivate(
    TestingInstance* instance)
    : TestCase(instance) {
}

bool TestX509CertificatePrivate::Init() {
  if (!pp::X509CertificatePrivate::IsAvailable())
    return false;

  return true;
}

void TestX509CertificatePrivate::RunTests(const std::string& filter) {
  RUN_TEST_FORCEASYNC_AND_NOT(ValidCertificate, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(InvalidCertificate, filter);
}

std::string TestX509CertificatePrivate::TestValidCertificate() {
  pp::X509CertificatePrivate certificate(instance_);
  bool successful = certificate.Initialize(
      reinterpret_cast<const char*>(kGoogleDer), sizeof(kGoogleDer));
  ASSERT_TRUE(successful);

  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_COMMON_NAME, "www.google.com"));
  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_LOCALITY_NAME, "Mountain View"));
  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_STATE_OR_PROVINCE_NAME, "California"));
  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_COUNTRY_NAME, "US"));
  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_ORGANIZATION_NAME, "Google Inc"));
  ASSERT_TRUE(FieldMatchesString(
      certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_ORGANIZATION_UNIT_NAME,
      std::string()));

  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_COMMON_NAME, "Thawte SGC CA"));
  ASSERT_TRUE(
      FieldMatchesString(certificate,
                         PP_X509CERTIFICATE_PRIVATE_ISSUER_LOCALITY_NAME,
                         std::string()));
  ASSERT_TRUE(FieldMatchesString(
      certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_STATE_OR_PROVINCE_NAME,
      std::string()));
  ASSERT_TRUE(FieldMatchesString(
      certificate, PP_X509CERTIFICATE_PRIVATE_ISSUER_COUNTRY_NAME, "ZA"));
  ASSERT_TRUE(FieldMatchesString(certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_ORGANIZATION_NAME,
      "Thawte Consulting (Pty) Ltd."));
  ASSERT_TRUE(FieldMatchesString(
      certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_ORGANIZATION_UNIT_NAME,
      std::string()));

  ASSERT_FALSE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SERIAL_NUMBER));
  ASSERT_TRUE(FieldMatchesDouble(certificate,
      PP_X509CERTIFICATE_PRIVATE_VALIDITY_NOT_BEFORE, 1238192407));
  ASSERT_TRUE(FieldMatchesDouble(certificate,
      PP_X509CERTIFICATE_PRIVATE_VALIDITY_NOT_AFTER, 1269728407));
  ASSERT_TRUE(FieldMatchesBuffer(certificate,
      PP_X509CERTIFICATE_PRIVATE_RAW,
      reinterpret_cast<const char*>(kGoogleDer), sizeof(kGoogleDer)));

  // Check unimplemented fields return null.
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_UNIQUE_ID));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_UNIQUE_ID));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_DISTINGUISHED_NAME));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_ISSUER_DISTINGUISHED_NAME));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_VERSION));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SIGNATURE_ALGORITHM_OID));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SIGNATURE_ALGORITHM_PARAMATERS_RAW));
  ASSERT_TRUE(FieldIsNull(certificate,
      PP_X509CERTIFICATE_PRIVATE_SUBJECT_PUBLIC_KEY));

  PASS();
}

std::string TestX509CertificatePrivate::TestInvalidCertificate() {
  bool successful;
  pp::X509CertificatePrivate null_certificate(instance_);
  successful = null_certificate.Initialize(NULL, 0);
  ASSERT_FALSE(successful);

  pp::X509CertificatePrivate bad_certificate(instance_);
  successful = bad_certificate.Initialize("acbde", 0);
  ASSERT_FALSE(successful);

  PASS();
}
