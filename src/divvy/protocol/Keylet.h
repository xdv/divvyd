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

#ifndef DIVVY_PROTOCOL_KEYLET_H_INCLUDED
#define DIVVY_PROTOCOL_KEYLET_H_INCLUDED

#include <divvy/protocol/LedgerFormats.h>
#include <divvy/basics/base_uint.h>

namespace divvy {

class STLedgerEntry;

/** A pair of SHAMap key and LedgerEntryType.

    A Keylet identifies both a key in the state map
    and its ledger entry type.

    @note Keylet is a portmanteau of the words key
          and LET, an acronym for LedgerEntryType.
*/
struct Keylet
{
    LedgerEntryType type;
    uint256 key;

    Keylet (LedgerEntryType type_,
            uint256 const& key_)
        : type(type_)
        , key(key_)
    {
    }

    /** Returns true if the SLE matches the type */
    bool
    check (STLedgerEntry const&) const;
};

}

#endif
