// Copyright (c) 2022 The DeFi Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFI_MASTERNODES_FUTURES_H
#define DEFI_MASTERNODES_FUTURES_H

#include <flushablestorage.h>
#include <masternodes/res.h>
#include <masternodes/balances.h>
#include <amount.h>
#include <script/script.h>

struct CDepositFutureSwapMessage {
    DCT_ID idTokenTo;
    CScript from;
    CBalances amounts;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(idTokenTo);
        READWRITE(from);
        READWRITE(amounts);
    }
};

struct CWithdrawFutureSwapMessage {
    DCT_ID idTokenTo;
    CScript from;
    CBalances amounts;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(idTokenTo);
        READWRITE(from);
        READWRITE(amounts);
    }
};

struct FutureSwapKey {
    CScript owner;
    DCT_ID tokenID;
    DCT_ID idTokenTo;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(owner);
        READWRITE(WrapBigEndian(tokenID.v));
        READWRITE(WrapBigEndian(idTokenTo.v));
    }
};

class CFutureSwapView : public virtual CStorageView
{
    public:
        void ForEachFutureSwapBalance(std::function<bool(FutureSwapKey const &, CAmount)> callback, FutureSwapKey const & start = {});

        Res AddFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo);
        Res SubFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo);

        struct ByFutureSwapBalanceKey { static constexpr uint8_t prefix() { return 0x26; } };

    private:
        CTokenAmount GetFutureSwapBalance(CScript const & owner, DCT_ID tokenID, DCT_ID idTokenTo) const;
        Res SetFutureSwapBalance(CScript const & owner, CTokenAmount amount, DCT_ID idTokenTo);

};

#endif // DEFI_MASTERNODES_FUTURES_H