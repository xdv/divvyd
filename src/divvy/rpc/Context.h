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

#ifndef DIVVY_RPC_CONTEXT_H_INCLUDED
#define DIVVY_RPC_CONTEXT_H_INCLUDED

#include <divvy/core/Config.h>
#include <divvy/core/JobQueue.h>
#include <divvy/net/InfoSub.h>
#include <divvy/rpc/Role.h>

#include <divvy/beast/utility/Journal.h>

namespace divvy {

class Application;
class NetworkOPs;
class LedgerMaster;

namespace RPC {

/** The context of information needed to call an RPC. */
struct Context
{
    /**
     * Data passed in from HTTP headers.
     */
    struct Headers
    {
        std::string user;
        std::string forwardedFor;
    };

    beast::Journal j;
    Json::Value params;
    Application& app;
    Resource::Charge& loadType;
    NetworkOPs& netOps;
    LedgerMaster& ledgerMaster;
    Resource::Consumer& consumer;
    Role role;
    std::shared_ptr<JobQueue::Coro> coro;
    InfoSub::pointer infoSub;
    Headers headers;
};

} // RPC
} // divvy

#endif
