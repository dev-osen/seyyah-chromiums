// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class runs CUJ tests for fedCM. Normally fedCM is invoked by javascript
// from webpages, and requires communication with multiple remote endpoints.
// This test suite does not do any of that at the moment.

#include "build/build_config.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/views/webid/account_selection_view_test_base.h"
#include "chrome/browser/ui/views/webid/fake_delegate.h"
#include "chrome/browser/ui/views/webid/fedcm_account_selection_view_desktop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/browser/identity_request_dialog_controller.h"
#include "content/public/test/browser_test.h"
#include "ui/base/interaction/interactive_test.h"

namespace {

class FedCmCUJTest : public InteractiveBrowserTest {
 public:
  auto OpenAccounts(blink::mojom::RpMode mode) {
    return Do([this, mode]() {
      delegate_ = std::make_unique<FakeDelegate>(
          browser()->GetActiveTabInterface()->GetContents());
      account_selection_view_ = std::make_unique<FedCmAccountSelectionView>(
          delegate_.get(), browser()->GetActiveTabInterface());
      idps_ = {base::MakeRefCounted<content::IdentityProviderData>(
          "idp-example.com", content::IdentityProviderMetadata(),
          content::ClientMetadata(GURL(), GURL(), GURL()),
          blink::mojom::RpContext::kSignIn, kDefaultDisclosureFields,
          /*has_login_status_mismatch=*/false)};
      accounts_ = {base::MakeRefCounted<Account>(
          "id", "email", "name", "given_name", GURL(),
          /*login_hints=*/std::vector<std::string>(),
          /*domain_hints=*/std::vector<std::string>(),
          /*labels=*/std::vector<std::string>())};
      accounts_[0]->identity_provider = idps_[0];
      account_selection_view_->Show(
          "rp-example.com", idps_, accounts_, Account::SignInMode::kExplicit,
          mode, /*new_accounts=*/std::vector<IdentityRequestAccountPtr>());
    });
  }

  // Opens the modal version of the account chooser.
  auto OpenAccountsModal() {
    return OpenAccounts(blink::mojom::RpMode::kActive);
  }

  // Opens the bubble version of the account chooser.
  auto OpenAccountsBubble() {
    return OpenAccounts(blink::mojom::RpMode::kPassive);
  }

  auto ShowTabModalUI() {
    return Do([this]() {
      tab_modal_ui_ = browser()->GetActiveTabInterface()->ShowModalUI();
    });
  }
  auto HideTabModalUI() {
    return Do([this]() { tab_modal_ui_.reset(); });
  }

 protected:
  std::unique_ptr<FakeDelegate> delegate_;
  std::vector<IdentityProviderDataPtr> idps_;
  std::vector<IdentityRequestAccountPtr> accounts_;
  std::unique_ptr<FedCmAccountSelectionView> account_selection_view_;
  std::unique_ptr<tabs::ScopedTabModalUI> tab_modal_ui_;
};

// Shows the account picker. Selects an account.
IN_PROC_BROWSER_TEST_F(FedCmCUJTest, SelectAccount) {
  RunTestSequence(OpenAccountsModal(),
                  WaitForShow(kFedCmAccountChooserDialogAccountElementId),
                  PressButton(kFedCmAccountChooserDialogAccountElementId), );
}

// TODO(https://crbug.com/382867817): Fix this on windows.
#if BUILDFLAG(IS_WIN)
#define MAYBE_BubbleHidesWhenModalUIShown DISABLED_BubbleHidesWhenModalUIShown
#else
#define MAYBE_BubbleHidesWhenModalUIShown BubbleHidesWhenModalUIShown
#endif
// Shows the bubble account picker. It should hide when a modal UI is shown. It
// should re-show when the modal UI goes away.
IN_PROC_BROWSER_TEST_F(FedCmCUJTest, MAYBE_BubbleHidesWhenModalUIShown) {
  RunTestSequence(
      OpenAccountsBubble(),
      WaitForShow(kFedCmAccountChooserDialogAccountElementId), ShowTabModalUI(),
      WaitForHide(kFedCmAccountChooserDialogAccountElementId), HideTabModalUI(),
      WaitForShow(kFedCmAccountChooserDialogAccountElementId), );
}

}  // namespace
