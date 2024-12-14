// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FACILITATED_PAYMENTS_CORE_METRICS_FACILITATED_PAYMENTS_METRICS_H_
#define COMPONENTS_FACILITATED_PAYMENTS_CORE_METRICS_FACILITATED_PAYMENTS_METRICS_H_

#include <optional>

#include "base/types/expected.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_utils.h"
#include "components/facilitated_payments/core/validation/payment_link_validator.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace base {
class TimeDelta;
}

namespace payments::facilitated {

// A payment system that is currently running.
enum class FacilitatedPaymentsType {
  kEwallet = 0,
  kPix = 1,
};

// Reasons for why the payflow was exited early. These only include the reasons
// after the renderer has detected a valid code and sent the signal to the
// browser process.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PayflowExitedReason {
  // The code validator encountered an error.
  kCodeValidatorFailed = 0,
  // The code for the payflow is not valid.
  kInvalidCode = 1,
  // The user has opted out of the payflow.
  kUserOptedOut = 2,
  // The user has no linked accounts available for the payflow.
  kNoLinkedAccount = 3,
  // The device is in landscape orientation when payflow was to be triggered.
  kLandscapeScreenOrientation = 4,
  // The API Client is not available when the payflow was to be triggered.
  kApiClientNotAvailable = 5,
  // The risk data needed to send the server request is not available.
  kRiskDataNotAvailable = 6,
  // The client token needed to send the server request is not available.
  kClientTokenNotAvailable = 7,
  // The InitiatePayment response indicated a failure.
  kInitiatePaymentFailed = 8,
  // The action token returned in the InitiatePayment response is not available.
  kActionTokenNotAvailable = 9,
  // The user has logged out after selecting a payment method.
  kUserLoggedOut = 10,
  // The FOP selector either wasn't shown, or was dismissed not as a result of a
  // user action.
  kFopSelectorClosedNotByUser = 11,
  // The FOP selector was dismissed by a user action e.g., swiping down, tapping
  // on the webpage behind the FOP selector, or tapping on the omnibox.
  kFopSelectorClosedByUser = 12,
  kMaxValue = kFopSelectorClosedByUser
};

// Log when a Pix code is copied to the clippboard on an allowlisted merchant
// website.
void LogPixCodeCopied(ukm::SourceId ukm_source_id);

// Log when the FOP selector UI is shown.
void LogFopSelectorShownUkm(ukm::SourceId ukm_source_id);

// Log after user accepts / rejects the FOP UI. The `accepted` will be false
// if the user rejects the UI, and it will be true if the user accepts the
// selector UI and selects a FoP to use.
void LogFopSelectorResultUkm(bool accepted, ukm::SourceId ukm_source_id);

// Log when user selects a FOP to pay with.
void LogFopSelected();

// Log the result and latency for validating a payment code using
// `data_decoder::DataDecoder`.
void LogPaymentCodeValidationResultAndLatency(
    base::expected<bool, std::string> result,
    base::TimeDelta duration);

// Log the result of whether the facilitated payments is available or not and
// the check's latency.
// `payment_type` must be either `kEwallet` or `kPix`.
// The `scheme` parameter is required for the 'kEwallet' payment type and should
// not be `kInvalid`.
void LogApiAvailabilityCheckResultAndLatency(
    FacilitatedPaymentsType payment_type,
    bool result,
    base::TimeDelta duration,
    std::optional<PaymentLinkValidator::Scheme> scheme = std::nullopt);

// Logs the result and latency for fetching the risk data. If the risk data was
// fetched successfully, `was_successful` is true. The call took `duration` to
// complete.
void LogLoadRiskDataResultAndLatency(bool was_successful,
                                     base::TimeDelta duration);

// Log the result and the latency of the GetClientToken call made to api client.
void LogGetClientTokenResultAndLatency(bool result, base::TimeDelta duration);

// Log the reason for the payflow was exited early. This includes all the
// reasons after receiving a signal from the renderer process that a valid code
// has been found.
void LogPayflowExitedReason(PayflowExitedReason reason);

// Log the attempt to send the call to the InitiatePayment backend endpoint.
void LogInitiatePaymentAttempt();

// Log the result and latency for the InitiatePayment backend endpoint.
void LogInitiatePaymentResultAndLatency(bool result, base::TimeDelta duration);

// Log the attempt to send the call to the InitiatePurchaseAction backend
// endpoint.
void LogInitiatePurchaseActionAttempt();

// Log the result and latency for the InitiatePurchaseAction call made to the
// payments platform (client).
void LogInitiatePurchaseActionResultAndLatency(PurchaseActionResult result,
                                               base::TimeDelta duration);

// Log the UKM for the InitiatePurchaseAction result.
void LogInitiatePurchaseActionResultUkm(PurchaseActionResult result,
                                        ukm::SourceId ukm_source_id);

// Logs showing a new UI screen.
void LogUiScreenShown(UiState ui_screen);

// Logs the latency for seeing the Pix FOP selector after a user has copied the
// Pix payment code on the browser.
void LogPixFopSelectorShownLatency(base::TimeDelta latency);

}  // namespace payments::facilitated

#endif  // COMPONENTS_FACILITATED_PAYMENTS_CORE_METRICS_FACILITATED_PAYMENTS_METRICS_H_
