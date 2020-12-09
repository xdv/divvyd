//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012-2014 Divvy Labs Inc.

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

#include <divvy/app/ledger/LedgerMaster.h>
#include <divvy/app/misc/NetworkOPs.h>
#include <divvy/json/json_value.h>
#include <divvy/protocol/ErrorCodes.h>
#include <divvy/protocol/JsonFields.h>
#include <divvy/rpc/impl/RPCHelpers.h>
#include <divvy/rpc/Context.h>

namespace divvy {

// {
//   'ident' : <indent>,
// }
Json::Value doOwnerInfo (RPC::Context& context)
{
    if (!context.params.isMember (jss::account) &&
        !context.params.isMember (jss::ident))
    {
        return RPC::missing_field_error (jss::account);
    }

    std::string strIdent = context.params.isMember (jss::account)
            ? context.params[jss::account].asString ()
            : context.params[jss::ident].asString ();
    Json::Value ret;

    // Get info on account.

    auto const& closedLedger = context.ledgerMaster.getClosedLedger ();
    AccountID accountID;
    auto jAccepted = RPC::accountFromString (accountID, strIdent);

    ret[jss::accepted] = ! jAccepted ?
            context.netOps.getOwnerInfo (closedLedger, accountID) : jAccepted;

    auto const& currentLedger = context.ledgerMaster.getCurrentLedger ();
    auto jCurrent = RPC::accountFromString (accountID, strIdent);

    ret[jss::current] = ! jCurrent ?
            context.netOps.getOwnerInfo (currentLedger, accountID) : jCurrent;
    return ret;
}

} // divvy
