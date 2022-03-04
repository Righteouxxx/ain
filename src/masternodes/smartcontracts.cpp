// Copyright (c) 2022 The DeFi Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <masternodes/smartcontracts.h>

void CSmartContractView::ForEachSmartContractBalance(std::function<bool(std::pair<uint8_t, BalanceKey> const &, CAmount)> callback, std::pair<uint8_t, BalanceKey> const & start)
{
    ForEach<BySmartContractBalanceKey, std::pair<uint8_t, BalanceKey>, CAmount>(callback, start);
}

Res CSmartContractView::SetSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount)
{
    if (amount.nValue != 0) {
        WriteBy<BySmartContractBalanceKey>(std::make_pair(smartContract, BalanceKey{owner, amount.nTokenId}), amount.nValue);
    } else {
        EraseBy<BySmartContractBalanceKey>(std::make_pair(smartContract, BalanceKey{owner, amount.nTokenId}));
    }
    return Res::Ok();
}

CTokenAmount CSmartContractView::GetSmartContractBalance(uint8_t smartContract, CScript const & owner, DCT_ID tokenID) const
{
    CAmount val;
    bool ok = ReadBy<BySmartContractBalanceKey>(std::make_pair(smartContract, BalanceKey{owner, tokenID}), val);
    if (ok) {
        return CTokenAmount{tokenID, val};
    }
    return CTokenAmount{tokenID, 0};
}

Res CSmartContractView::AddSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount)
{
    if (amount.nValue == 0) {
        return Res::Ok();
    }
    auto balance = GetSmartContractBalance(smartContract, owner, amount.nTokenId);
    auto res = balance.Add(amount.nValue);
    if (!res.ok) {
        return res;
    }
    return SetSmartContractBalance(smartContract, owner, balance);
}

Res CSmartContractView::SubSmartContractBalance(uint8_t smartContract, CScript const & owner, CTokenAmount amount)
{
    if (amount.nValue == 0) {
        return Res::Ok();
    }
    auto balance = GetSmartContractBalance(smartContract, owner, amount.nTokenId);
    auto res = balance.Sub(amount.nValue);
    if (!res.ok) {
        return res;
    }
    return SetSmartContractBalance(smartContract, owner, balance);
}
