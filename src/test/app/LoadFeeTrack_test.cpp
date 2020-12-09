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

#include <divvy/app/misc/LoadFeeTrack.h>
#include <divvy/core/Config.h>
#include <divvy/beast/unit_test.h>
#include <divvy/ledger/ReadView.h>

namespace divvy {

class LoadFeeTrack_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        Config d; // get a default configuration object
        LoadFeeTrack l;
        Fees const fees = [&]()
        {
            Fees f;
            f.base = d.FEE_DEFAULT;
            f.units = d.TRANSACTION_FEE_BASE;
            f.reserve = 200 * SYSTEM_CURRENCY_PARTS;
            f.increment = 50 * SYSTEM_CURRENCY_PARTS;
            return f;
        }();

        BEAST_EXPECT (scaleFeeLoad (10000, l, fees, false) == 10000);
        BEAST_EXPECT (scaleFeeLoad (1, l, fees, false) == 1);
    }
};

BEAST_DEFINE_TESTSUITE(LoadFeeTrack,divvy_core,divvy);

} // divvy
