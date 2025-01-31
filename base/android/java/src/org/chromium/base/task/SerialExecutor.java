// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.task;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.ArrayDeque;
import java.util.concurrent.Executor;

@NullMarked
class SerialExecutor implements Executor {
    final ArrayDeque<Runnable> mTasks = new ArrayDeque<Runnable>();
    @Nullable Runnable mActive;

    @Override
    public synchronized void execute(final Runnable r) {
        mTasks.offer(
                () -> {
                    try {
                        r.run();
                    } finally {
                        scheduleNext();
                    }
                });
        if (mActive == null) {
            scheduleNext();
        }
    }

    protected synchronized void scheduleNext() {
        if ((mActive = mTasks.poll()) != null) {
            AsyncTask.THREAD_POOL_EXECUTOR.execute(mActive);
        }
    }
}
