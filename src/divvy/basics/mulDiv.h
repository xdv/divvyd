//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012-2015 Ripple Labs Inc.

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

#ifndef DIVVY_BASICS_MULDIV_H_INCLUDED
#define DIVVY_BASICS_MULDIV_H_INCLUDED

#include <cstdint>
#include <utility>

namespace divvy
{

/** Return value*mul/div accurately.
    Computes the result of the multiplication and division in
    a single step, avoiding overflow and retaining precision.
    Throws:
        None
    Returns:
        `std::pair`:
            `first` is `false` if the calculation overflows,
                `true` if the calculation is safe.
            `second` is the result of the calculation if
                `first` is `false`, max value of `uint64_t`
                if `true`.
*/
std::pair<bool, std::uint64_t>
mulDiv(std::uint64_t value, std::uint64_t mul, std::uint64_t div);

} // divvy

#endif
