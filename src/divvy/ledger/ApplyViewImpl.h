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

#ifndef DIVVY_LEDGER_APPLYVIEWIMPL_H_INCLUDED
#define DIVVY_LEDGER_APPLYVIEWIMPL_H_INCLUDED

#include <divvy/ledger/OpenView.h>
#include <divvy/ledger/detail/ApplyViewBase.h>
#include <divvy/protocol/STAmount.h>
#include <divvy/protocol/TER.h>
#include <boost/optional.hpp>

namespace divvy {

/** Editable, discardable view that can build metadata for one tx.

    Iteration of the tx map is delegated to the base.

    @note Presented as ApplyView to clients.
*/
class ApplyViewImpl final
    : public detail::ApplyViewBase
{
public:
    ApplyViewImpl() = delete;
    ApplyViewImpl (ApplyViewImpl const&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl&&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl const&) = delete;

    ApplyViewImpl (ApplyViewImpl&&) = default;
    ApplyViewImpl(
        ReadView const* base, ApplyFlags flags);

    /** Apply the transaction.

        After a call to `apply`, the only valid
        operation on this object is to call the
        destructor.
    */
    void
    apply (OpenView& to,
        STTx const& tx, TER ter,
            beast::Journal j);

    /** Set the amount of currency delivered.

        This value is used when generating metadata
        for payments, to set the DeliveredAmount field.
        If the amount is not specified, the field is
        excluded from the resulting metadata.
    */
    void
    deliver (STAmount const& amount)
    {
        deliver_ = amount;
    }

    /** Get the number of modified entries
    */
    std::size_t
    size ();

    /** Visit modified entries
    */
    void
    visit (
        OpenView& target,
        std::function <void (
            uint256 const& key,
            bool isDelete,
            std::shared_ptr <SLE const> const& before,
            std::shared_ptr <SLE const> const& after)> const& func);
private:
    boost::optional<STAmount> deliver_;
};

} // divvy

#endif
