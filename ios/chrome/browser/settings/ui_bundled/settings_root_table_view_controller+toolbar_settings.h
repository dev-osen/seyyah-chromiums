// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_SETTINGS_ROOT_TABLE_VIEW_CONTROLLER_TOOLBAR_SETTINGS_H_
#define IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_SETTINGS_ROOT_TABLE_VIEW_CONTROLLER_TOOLBAR_SETTINGS_H_

#import "ios/chrome/browser/settings/ui_bundled/settings_root_table_view_controller.h"

// This category adds a method to SettingsRootTableViewController to create
// a Settings button for the toolbar.
@interface SettingsRootTableViewController (ToolbarSettings)

// Builds and returns a Settings button for the toolbar.
- (UIBarButtonItem*)settingsButtonWithAction:(SEL)action;

@end

#endif  // IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_SETTINGS_ROOT_TABLE_VIEW_CONTROLLER_TOOLBAR_SETTINGS_H_
