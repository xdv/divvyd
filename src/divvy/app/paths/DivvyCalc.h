//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef DIVVY_APP_PATHS_DIVVYCALC_H_INCLUDED
#define DIVVY_APP_PATHS_DIVVYCALC_H_INCLUDED

#include <divvy/ledger/PaymentSandbox.h>
#include <divvy/app/paths/PathState.h>
#include <divvy/basics/Log.h>
#include <divvy/protocol/STAmount.h>
#include <divvy/protocol/TER.h>

#include <boost/container/flat_set.hpp>

namespace divvy {
class Config;
namespace path {

namespace detail {
struct FlowDebugInfo;
}

/** DivvyCalc calculates the quality of a payment path.

    Quality is the amount of input required to produce a given output along a
    specified path - another name for this is exchange rate.
*/
class DivvyCalc
{
public:
    struct Input
    {
        explicit Input() = default;

        bool partialPaymentAllowed = false;
        bool defaultPathsAllowed = true;
        bool limitQuality = false;
        bool isLedgerOpen = true;
    };
    struct Output
    {
        explicit Output() = default;

        // The computed input amount.
        STAmount actualAmountIn;

        // The computed output amount.
        STAmount actualAmountOut;

        // Collection of offers found expired or unfunded. When a payment
        // succeeds, unfunded and expired offers are removed. When a payment
        // fails, they are not removed. This vector contains the offers that
        // could have been removed but were not because the payment fails. It is
        // useful for offer crossing, which does remove the offers.
        boost::container::flat_set<uint256> removableOffers;
    private:
        TER calculationResult_ = temUNKNOWN;

    public:
        TER result () const
        {
            return calculationResult_;
        }
        void setResult (TER const value)
        {
            calculationResult_ = value;
        }

    };

    static
    Output
    divvyCalculate(
        PaymentSandbox& view,

        // Compute paths using this ledger entry set.  Up to caller to actually
        // apply to ledger.

        // Issuer:
        //      XDV: xdvAccount()
        //  non-XDV: uSrcAccountID (for any issuer) or another account with
        //           trust node.
        STAmount const& saMaxAmountReq,             // --> -1 = no limit.

        // Issuer:
        //      XDV: xdvAccount()
        //  non-XDV: uDstAccountID (for any issuer) or another account with
        //           trust node.
        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,

        // A set of paths that are included in the transaction that we'll
        // explore for liquidity.
        STPathSet const& spsPaths,
        Logs& l,
        Input const* const pInputs = nullptr);

    // The view we are currently working on
    PaymentSandbox& view;

    // If the transaction fails to meet some constraint, still need to delete
    // unfunded offers in a deterministic order (hence the ordered container).
    //
    // Offers that were found unfunded.
    boost::container::flat_set<uint256> permanentlyUnfundedOffers_;

    // First time working in reverse a funding source was mentioned.  Source may
    // only be used there.

    // Map of currency, issuer to node index.
    AccountIssueToNodeIndex mumSource_;
    beast::Journal j_;
    Logs& logs_;

private:
    DivvyCalc (
        PaymentSandbox& view_,
        STAmount const& saMaxAmountReq,             // --> -1 = no limit.
        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,
        STPathSet const& spsPaths,
        Logs& l)
            : view (view_),
              j_ (l.journal ("DivvyCalc")),
              logs_ (l),
              saDstAmountReq_(saDstAmountReq),
              saMaxAmountReq_(saMaxAmountReq),
              uDstAccountID_(uDstAccountID),
              uSrcAccountID_(uSrcAccountID),
              spsPaths_(spsPaths)
    {
    }

    /** Compute liquidity through these path sets. */
    TER divvyCalculate (detail::FlowDebugInfo* flowDebugInfo=nullptr);

    /** Add a single PathState.  Returns true on success.*/
    bool addPathState(STPath const&, TER&);

    STAmount const& saDstAmountReq_;
    STAmount const& saMaxAmountReq_;
    AccountID const& uDstAccountID_;
    AccountID const& uSrcAccountID_;
    STPathSet const& spsPaths_;

    // The computed input amount.
    STAmount actualAmountIn_;

    // The computed output amount.
    STAmount actualAmountOut_;

    // Expanded path with all the actual nodes in it.
    // A path starts with the source account, ends with the destination account
    // and goes through other acounts or order books.
    PathState::List pathStateList_;

    Input inputFlags;
};

} // path
} // divvy

#endif
