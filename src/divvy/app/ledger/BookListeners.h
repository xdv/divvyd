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

#ifndef DIVVY_APP_LEDGER_BOOKLISTENERS_H_INCLUDED
#define DIVVY_APP_LEDGER_BOOKLISTENERS_H_INCLUDED

#include <divvy/net/InfoSub.h>
#include <memory>
#include <mutex>

namespace divvy {

/** Listen to public/subscribe messages from a book. */
class BookListeners
{
public:
    using pointer = std::shared_ptr<BookListeners>;

    BookListeners()
    {
    }

    /** Add a new subscription for this book
    */
    void
    addSubscriber(InfoSub::ref sub);

    /** Stop publishing to a subscriber
    */
    void
    removeSubscriber(std::uint64_t sub);

    /** Publish a transaction to subscribers

        Publish a transaction to clients subscribed to changes on this book.
        Uses havePublished to prevent sending duplicate transactions to clients
        that have subscribed to multiple books.

        @param jvObj JSON transaction data to publish
        @param havePublished InfoSub sequence numbers that have already
                             published this transaction.

    */
    void
    publish(Json::Value const& jvObj, hash_set<std::uint64_t>& havePublished);

private:
    std::recursive_mutex mLock;

    hash_map<std::uint64_t, InfoSub::wptr> mListeners;
};

}  // namespace divvy

#endif
