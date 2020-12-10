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

#include <divvy/app/paths/Credit.h>
#include <divvy/app/paths/impl/AmountSpec.h>
#include <divvy/app/paths/impl/Steps.h>
#include <divvy/app/paths/impl/StepChecks.h>
#include <divvy/basics/Log.h>
#include <divvy/ledger/PaymentSandbox.h>
#include <divvy/protocol/IOUAmount.h>
#include <divvy/protocol/Quality.h>
#include <divvy/protocol/XDVAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace divvy {

template <class TDerived>
class XDVEndpointStep : public StepImp<
    XDVAmount, XDVAmount, XDVEndpointStep<TDerived>>
{
private:
    AccountID acc_;
    bool const isLast_;
    beast::Journal j_;

    // Since this step will always be an endpoint in a strand
    // (either the first or last step) the same cache is used
    // for cachedIn and cachedOut and only one will ever be used
    boost::optional<XDVAmount> cache_;

    boost::optional<EitherAmount>
    cached () const
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (*cache_);
    }

public:
    XDVEndpointStep (
        StrandContext const& ctx,
        AccountID const& acc)
            : acc_(acc)
            , isLast_(ctx.isLast)
            , j_ (ctx.j) {}

    AccountID const& acc () const
    {
        return acc_;
    }

    boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const override
    {
        if (isLast_)
            return std::make_pair(xdvAccount(), acc_);
        return std::make_pair(acc_, xdvAccount());
    }

    boost::optional<EitherAmount>
    cachedIn () const override
    {
        return cached ();
    }

    boost::optional<EitherAmount>
    cachedOut () const override
    {
        return cached ();
    }

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<XDVAmount, XDVAmount>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        XDVAmount const& out);

    std::pair<XDVAmount, XDVAmount>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        XDVAmount const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

    // Check for errors and violations of frozen constraints.
    TER check (StrandContext const& ctx) const;

protected:
    XDVAmount
    xdvLiquidImpl (ReadView& sb, std::int32_t reserveReduction) const
    {
        return divvy::xdvLiquid (sb, acc_, reserveReduction, j_);
    }

    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\nAcc: " << acc_;
        return ostr.str ();
    }

private:
    template <class P>
    friend bool operator==(
        XDVEndpointStep<P> const& lhs,
        XDVEndpointStep<P> const& rhs);

    friend bool operator!=(
        XDVEndpointStep const& lhs,
        XDVEndpointStep const& rhs)
    {
        return ! (lhs == rhs);
    }

    bool equal (Step const& rhs) const override
    {
        if (auto ds = dynamic_cast<XDVEndpointStep const*> (&rhs))
        {
            return *this == *ds;
        }
        return false;
    }
};

//------------------------------------------------------------------------------

// Flow is used in two different circumstances for transferring funds:
//  o Payments, and
//  o Offer crossing.
// The rules for handling funds in these two cases are almost, but not
// quite, the same.

// Payment XDVEndpointStep class (not offer crossing).
class XDVEndpointPaymentStep : public XDVEndpointStep<XDVEndpointPaymentStep>
{
public:
    explicit XDVEndpointPaymentStep() = default;

    using XDVEndpointStep<XDVEndpointPaymentStep>::XDVEndpointStep;

    XDVAmount
    xdvLiquid (ReadView& sb) const
    {
        return xdvLiquidImpl (sb, 0);;
    }

    std::string logString () const override
    {
        return logStringImpl ("XDVEndpointPaymentStep");
    }
};

// Offer crossing XDVEndpointStep class (not a payment).
class XDVEndpointOfferCrossingStep :
    public XDVEndpointStep<XDVEndpointOfferCrossingStep>
{
private:

    // For historical reasons, offer crossing is allowed to dig further
    // into the XDV reserve than an ordinary payment.  (I believe it's
    // because the trust line was created after the XDV was removed.)
    // Return how much the reserve should be reduced.
    //
    // Note that reduced reserve only happens if the trust line does not
    // currently exist.
    static std::int32_t computeReserveReduction (
        StrandContext const& ctx, AccountID const& acc)
    {
        if (ctx.isFirst &&
            !ctx.view.read (keylet::line (acc, ctx.strandDeliver)))
                return -1;
        return 0;
    }

public:
    XDVEndpointOfferCrossingStep (
        StrandContext const& ctx, AccountID const& acc)
    : XDVEndpointStep<XDVEndpointOfferCrossingStep> (ctx, acc)
    , reserveReduction_ (computeReserveReduction (ctx, acc))
    {
    }

    XDVAmount
    xdvLiquid (ReadView& sb) const
    {
        return xdvLiquidImpl (sb, reserveReduction_);
    }

    std::string logString () const override
    {
        return logStringImpl ("XDVEndpointOfferCrossingStep");
    }

private:
    std::int32_t const reserveReduction_;
};

//------------------------------------------------------------------------------

template <class TDerived>
inline bool operator==(XDVEndpointStep<TDerived> const& lhs,
    XDVEndpointStep<TDerived> const& rhs)
{
    return lhs.acc_ == rhs.acc_ && lhs.isLast_ == rhs.isLast_;
}

template <class TDerived>
boost::optional<Quality>
XDVEndpointStep<TDerived>::qualityUpperBound(
    ReadView const& v, bool& redeems) const
{
    redeems = this->redeems(v, true);
    return Quality{STAmount::uRateOne};
}


template <class TDerived>
std::pair<XDVAmount, XDVAmount>
XDVEndpointStep<TDerived>::revImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    XDVAmount const& out)
{
    auto const balance = static_cast<TDerived const*>(this)->xdvLiquid (sb);

    auto const result = isLast_ ? out : std::min (balance, out);

    auto& sender = isLast_ ? xdvAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : xdvAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {XDVAmount{beast::zero}, XDVAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<XDVAmount, XDVAmount>
XDVEndpointStep<TDerived>::fwdImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    XDVAmount const& in)
{
    assert (cache_);
    auto const balance = static_cast<TDerived const*>(this)->xdvLiquid (sb);

    auto const result = isLast_ ? in : std::min (balance, in);

    auto& sender = isLast_ ? xdvAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : xdvAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {XDVAmount{beast::zero}, XDVAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<bool, EitherAmount>
XDVEndpointStep<TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.error()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (XDVAmount (beast::zero))};
    }

    assert (in.native);

    auto const& xdvIn = in.xdv;
    auto const balance = static_cast<TDerived const*>(this)->xdvLiquid (sb);

    if (!isLast_ && balance < xdvIn)
    {
        JLOG (j_.error()) << "XDVEndpointStep: Strand re-execute check failed."
            << " Insufficient balance: " << to_string (balance)
            << " Requested: " << to_string (xdvIn);
        return {false, EitherAmount (balance)};
    }

    if (xdvIn != *cache_)
    {
        JLOG (j_.error()) << "XDVEndpointStep: Strand re-execute check failed."
            << " ExpectedIn: " << to_string (*cache_)
            << " CachedIn: " << to_string (xdvIn);
    }
    return {true, in};
}

template <class TDerived>
TER
XDVEndpointStep<TDerived>::check (StrandContext const& ctx) const
{
    if (!acc_)
    {
        JLOG (j_.debug()) << "XDVEndpointStep: specified bad account.";
        return temBAD_PATH;
    }

    auto sleAcc = ctx.view.read (keylet::account (acc_));
    if (!sleAcc)
    {
        JLOG (j_.warn()) << "XDVEndpointStep: can't send or receive XDV from "
                             "non-existent account: "
                          << acc_;
        return terNO_ACCOUNT;
    }

    if (!ctx.isFirst && !ctx.isLast)
    {
        return temBAD_PATH;
    }

    auto& src = isLast_ ? xdvAccount () : acc_;
    auto& dst = isLast_ ? acc_ : xdvAccount();
    auto ter = checkFreeze (ctx.view, src, dst, xdvCurrency ());
    if (ter != tesSUCCESS)
        return ter;

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

namespace test
{
// Needed for testing
bool xdvEndpointStepEqual (Step const& step, AccountID const& acc)
{
    if (auto xs =
        dynamic_cast<XDVEndpointStep<XDVEndpointPaymentStep> const*> (&step))
    {
        return xs->acc () == acc;
    }
    return false;
}
}

//------------------------------------------------------------------------------

std::pair<TER, std::unique_ptr<Step>>
make_XDVEndpointStep (
    StrandContext const& ctx,
    AccountID const& acc)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<XDVEndpointOfferCrossingStep> (ctx, acc);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
    else // payment
    {
        auto paymentStep =
            std::make_unique<XDVEndpointPaymentStep> (ctx, acc);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move (r)};
}

} // divvy
