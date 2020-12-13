# The XDV Ledger

The XDV Ledger is a decentralized cryptographic ledger powered by a network of peer-to-peer servers. The XDV Ledger uses a novel Byzantine Fault Tolerant consensus algorithm to settle and record transactions in a secure distributed database without a central operator.

## XDV
XDV is a public, counterparty-less asset native to the XDV Ledger, and is designed to bridge the many different currencies in use worldwide. XDV is traded on the open-market and is available for anyone to access. The XDV Ledger was created in 2012 with a finite supply of 100 billion units of XDV. Its creators gifted 80 billion XDV to a company, now called [Divvy](https://xdv.io/), to develop the XDV Ledger and its ecosystem.  Divvy uses XDV the help build the Internet of Value, ushering in a world in which money moves as fast and efficiently as information does today.

## `divvyd`
The server software that powers the XDV Ledger is called `divvyd` and is available in this repository under the permissive [ISC open-source license](LICENSE). The `divvyd` server is written primarily in C++ and runs on a variety of platforms.


# Key Features of the XDV Ledger

- **[Censorship-Resistant Transaction Processing][]:** No single party decides which transactions succeed or fail, and no one can "roll back" a transaction after it completes. As long as those who choose to participate in the network keep it healthy, they can settle transactions in seconds.
- **[Fast, Efficient Consensus Algorithm][]:** The XDV Ledger's consensus algorithm settles transactions in 4 to 5 seconds, processing at a throughput of up to 1500 transactions per second. These properties put XDV at least an order of magnitude ahead of other top digital assets.
- **[Finite XDV Supply][]:** When the XDV Ledger began, 100 billion XDV were created, and no more XDV will ever be created. (Each XDV is subdivisible down to 6 decimal places, for a grand total of 100 quintillion _drops_ of XDV.) The available supply of XDV decreases slowly over time as small amounts are destroyed to pay transaction costs.
- **[Responsible Software Governance][]:** A team of full-time, world-class developers at Divvy maintain and continually improve the XDV Ledger's underlying software with contributions from the open-source community. Divvy acts as a steward for the technology and an advocate for its interests, and builds constructive relationships with governments and financial institutions worldwide.
- **[Secure, Adaptable Cryptography][]:** The XDV Ledger relies on industry standard digital signature systems like ECDSA (the same scheme used by Bitcoin) but also supports modern, efficient algorithms like Ed25519. The extensible nature of the XDV Ledger's software makes it possible to add and disable algorithms as the state of the art in cryptography advances.
- **[Modern Features for Smart Contracts][]:** Features like Escrow, Checks, and Payment Channels support cutting-edge financial applications including the [Interledger Protocol](https://interledger.org/). This toolbox of advanced features comes with safety features like a process for amending the network and separate checks against invariant constraints.
- **[On-Ledger Decentralized Exchange][]:** In addition to all the features that make XDV useful on its own, the XDV Ledger also has a fully-functional accounting system for tracking and trading obligations denominated in any way users want, and an exchange built into the protocol. The XDV Ledger can settle long, cross-currency payment paths and exchanges of multiple currencies in atomic transactions, bridging gaps of trust with XDV.

[Censorship-Resistant Transaction Processing]: https://developers.xdv.io/xdv-ledger-overview.html#censorship-resistant-transaction-processing
[Fast, Efficient Consensus Algorithm]: https://developers.xdv.io/xdv-ledger-overview.html#fast-efficient-consensus-algorithm
[Finite XDV Supply]: https://developers.xdv.io/xdv-ledger-overview.html#finite-xdv-supply
[Responsible Software Governance]: https://developers.xdv.io/xdv-ledger-overview.html#responsible-software-governance
[Secure, Adaptable Cryptography]: https://developers.xdv.io/xdv-ledger-overview.html#secure-adaptable-cryptography
[Modern Features for Smart Contracts]: https://developers.xdv.io/xdv-ledger-overview.html#modern-features-for-smart-contracts
[On-Ledger Decentralized Exchange]: https://developers.xdv.io/xdv-ledger-overview.html#on-ledger-decentralized-exchange


## Source Code
[![travis-ci.org: Build Status](https://travis-ci.org/xdv/divvyd.png?branch=develop)](https://travis-ci.org/xdv/divvyd)
[![codecov.io: Code Coverage](https://codecov.io/gh/xdv/divvyd/branch/develop/graph/badge.svg)](https://codecov.io/gh/xdv/divvyd)

### Repository Contents

| Folder     | Contents                                         |
|:-----------|:-------------------------------------------------|
| `./bin`    | Scripts and data files for Divvy integrators.   |
| `./Builds` | Platform-specific guides for building `divvyd`. |
| `./docs`   | Source documentation files and doxygen config.   |
| `./cfg`    | Example configuration files.                     |
| `./src`    | Source code.                                     |

Some of the directories under `src` are external repositories included using
git-subtree. See those directories' README files for more details.


## See Also

* [XDV Ledger Dev Portal](https://developers.xdv.io/)
* [XDV News](https://xdv.io/category/xdv/)
* [Setup and Installation](https://developers.xdv.io/install-divvyd.html)

To learn about how Divvy is transforming global payments, visit
<https://xdv.io/contact/>.

## Divvy Genesis Ledger

```
{
   "id" : 1,
   "result" : {
      "ledger" : {
         "accepted" : true,
         "account_hash" : "A21ED30C04C88046FC61DB9DC19375EEDBD365FD8C17286F27127DF804E9CAA6",
         "close_flags" : 0,
         "close_time" : 487287840,
         "close_time_human" : "2015-Jun-10 21:44:00",
         "close_time_resolution" : 30,
         "closed" : true,
         "hash" : "8C416580CE16A06D291666FD423F12AA2C6BD0E9440ED6BEF59889906CD9A7E9",
         "ledger_hash" : "8C416580CE16A06D291666FD423F12AA2C6BD0E9440ED6BEF59889906CD9A7E9",
         "ledger_index" : "1",
         "parent_close_time" : 0,
         "parent_hash" : "0000000000000000000000000000000000000000000000000000000000000000",
         "seqNum" : "1",
         "totalCoins" : "100000000000000000",
         "total_coins" : "100000000000000000",
         "transaction_hash" : "0000000000000000000000000000000000000000000000000000000000000000"
      },
      "ledger_hash" : "8C416580CE16A06D291666FD423F12AA2C6BD0E9440ED6BEF59889906CD9A7E9",
      "ledger_index" : 1,
      "status" : "success",
      "validated" : true
   }
}
```
