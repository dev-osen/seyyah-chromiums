// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import type {BrowserProxy, SystemLog} from 'chrome://system/browser_proxy.js';
import {TestBrowserProxy} from 'chrome://webui-test/test_browser_proxy.js';

export class TestAboutSysBrowserProxy extends TestBrowserProxy implements
    BrowserProxy {
  private systemLogs: SystemLog[] = [];

  constructor() {
    super([
      'requestSystemInfo',
    ]);
  }

  setSystemLogs(logs: SystemLog[]) {
    this.systemLogs = logs;
  }

  requestSystemInfo() {
    this.methodCalled('requestSystemInfo');
    return Promise.resolve(this.systemLogs);
  }
}
