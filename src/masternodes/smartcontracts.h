// Copyright (c) 2022 The DeFi Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFI_MASTERNODES_SMARTCONTRACTS_H
#define DEFI_MASTERNODES_SMARTCONTRACTS_H

#include <flushablestorage.h>
#include <masternodes/res.h>
#include <masternodes/balances.h>
#include <amount.h>
#include <script/script.h>

struct CSmartContractMessage;

class CSmartContractView : public virtual CStorageView
{
    public:
        void ForEachSmartContractBalance(std::function<bool(std::pair<uint8_t, BalanceKey> const &, CAmount)> callback, std::pair<uint8_t, BalanceKey> const & start = {});

        CTokenAmount GetSmartContractBalance(uint8_t smartContract, CScript const & owner, DCT_ID tokenID) const;

        Res AddSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount);
        Res SubSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount);

        struct BySmartContractBalanceKey { static constexpr uint8_t prefix() { return 0x26; } };

    private:
        Res SetSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount);

};

#endif // DEFI_MASTERNODES_SMARTCONTRACTS_H