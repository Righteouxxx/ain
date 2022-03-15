// Copyright (c) 2022 The DeFi Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <masternodes/futures.h>

void CFutureSwapView::ForEachFutureSwapBalance(std::function<bool(FutureSwapKey const &, CAmount)> callback, FutureSwapKey const & start)
{
    ForEach<ByFutureSwapBalanceKey, FutureSwapKey, CAmount>(callback, start);
}

Res CFutureSwapView::SetFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo)
{
    if (amount.nValue != 0) {
        WriteBy<ByFutureSwapBalanceKey>(FutureSwapKey{owner, amount.nTokenId, idTokenTo}, amount.nValue);
    } else {
        EraseBy<ByFutureSwapBalanceKey>(FutureSwapKey{owner, amount.nTokenId, idTokenTo});
    }
    return Res::Ok();
}

CTokenAmount CFutureSwapView::GetFutureSwapBalance(CScript const & owner, DCT_ID tokenID, DCT_ID idTokenTo) const
{
    CAmount val;
    bool ok = ReadBy<ByFutureSwapBalanceKey>(FutureSwapKey{owner, tokenID, idTokenTo}, val);
    if (ok) {
        return CTokenAmount{tokenID, val};
    }
    return CTokenAmount{tokenID, 0};
}

Res CFutureSwapView::AddFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo)
{
    if (amount.nValue == 0) {
        return Res::Ok();
    }
    auto balance = GetFutureSwapBalance(owner, amount.nTokenId, idTokenTo);
    auto res = balance.Add(amount.nValue);
    if (!res.ok) {
        return res;
    }
    return SetFutureSwapBalance(owner, balance, idTokenTo);
}

Res CFutureSwapView::SubFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo)
{
    if (amount.nValue == 0) {
        return Res::Ok();
    }
    auto balance = GetFutureSwapBalance(owner, amount.nTokenId, idTokenTo);
    auto res = balance.Sub(amount.nValue);
    if (!res.ok) {
        return res;
    }
    return SetFutureSwapBalance(owner, balance, idTokenTo);
}
