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

#include <divvy/app/ledger/AcceptedLedger.h>
#include <divvy/app/ledger/InboundLedgers.h>
#include <divvy/app/ledger/LedgerMaster.h>
#include <divvy/app/main/Application.h>
#include <divvy/app/misc/NetworkOPs.h>
#include <divvy/basics/UptimeClock.h>
#include <divvy/core/DatabaseCon.h>
#include <divvy/json/json_value.h>
#include <divvy/ledger/CachedSLEs.h>
#include <divvy/net/RPCErr.h>
#include <divvy/nodestore/Database.h>
#include <divvy/nodestore/DatabaseShard.h>
#include <divvy/protocol/ErrorCodes.h>
#include <divvy/protocol/JsonFields.h>
#include <divvy/rpc/Context.h>

namespace divvy {

static
void
textTime(std::string& text, UptimeClock::time_point& seconds,
         const char* unitName, std::chrono::seconds unitVal)
{
    auto i = seconds.time_since_epoch() / unitVal;

    if (i == 0)
        return;

    seconds -= unitVal * i;

    if (!text.empty ())
        text += ", ";

    text += std::to_string(i);
    text += " ";
    text += unitName;

    if (i > 1)
        text += "s";
}

// {
//   min_count: <number>  // optional, defaults to 10
// }
Json::Value doGetCounts (RPC::Context& context)
{
    int minCount = 10;

    if (context.params.isMember (jss::min_count))
        minCount = context.params[jss::min_count].asUInt ();

    auto objectCounts = CountedObjects::getInstance ().getCounts (minCount);

    Json::Value ret (Json::objectValue);

    for (auto const& it : objectCounts)
    {
        ret [it.first] = it.second;
    }

    int dbKB = getKBUsedAll (context.app.getLedgerDB ().getSession ());

    if (dbKB > 0)
        ret[jss::dbKBTotal] = dbKB;

    dbKB = getKBUsedDB (context.app.getLedgerDB ().getSession ());

    if (dbKB > 0)
        ret[jss::dbKBLedger] = dbKB;

    dbKB = getKBUsedDB (context.app.getTxnDB ().getSession ());

    if (dbKB > 0)
        ret[jss::dbKBTransaction] = dbKB;

    {
        std::size_t c = context.app.getOPs().getLocalTxCount ();
        if (c > 0)
            ret[jss::local_txs] = static_cast<Json::UInt> (c);
    }

    ret[jss::write_load] = context.app.getNodeStore ().getWriteLoad ();

    ret[jss::historical_perminute] = static_cast<int>(
        context.app.getInboundLedgers().fetchRate());
    ret[jss::SLE_hit_rate] = context.app.cachedSLEs().rate();
    ret[jss::node_hit_rate] = context.app.getNodeStore ().getCacheHitRate ();
    ret[jss::ledger_hit_rate] = context.app.getLedgerMaster ().getCacheHitRate ();
    ret[jss::AL_hit_rate] = context.app.getAcceptedLedgerCache ().getHitRate ();

    ret[jss::fullbelow_size] = static_cast<int>(context.app.family().fullbelow().size());
    ret[jss::treenode_cache_size] = context.app.family().treecache().getCacheSize();
    ret[jss::treenode_track_size] = context.app.family().treecache().getTrackSize();

    std::string uptime;
    auto s = UptimeClock::now();
    using namespace std::chrono_literals;
    textTime (uptime, s, "year", 365 * 24h);
    textTime (uptime, s, "day", 24h);
    textTime (uptime, s, "hour", 1h);
    textTime (uptime, s, "minute", 1min);
    textTime (uptime, s, "second", 1s);
    ret[jss::uptime] = uptime;

    ret[jss::node_writes] = context.app.getNodeStore().getStoreCount();
    ret[jss::node_reads_total] = context.app.getNodeStore().getFetchTotalCount();
    ret[jss::node_reads_hit] = context.app.getNodeStore().getFetchHitCount();
    ret[jss::node_written_bytes] = context.app.getNodeStore().getStoreSize();
    ret[jss::node_read_bytes] = context.app.getNodeStore().getFetchSize();

    if (auto shardStore = context.app.getShardStore())
    {
        Json::Value& jv = (ret[jss::shards] = Json::objectValue);
        jv[jss::fullbelow_size] =
            static_cast<int>(context.app.shardFamily()->fullbelow().size());
        jv[jss::treenode_cache_size] =
            context.app.shardFamily()->treecache().getCacheSize();
        jv[jss::treenode_track_size] =
            context.app.shardFamily()->treecache().getTrackSize();
        ret[jss::write_load] = shardStore->getWriteLoad();
        ret[jss::node_hit_rate] = shardStore->getCacheHitRate();
        jv[jss::node_writes] = shardStore->getStoreCount();
        jv[jss::node_reads_total] = shardStore->getFetchTotalCount();
        jv[jss::node_reads_hit] = shardStore->getFetchHitCount();
        jv[jss::node_written_bytes] = shardStore->getStoreSize();
        jv[jss::node_read_bytes] = shardStore->getFetchSize();
    }

    return ret;
}

} // divvy
