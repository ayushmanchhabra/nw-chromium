// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tasks.tab_management;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.LazyOneshotSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.hub.HubManager;
import org.chromium.chrome.browser.hub.Pane;
import org.chromium.chrome.browser.hub.PaneId;
import org.chromium.chrome.browser.tab_ui.TabSwitcher;
import org.chromium.chrome.browser.tabmodel.TabClosureParams;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.tab_groups.TabGroupModelFilter;

/** Helper for closing all tabs via {@link CloseAllTabsDialog}. */
public class CloseAllTabsHelper {
    /** Closes all tabs hiding tab groups. */
    public static void closeAllTabsHidingTabGroups(TabModelSelector tabModelSelector) {
        var filterProvider = tabModelSelector.getTabModelFilterProvider();
        TabClosureParams params = TabClosureParams.closeAllTabs().hideTabGroups(true).build();
        ((TabGroupModelFilter) filterProvider.getTabModelFilter(false)).closeTabs(params);
        ((TabGroupModelFilter) filterProvider.getTabModelFilter(true)).closeTabs(params);
    }

    /**
     * Create a runnable to close all tabs using appropriate animations where applicable.
     *
     * @param hubManagerSupplier The supplier for the manager of the Hub.
     * @param regularTabSwitcherSupplier The supplier for the regular {@link TabSwitcher}.
     * @param incognitoTabSwitcherSupplier The supplier for the incognito {@link TabSwitcher}.
     * @param tabModelSelector The tab model selector for the activity.
     * @param isIncognitoOnly Whether to only close incognito tabs.
     */
    public static Runnable buildCloseAllTabsRunnable(
            LazyOneshotSupplier<HubManager> hubManagerSupplier,
            OneshotSupplier<TabSwitcher> regularTabSwitcherSupplier,
            OneshotSupplier<TabSwitcher> incognitoTabSwitcherSupplier,
            TabModelSelector tabModelSelector,
            boolean isIncognitoOnly) {
        return () ->
                closeAllTabsRunnableImpl(
                        hubManagerSupplier,
                        regularTabSwitcherSupplier,
                        incognitoTabSwitcherSupplier,
                        tabModelSelector,
                        isIncognitoOnly);
    }

    private static void closeAllTabsRunnableImpl(
            LazyOneshotSupplier<HubManager> hubManagerSupplier,
            OneshotSupplier<TabSwitcher> regularTabSwitcherSupplier,
            OneshotSupplier<TabSwitcher> incognitoTabSwitcherSupplier,
            TabModelSelector tabModelSelector,
            boolean isIncognitoOnly) {

        HubState hubState = getHubState(hubManagerSupplier);
        boolean isRegularHubPane = hubState.currentPaneId == PaneId.TAB_SWITCHER;
        boolean isIncognitoHubPane = hubState.currentPaneId == PaneId.INCOGNITO_TAB_SWITCHER;
        boolean isPaneAndCloseCombinationValid =
                (isRegularHubPane && !isIncognitoOnly) || isIncognitoHubPane;
        boolean canShowAnimation = hubState.isVisible && isPaneAndCloseCombinationValid;

        Runnable onAnimationFinished = () -> closeAllTabs(tabModelSelector, isIncognitoOnly);
        if (canShowAnimation && ChromeFeatureList.sGtsCloseTabAnimation.isEnabled()) {
            TabSwitcher tabSwitcher =
                    isIncognitoHubPane
                            ? incognitoTabSwitcherSupplier.get()
                            : regularTabSwitcherSupplier.get();
            assert tabSwitcher != null;
            tabSwitcher.showCloseAllTabsAnimation(onAnimationFinished);
        } else {
            onAnimationFinished.run();
        }
    }

    private static void closeAllTabs(TabModelSelector tabModelSelector, boolean isIncognitoOnly) {
        if (isIncognitoOnly) {
            tabModelSelector
                    .getModel(/* isIncognito= */ true)
                    .closeTabs(TabClosureParams.closeAllTabs().build());
        } else {
            closeAllTabsHidingTabGroups(tabModelSelector);
        }
    }

    private static class HubState {
        public boolean isVisible;
        public @PaneId int currentPaneId = PaneId.COUNT;
    }

    private static HubState getHubState(LazyOneshotSupplier<HubManager> hubManagerSupplier) {
        var state = new HubState();
        if (!hubManagerSupplier.hasValue()) return state;

        HubManager manager = hubManagerSupplier.get();

        state.isVisible = manager.getHubVisibilitySupplier().get();
        @Nullable Pane focusedPane = manager.getPaneManager().getFocusedPaneSupplier().get();
        if (focusedPane != null) {
            state.currentPaneId = focusedPane.getPaneId();
        }
        return state;
    }
}
