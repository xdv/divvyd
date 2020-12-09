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

#include <divvy/ledger/CachedView.h>
#include <divvy/basics/contract.h>
#include <divvy/protocol/Serializer.h>

namespace divvy {
namespace detail {

bool
CachedViewImpl::exists (Keylet const& k) const
{
    return read(k) != nullptr;
}

std::shared_ptr<SLE const>
CachedViewImpl::read (Keylet const& k) const
{
    {
        std::lock_guard<
            std::mutex> lock(mutex_);
        auto const iter = map_.find(k.key);
        if (iter != map_.end())
        {
            if (! k.check(*iter->second))
                return nullptr;
            return iter->second;
        }
    }
    auto const digest =
        base_.digest(k.key);
    if (! digest)
        return nullptr;
    auto sle = cache_.fetch(*digest,
        [&]() { return base_.read(k); });
    std::lock_guard<
        std::mutex> lock(mutex_);
    auto const iter =
        map_.find(k.key);
    if (iter == map_.end())
    {
        map_.emplace(k.key, sle);
        return sle;
    }
    if (! k.check(*iter->second))
        LogicError("CachedView::read: wrong type");
    return iter->second;

}

} // detail
} // divvy
