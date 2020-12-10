//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012-2014 Ripple Labs Inc.

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
#include <divvy/protocol/ErrorCodes.h>
#include <divvy/resource/Fees.h>
#include <divvy/rpc/Context.h>
#include <divvy/rpc/impl/TransactionSign.h>

namespace divvy {

// {
//   tx_json: <object>,
//   secret: <secret>
// }
Json::Value doSign (RPC::Context& context)
{
    if (context.role != Role::ADMIN && !context.app.config().canSign())
    {
        return RPC::make_error (rpcNOT_SUPPORTED,
            "Signing is not supported by this server.");
    }

    context.loadType = Resource::feeHighBurdenRPC;
    NetworkOPs::FailHard const failType =
        NetworkOPs::doFailHard (
            context.params.isMember (jss::fail_hard)
            && context.params[jss::fail_hard].asBool ());

    auto ret = RPC::transactionSign (
        context.params, failType, context.role,
        context.ledgerMaster.getValidatedLedgerAge(), context.app);

    ret[jss::deprecated] = "This command has been deprecated and will be "
                           "removed in a future version of the server. Please "
                           "migrate to a standalone signing tool.";

    return ret;
}

} // divvy
