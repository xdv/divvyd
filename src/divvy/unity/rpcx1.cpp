//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012, 2013 Divvy Labs Inc.

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


// This has to be included early to prevent an obscure MSVC compile error
#include <boost/asio/deadline_timer.hpp>

#include <divvy/protocol/JsonFields.h>
#include <divvy/rpc/RPCHandler.h>
#include <divvy/rpc/handlers/Handlers.h>

#include <divvy/rpc/handlers/AccountCurrenciesHandler.cpp>
#include <divvy/rpc/handlers/AccountInfo.cpp>
#include <divvy/rpc/handlers/AccountLines.cpp>
#include <divvy/rpc/handlers/AccountChannels.cpp>
#include <divvy/rpc/handlers/AccountObjects.cpp>
#include <divvy/rpc/handlers/AccountOffers.cpp>
#include <divvy/rpc/handlers/AccountTx.cpp>
#include <divvy/rpc/handlers/AccountTxOld.cpp>
#include <divvy/rpc/handlers/AccountTxSwitch.cpp>
#include <divvy/rpc/handlers/BlackList.cpp>
#include <divvy/rpc/handlers/BookOffers.cpp>
#include <divvy/rpc/handlers/CanDelete.cpp>
#include <divvy/rpc/handlers/Connect.cpp>
#include <divvy/rpc/handlers/ConsensusInfo.cpp>
#include <divvy/rpc/handlers/DepositAuthorized.cpp>
#include <divvy/rpc/handlers/DownloadShard.cpp>
#include <divvy/rpc/handlers/Feature1.cpp>
#include <divvy/rpc/handlers/Fee1.cpp>
#include <divvy/rpc/handlers/FetchInfo.cpp>
#include <divvy/rpc/handlers/GatewayBalances.cpp>
#include <divvy/rpc/handlers/GetCounts.cpp>
#include <divvy/rpc/handlers/LedgerHandler.cpp>
#include <divvy/rpc/handlers/LedgerAccept.cpp>
#include <divvy/rpc/handlers/LedgerCleanerHandler.cpp>
#include <divvy/rpc/handlers/LedgerClosed.cpp>
#include <divvy/rpc/handlers/LedgerCurrent.cpp>
#include <divvy/rpc/handlers/LedgerData.cpp>
#include <divvy/rpc/handlers/LedgerEntry.cpp>
#include <divvy/rpc/handlers/LedgerHeader.cpp>
#include <divvy/rpc/handlers/LedgerRequest.cpp>
#include <divvy/rpc/handlers/LogLevel.cpp>
#include <divvy/rpc/handlers/LogRotate.cpp>
#include <divvy/rpc/handlers/NoDivvyCheck.cpp>
#include <divvy/rpc/handlers/OwnerInfo.cpp>
