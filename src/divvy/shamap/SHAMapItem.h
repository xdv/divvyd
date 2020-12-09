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

#ifndef DIVVY_SHAMAP_SHAMAPITEM_H_INCLUDED
#define DIVVY_SHAMAP_SHAMAPITEM_H_INCLUDED

#include <divvy/basics/base_uint.h>
#include <divvy/basics/Blob.h>
#include <divvy/basics/Slice.h>
#include <divvy/protocol/Serializer.h>
#include <divvy/beast/utility/Journal.h>

#include <cstddef>

namespace divvy {

// an item stored in a SHAMap
class SHAMapItem
{
private:
    uint256    tag_;
    Blob       data_;

public:
    SHAMapItem (uint256 const& tag, Blob const & data);
    SHAMapItem (uint256 const& tag, Serializer const& s);
    SHAMapItem (uint256 const& tag, Serializer&& s);

    Slice slice() const;

    uint256 const& key() const;

    Blob const& peekData() const;

    std::size_t size() const;
    void const* data() const;
};

//------------------------------------------------------------------------------

inline
Slice
SHAMapItem::slice() const
{
    return {data_.data(), data_.size()};
}

inline
std::size_t
SHAMapItem::size() const
{
    return data_.size();
}

inline
void const*
SHAMapItem::data() const
{
    return data_.data();
}

inline
uint256 const&
SHAMapItem::key() const
{
    return tag_;
}

inline
Blob const&
SHAMapItem::peekData() const
{
    return data_;
}

} // divvy

#endif
