// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ui.signin.signin_promo;

import android.content.Context;
import android.text.format.DateUtils;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.signin.services.IdentityServicesProvider;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.chrome.browser.signin.services.SigninPreferencesManager;
import org.chromium.chrome.browser.ui.signin.R;
import org.chromium.chrome.browser.ui.signin.SigninAndHistorySyncActivityLauncher;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.identitymanager.ConsentLevel;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.signin.metrics.SigninAccessPoint;

/** {@link SigninPromoDelegate} for ntp signin promo. */
public class NtpSigninPromoDelegate extends SigninPromoDelegate {
    @VisibleForTesting static final int MAX_IMPRESSIONS_NTP = 5;
    // 14 days in hours.
    @VisibleForTesting static final int NTP_SYNC_PROMO_NTP_SINCE_FIRST_TIME_SHOWN_LIMIT_HOURS = 336;
    @VisibleForTesting static final int NTP_SYNC_PROMO_RESET_AFTER_DAYS = 30;

    /**
     * If the signin promo card has been hidden for longer than the {@link
     * #NTP_SYNC_PROMO_RESET_AFTER_DAYS}, resets the impression counts, {@link
     * ChromePreferenceKeys#SIGNIN_PROMO_NTP_FIRST_SHOWN_TIME} and {@link
     * ChromePreferenceKeys#SIGNIN_PROMO_NTP_LAST_SHOWN_TIME} to allow the promo card to show again.
     */
    public static void resetNtpSyncPromoLimitsIfHiddenForTooLong() {
        final long currentTime = System.currentTimeMillis();
        final long resetAfterMs = NTP_SYNC_PROMO_RESET_AFTER_DAYS * DateUtils.DAY_IN_MILLIS;
        final long lastShownTime =
                ChromeSharedPreferences.getInstance()
                        .readLong(ChromePreferenceKeys.SIGNIN_PROMO_NTP_LAST_SHOWN_TIME, 0L);
        if (lastShownTime <= 0) return;

        if (currentTime - lastShownTime >= resetAfterMs) {
            ChromeSharedPreferences.getInstance().writeInt(getPromoShowCountPreferenceName(), 0);
            ChromeSharedPreferences.getInstance()
                    .removeKey(ChromePreferenceKeys.SIGNIN_PROMO_NTP_FIRST_SHOWN_TIME);
            ChromeSharedPreferences.getInstance()
                    .removeKey(ChromePreferenceKeys.SIGNIN_PROMO_NTP_LAST_SHOWN_TIME);
        }
    }

    public NtpSigninPromoDelegate(
            Context context,
            Profile profile,
            SigninAndHistorySyncActivityLauncher launcher,
            Runnable onPromoStateChange) {
        super(context, profile, launcher, onPromoStateChange);
    }

    @Override
    String getTitle() {
        return mContext.getString(R.string.signin_promo_title_ntp_feed_top_promo);
    }

    @Override
    String getDescription() {
        return mContext.getString(R.string.signin_promo_description_ntp_feed_top_promo);
    }

    @Override
    @SigninAccessPoint
    int getAccessPoint() {
        return SigninAccessPoint.NTP_FEED_TOP_PROMO;
    }

    @Override
    void onDismissButtonClicked() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(ChromePreferenceKeys.SIGNIN_PROMO_NTP_PROMO_DISMISSED, true);
    }

    @Override
    boolean canShowPromo(@Nullable CoreAccountInfo visibleAccount) {
        IdentityManager identityManager =
                IdentityServicesProvider.get().getIdentityManager(mProfile);
        SigninManager signinManager = IdentityServicesProvider.get().getSigninManager(mProfile);
        if (identityManager.hasPrimaryAccount(ConsentLevel.SIGNIN)
                || !signinManager.isSigninAllowed()) {
            return false;
        }
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.FORCE_DISABLE_EXTENDED_SYNC_PROMOS)) {
            return false;
        }

        boolean isPromoDismissed =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(ChromePreferenceKeys.SIGNIN_PROMO_NTP_PROMO_DISMISSED, false);
        if (isPromoDismissed) {
            return false;
        }
        if (timeElapsedSinceFirstShownExceedsLimit()) {
            return false;
        }
        if (visibleAccount == null) {
            return true;
        }
        // Don't show the promo if account image is not available yet.
        return identityManager.findExtendedAccountInfoByEmailAddress(visibleAccount.getEmail())
                != null;
    }

    @Override
    void recordImpression() {
        ChromeSharedPreferences.getInstance().incrementInt(getPromoShowCountPreferenceName());
    }

    @Override
    boolean isMaxImpressionsReached() {
        return ChromeSharedPreferences.getInstance().readInt(getPromoShowCountPreferenceName())
                >= MAX_IMPRESSIONS_NTP;
    }

    private static boolean timeElapsedSinceFirstShownExceedsLimit() {
        final long timeSinceFirstShownLimitMs =
                NTP_SYNC_PROMO_NTP_SINCE_FIRST_TIME_SHOWN_LIMIT_HOURS * DateUtils.HOUR_IN_MILLIS;
        final long currentTime = System.currentTimeMillis();
        final long firstShownTime =
                ChromeSharedPreferences.getInstance()
                        .readLong(ChromePreferenceKeys.SIGNIN_PROMO_NTP_FIRST_SHOWN_TIME, 0L);
        return firstShownTime > 0 && currentTime - firstShownTime >= timeSinceFirstShownLimitMs;
    }

    private static String getPromoShowCountPreferenceName() {
        return ChromePreferenceKeys.SYNC_PROMO_SHOW_COUNT.createKey(
                SigninPreferencesManager.SyncPromoAccessPointId.NTP);
    }
}
