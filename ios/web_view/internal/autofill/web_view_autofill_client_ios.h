// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_

#import <memory>
#import <string>
#import <vector>

#import "base/memory/raw_ptr.h"
#import "base/memory/weak_ptr.h"
#import "components/autofill/core/browser/autocomplete_history_manager.h"
#import "components/autofill/core/browser/autofill_client.h"
#import "components/autofill/core/browser/crowdsourcing/autofill_crowdsourcing_manager.h"
#import "components/autofill/core/browser/crowdsourcing/votes_uploader.h"
#import "components/autofill/core/browser/data_manager/personal_data_manager.h"
#import "components/autofill/core/browser/logging/log_manager.h"
#import "components/autofill/core/browser/metrics/form_interactions_ukm_logger.h"
#import "components/autofill/core/browser/single_field_fill_router.h"
#import "components/autofill/core/browser/strike_databases/strike_database.h"
#import "components/prefs/pref_service.h"
#import "components/sync/service/sync_service.h"
#import "ios/web/public/web_state.h"
#import "ios/web_view/internal/autofill/cwv_autofill_client_ios_bridge.h"
#import "ios/web_view/internal/autofill/ios_web_view_payments_autofill_client.h"
#import "ios/web_view/internal/web_view_browser_state.h"

namespace autofill {

class LogRouter;
enum class SuggestionType;

namespace payments {
class IOSWebViewPaymentsAutofillClient;
}  // namespace payments

// WebView implementation of AutofillClient.
class WebViewAutofillClientIOS : public AutofillClient {
 public:
  static std::unique_ptr<WebViewAutofillClientIOS> Create(
      web::WebState* web_state,
      ios_web_view::WebViewBrowserState* browser_state);

  WebViewAutofillClientIOS(
      PrefService* pref_service,
      PersonalDataManager* personal_data_manager,
      AutocompleteHistoryManager* autocomplete_history_manager,
      web::WebState* web_state,
      signin::IdentityManager* identity_manager,
      StrikeDatabase* strike_database,
      syncer::SyncService* sync_service,
      LogRouter* log_router);

  WebViewAutofillClientIOS(const WebViewAutofillClientIOS&) = delete;
  WebViewAutofillClientIOS& operator=(const WebViewAutofillClientIOS&) = delete;

  ~WebViewAutofillClientIOS() override;

  // AutofillClient:
  base::WeakPtr<AutofillClient> GetWeakPtr() override;
  const std::string& GetAppLocale() const override;
  bool IsOffTheRecord() const override;
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory() override;
  AutofillDriverFactory& GetAutofillDriverFactory() override;
  AutofillCrowdsourcingManager& GetCrowdsourcingManager() override;
  VotesUploader& GetVotesUploader() override;
  PersonalDataManager& GetPersonalDataManager() override;
  SingleFieldFillRouter& GetSingleFieldFillRouter() override;
  AutocompleteHistoryManager* GetAutocompleteHistoryManager() override;
  PrefService* GetPrefs() override;
  const PrefService* GetPrefs() const override;
  syncer::SyncService* GetSyncService() override;
  signin::IdentityManager* GetIdentityManager() override;
  const signin::IdentityManager* GetIdentityManager() const override;
  FormDataImporter* GetFormDataImporter() override;
  payments::PaymentsAutofillClient* GetPaymentsAutofillClient() override;
  StrikeDatabase* GetStrikeDatabase() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  AddressNormalizer* GetAddressNormalizer() override;
  const GURL& GetLastCommittedPrimaryMainFrameURL() const override;
  url::Origin GetLastCommittedPrimaryMainFrameOrigin() const override;
  security_state::SecurityLevel GetSecurityLevelForUmaHistograms() override;
  const translate::LanguageState* GetLanguageState() override;
  translate::TranslateDriver* GetTranslateDriver() override;
  void ShowAutofillSettings(SuggestionType suggestion_type) override;
  void ConfirmSaveAddressProfile(
      const AutofillProfile& profile,
      const AutofillProfile* original_profile,
      bool is_migration_to_account,
      AddressProfileSavePromptCallback callback) override;
  SuggestionUiSessionId ShowAutofillSuggestions(
      const AutofillClient::PopupOpenArgs& open_args,
      base::WeakPtr<AutofillSuggestionDelegate> delegate) override;
  void UpdateAutofillDataListValues(
      base::span<const autofill::SelectOption> datalist) override;
  void PinAutofillSuggestions() override;
  void HideAutofillSuggestions(SuggestionHidingReason reason) override;
  bool IsAutofillEnabled() const override;
  bool IsAutofillProfileEnabled() const override;
  bool IsAutofillPaymentMethodsEnabled() const override;
  bool IsAutocompleteEnabled() const override;
  bool IsPasswordManagerEnabled() const override;
  void DidFillForm(AutofillTriggerSource trigger_source,
                   bool is_refill) override;
  bool IsContextSecure() const override;
  autofill::FormInteractionsFlowId GetCurrentFormInteractionsFlowId() override;
  autofill_metrics::FormInteractionsUkmLogger& GetFormInteractionsUkmLogger()
      override;
  bool IsLastQueriedField(FieldGlobalId field_id) override;

  LogManager* GetCurrentLogManager() override;

  void set_bridge(id<CWVAutofillClientIOSBridge> bridge);

 private:
  raw_ptr<PrefService> pref_service_;
  std::unique_ptr<AutofillCrowdsourcingManager> crowdsourcing_manager_;
  std::unique_ptr<VotesUploader> votes_uploader_;
  PersonalDataManager* personal_data_manager_;
  AutocompleteHistoryManager* autocomplete_history_manager_;
  web::WebState* web_state_;
  __weak id<CWVAutofillClientIOSBridge> bridge_;
  raw_ptr<signin::IdentityManager> identity_manager_;
  std::unique_ptr<FormDataImporter> form_data_importer_;
  StrikeDatabase* strike_database_;
  syncer::SyncService* sync_service_ = nullptr;
  raw_ptr<LogRouter> log_router_;
  std::unique_ptr<LogManager> log_manager_;
  autofill_metrics::FormInteractionsUkmLogger form_interactions_ukm_logger_{
      this};

  // Order matters for this initialization. This initialization must happen
  // after all of the members passed into the constructor of
  // `payments_autofill_client_` are initialized, other than `this`.
  payments::IOSWebViewPaymentsAutofillClient payments_autofill_client_{
      this, bridge_, web_state_};
  SingleFieldFillRouter single_field_fill_router_{
      autocomplete_history_manager_, payments_autofill_client_.GetIbanManager(),
      payments_autofill_client_.GetMerchantPromoCodeManager()};

  base::WeakPtrFactory<WebViewAutofillClientIOS> weak_ptr_factory_{this};
};

}  // namespace autofill

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
