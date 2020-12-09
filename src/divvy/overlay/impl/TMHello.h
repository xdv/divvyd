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

#ifndef DIVVY_OVERLAY_TMHELLO_H_INCLUDED
#define DIVVY_OVERLAY_TMHELLO_H_INCLUDED

#include <divvy/protocol/messages.h>
#include <divvy/app/main/Application.h>
#include <divvy/beast/utility/Journal.h>
#include <divvy/protocol/BuildInfo.h>

#include <boost/beast/http/message.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>
#include <utility>

namespace divvy {

enum
{
    // The clock drift we allow a remote peer to have
    clockToleranceDeltaSeconds = 20
};

/** Computes a shared value based on the SSL connection state.
    When there is no man in the middle, both sides will compute the same
    value. In the presence of an attacker, the computed values will be
    different.
    If the shared value generation fails, the link MUST be dropped.
    @return A pair. Second will be false if shared value generation failed.
*/
boost::optional<uint256>
makeSharedValue (SSL* ssl, beast::Journal journal);

/** Build a TMHello protocol message. */
protocol::TMHello
buildHello (uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote, Application& app);

/** Insert HTTP headers based on the TMHello protocol message. */
void
appendHello (boost::beast::http::fields& h, protocol::TMHello const& hello);

/** Parse HTTP headers into TMHello protocol message.
    @return A protocol message on success; an empty optional
            if the parsing failed.
*/
boost::optional<protocol::TMHello>
parseHello (bool request, boost::beast::http::fields const& h, beast::Journal journal);

/** Validate and store the public key in the TMHello.
    This includes signature verification on the shared value.
    @return The remote end public key on success; an empty
            optional if the check failed.
*/
boost::optional<PublicKey>
verifyHello (protocol::TMHello const& h, uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote,
    beast::Journal journal, Application& app);

/** Parse a set of protocol versions.
    The returned list contains no duplicates and is sorted ascending.
    Any strings that are not parseable as RTXP protocol strings are
    excluded from the result set.
*/
std::vector<ProtocolVersion>
parse_ProtocolVersions(boost::beast::string_view const& s);

}

#endif
