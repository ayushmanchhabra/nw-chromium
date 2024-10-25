// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Basic generate bid script that offers a bid of 1 using the first ad's
// `renderURL`.
function generateBid(
    interestGroup, auctionSignals, perBuyerSignals, trustedBiddingSignals,
    browserSignals) {
  const ad = interestGroup.ads[0];
  const bid = interestGroup.name === 'winner' ? 2 : 1;
  realTimeReporting.contributeToHistogram({bucket: 101, priorityWeight: 0.5});
  return {'ad': interestGroup.name, 'bid': bid, 'render': ad.renderURL};
}

function reportWin(
    auctionSignals, perBuyerSignals, sellerSignals, browserSignals) {
  sendReportTo(
      browserSignals.interestGroupOwner + '/echoall?report_bidder/' +
      browserSignals.interestGroupName);
}
