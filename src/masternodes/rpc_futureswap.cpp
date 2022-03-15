#include <masternodes/govvariables/attributes.h>
#include <masternodes/mn_rpc.h>

UniValue depositfutureswap(const JSONRPCRequest& request) {
    auto pwallet = GetWallet(request);

    RPCHelpMan{"depositfutureswap",
                "Creates a vault transaction.\n" +
                HelpRequiringPassphrase(pwallet) + "\n",
                {
                    {"from", RPCArg::Type::STR, RPCArg::Optional::NO, "Any valid address"},
                    {"amounts", RPCArg::Type::STR, RPCArg::Optional::NO, "Amounts in amount@token format."},
                    {"tokenTo", RPCArg::Type::STR, RPCArg::Optional::NO, "One of the keys may be specified (id/symbol)"},
                    {"inputs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG,
                        "A json array of json objects",
                            {
                                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                     {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                     {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                                },
                            },
                        },
                    },
                },
                RPCResult{
                   "\"hash\"                  (string) The hex-encoded hash of broadcasted transaction\n"
                },
                RPCExamples{
                   HelpExampleCli("depositfutureswap", "2MzfSNCkjgCbNLen14CYrVtwGomfDA5AGYv") +
                   HelpExampleRpc("depositfutureswap", R"("2MzfSNCkjgCbNLen14CYrVtwGomfDA5AGYv", "LOAN0001")")
                },
    }.Check(request);

    if (pwallet->chain().isInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Cannot depositfutureswap while still in Initial Block Download");

    auto contracts = Params().GetConsensus().smartContracts;
    const auto& contractPair = contracts.find(SMART_CONTRACT_DFIP_XXXX);

    if (request.params[0].isNull()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Source address must be provided.");
    }

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    const auto script = GetScriptForDestination(dest);

    if (request.params[2].isNull()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "tokenTo must be provided.");
    }

    CDepositFutureSwapMessage msg{};
    msg.from = script;
    msg.amounts = DecodeAmounts(pwallet->chain(), request.params[1], "");

    auto tokenTo = pcustomcsview->GetTokenGuessId(request.params[2].getValStr(), msg.idTokenTo);
    if (!tokenTo)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "tokenTo was not found");
    // encode
    CDataStream metadata(DfTxMarker, SER_NETWORK, PROTOCOL_VERSION);
    metadata << static_cast<unsigned char>(CustomTxType::DepositFutureSwap)
                << msg;
    CScript scriptMeta;
    scriptMeta << OP_RETURN << ToByteVector(metadata);

    int targetHeight = chainHeight(*pwallet->chain().lock()) + 1;

    const auto txVersion = GetTransactionVersion(targetHeight);
    CMutableTransaction rawTx(txVersion);

    rawTx.vout.emplace_back(0, scriptMeta);

    CTransactionRef optAuthTx;
    std::set<CScript> auth{script};
    rawTx.vin = GetAuthInputsSmart(pwallet, rawTx.nVersion, auth, false, optAuthTx, request.params[3]);

    // Set change address
    CCoinControl coinControl;
    coinControl.destChange = dest;

    // fund
    fund(rawTx, pwallet, optAuthTx, &coinControl);

    // check execution
    execTestTx(CTransaction(rawTx), targetHeight, optAuthTx);

    return signsend(rawTx, pwallet, optAuthTx)->GetHash().GetHex();
}

UniValue listfutureswap(const JSONRPCRequest& request) {
    auto pwallet = GetWallet(request);

    RPCHelpMan{"listfutureswap",
                "Creates a vault transaction.\n" +
                HelpRequiringPassphrase(pwallet) + "\n",
                {
                    {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "Any valid address"},
                },
                RPCResult{
                   "\"hash\"                  (string) The hex-encoded hash of broadcasted transaction\n"
                },
                RPCExamples{
                   HelpExampleCli("depositfutureswap", "2MzfSNCkjgCbNLen14CYrVtwGomfDA5AGYv")
                },
    }.Check(request);

    if (request.params[0].isNull()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Owner address must be provided to listfutureswap");
    }

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    const auto script = GetScriptForDestination(dest);
    UniValue balances{UniValue::VARR};
    pcustomcsview->ForEachFutureSwapBalance([&](FutureSwapKey const & key, CAmount amount) {
        if (key.owner != script)
            return false;

        balances.push_back(CTokenAmount{key.tokenID, amount}.ToString());
        return true;
    }, FutureSwapKey{script, {}});

    return balances;
}

static const CRPCCommand commands[] =
{
//  category        name                         actor (function)        params
//  --------------- ----------------------       ---------------------   ----------
    {"futureswap",  "depositfutureswap",         &depositfutureswap,     {"from", "amounts", "idTokenTo"}},
    // {"futureswap",  "withdrawfutureswap",     &withdrawfutureswap,    {"from", "amounts", "idTokenTo"}},
    {"futureswap",  "listfutureswap",            &listfutureswap,        {"ownerAddress"}},
};

void RegisterFutureSwapRPCCommands(CRPCTable& tableRPC) {
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
