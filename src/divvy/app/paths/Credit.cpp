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

#include <divvy/ledger/ReadView.h>
#include <divvy/protocol/AmountConversions.h>
#include <divvy/protocol/STAmount.h>
#include <divvy/protocol/Indexes.h>

namespace divvy {

STAmount
creditLimit (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency)
{
    STAmount result ({currency, account});

    auto sleDivvyState = view.read(
        keylet::line(account, issuer, currency));

    if (sleDivvyState)
    {
        result = sleDivvyState->getFieldAmount (
            account < issuer ? sfLowLimit : sfHighLimit);
        result.setIssuer (account);
    }

    assert (result.getIssuer () == account);
    assert (result.getCurrency () == currency);
    return result;
}

IOUAmount
creditLimit2 (
    ReadView const& v,
    AccountID const& acc,
    AccountID const& iss,
    Currency const& cur)
{
    return toAmount<IOUAmount> (creditLimit (v, acc, iss, cur));
}

STAmount creditBalance (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency)
{
    STAmount result ({currency, account});

    auto sleDivvyState = view.read(
        keylet::line(account, issuer, currency));

    if (sleDivvyState)
    {
        result = sleDivvyState->getFieldAmount (sfBalance);
        if (account < issuer)
            result.negate ();
        result.setIssuer (account);
    }

    assert (result.getIssuer () == account);
    assert (result.getCurrency () == currency);
    return result;
}

} // divvy
