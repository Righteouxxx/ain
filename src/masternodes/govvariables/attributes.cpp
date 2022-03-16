// Copyright (c) 2020 The DeFi Foundation
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <masternodes/govvariables/attributes.h>

#include <core_io.h> /// ValueFromAmount
#include <masternodes/masternodes.h> /// CCustomCSView
#include <masternodes/mn_checks.h> /// GetAggregatePrice
#include <util/strencodings.h>

extern UniValue AmountsToJSON(TAmounts const & diffs);

static inline std::string trim_all_ws(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

static std::vector<std::string> KeyBreaker(const std::string& str, const char delim = '/'){
    std::string section;
    std::istringstream stream(str);
    std::vector<std::string> strVec;

    while (std::getline(stream, section, delim)) {
        strVec.push_back(section);
    }
    return strVec;
}

const std::map<std::string, uint8_t>& ATTRIBUTES::allowedVersions() {
    static const std::map<std::string, uint8_t> versions{
            {"v0",  VersionTypes::v0},
    };
    return versions;
}

const std::map<uint8_t, std::string>& ATTRIBUTES::displayVersions() {
    static const std::map<uint8_t, std::string> versions{
            {VersionTypes::v0,  "v0"},
    };
    return versions;
}

const std::map<std::string, uint8_t>& ATTRIBUTES::allowedTypes() {
    static const std::map<std::string, uint8_t> types{
        {"oracles",     AttributeTypes::Oracles},
        {"params",      AttributeTypes::Param},
        {"poolpairs",   AttributeTypes::Poolpairs},
        {"token",       AttributeTypes::Token},
    };
    return types;
}

const std::map<uint8_t, std::string>& ATTRIBUTES::displayTypes() {
    static const std::map<uint8_t, std::string> types{
        {AttributeTypes::Live,      "live"},
        {AttributeTypes::Oracles,   "oracles"},
        {AttributeTypes::Param,     "params"},
        {AttributeTypes::Poolpairs, "poolpairs"},
        {AttributeTypes::Token,     "token"},
    };
    return types;
}

const std::map<std::string, uint8_t>& ATTRIBUTES::allowedParamIDs() {
    static const std::map<std::string, uint8_t> params{
            {"dfip2201",    ParamIDs::DFIP2201}
    };
    return params;
}

const std::map<uint8_t, std::string>& ATTRIBUTES::displayParamsIDs() {
    static const std::map<uint8_t, std::string> params{
            {ParamIDs::DFIP2201,    "dfip2201"},
            {ParamIDs::Economy,     "economy"},
    };
    return params;
}

const std::map<std::string, uint8_t>& ATTRIBUTES::allowedOracleIDs() {
    static const std::map<std::string, uint8_t> params{
            {"splits",    OracleIDs::Splits}
    };
    return params;
}

const std::map<uint8_t, std::string>& ATTRIBUTES::displayOracleIDs() {
    static const std::map<uint8_t, std::string> params{
            {OracleIDs::Splits,    "splits"},
    };
    return params;
}

const std::map<uint8_t, std::map<std::string, uint8_t>>& ATTRIBUTES::allowedKeys() {
    static const std::map<uint8_t, std::map<std::string, uint8_t>> keys{
        {
            AttributeTypes::Token, {
                {"payback_dfi",             TokenKeys::PaybackDFI},
                {"payback_dfi_fee_pct",     TokenKeys::PaybackDFIFeePCT},
                {"dex_in_fee_pct",          TokenKeys::DexInFeePct},
                {"dex_out_fee_pct",         TokenKeys::DexOutFeePct},
                {"fixed_interval_price_id", TokenKeys::FixedIntervalPriceId},
                {"loan_collateral_enabled", TokenKeys::LoanCollateralEnabled},
                {"loan_collateral_factor",  TokenKeys::LoanCollateralFactor},
                {"loan_minting_enabled",    TokenKeys::LoanMintingEnabled},
                {"loan_minting_interest",   TokenKeys::LoanMintingInterest},
            }
        },
        {
            AttributeTypes::Poolpairs, {
                {"token_a_fee_pct",     PoolKeys::TokenAFeePCT},
                {"token_b_fee_pct",     PoolKeys::TokenBFeePCT},
            }
        },
        {
            AttributeTypes::Param, {
                {"active",              DFIP2201Keys::Active},
                {"minswap",             DFIP2201Keys::MinSwap},
                {"premium",             DFIP2201Keys::Premium},
            }
        },
    };
    return keys;
}

const std::map<TokenKeys, CAttributeValue> ATTRIBUTES::tokenKeysToType {
    {TokenKeys::PaybackDFI,            bool{}},
    {TokenKeys::PaybackDFIFeePCT,      CAmount{}},
    {TokenKeys::DexInFeePct,           CAmount{}},
    {TokenKeys::DexOutFeePct,          CAmount{}},
    {TokenKeys::FixedIntervalPriceId,  CTokenCurrencyPair{}},
    {TokenKeys::LoanCollateralEnabled, bool{}},
    {TokenKeys::LoanCollateralFactor,  CAmount{}},
    {TokenKeys::LoanMintingEnabled,    bool{}},
    {TokenKeys::LoanMintingInterest,   CAmount{}},
};

const std::map<PoolKeys, CAttributeValue> ATTRIBUTES::poolKeysToType {
    {PoolKeys::TokenAFeePCT,      CAmount{}},
    {PoolKeys::TokenBFeePCT,      CAmount{}},
};

const std::map<uint8_t, std::map<uint8_t, std::string>>& ATTRIBUTES::displayKeys() {
    static const std::map<uint8_t, std::map<uint8_t, std::string>> keys{
        {
            AttributeTypes::Token, {
                {TokenKeys::PaybackDFI,       "payback_dfi"},
                {TokenKeys::PaybackDFIFeePCT, "payback_dfi_fee_pct"},
                {TokenKeys::DexInFeePct,      "dex_in_fee_pct"},
                {TokenKeys::DexOutFeePct,     "dex_out_fee_pct"},
                {TokenKeys::FixedIntervalPriceId,  "fixed_interval_price_id"},
                {TokenKeys::LoanCollateralEnabled, "loan_collateral_enabled"},
                {TokenKeys::LoanCollateralFactor,  "loan_collateral_factor"},
                {TokenKeys::LoanMintingEnabled,    "loan_minting_enabled"},
                {TokenKeys::LoanMintingInterest,   "loan_minting_interest"},
                {TokenKeys::Ascendant,        "ascendant"},
                {TokenKeys::Descendant,       "descendant"},
                {TokenKeys::Epitaph,          "epitaph"},
            }
        },
        {
            AttributeTypes::Poolpairs, {
                {PoolKeys::TokenAFeePCT,      "token_a_fee_pct"},
                {PoolKeys::TokenBFeePCT,      "token_b_fee_pct"},
            }
        },
        {
            AttributeTypes::Param, {
                {DFIP2201Keys::Active,        "active"},
                {DFIP2201Keys::Premium,       "premium"},
                {DFIP2201Keys::MinSwap,       "minswap"},
            }
        },
        {
            AttributeTypes::Live, {
                {EconomyKeys::PaybackDFITokens,  "dfi_payback_tokens"},
            }
        },
    };
    return keys;
}

static ResVal<int32_t> VerifyInt32(const std::string& str) {
    int32_t int32;
    if (!ParseInt32(str, &int32)) {
        return Res::Err("Value must be an integer");
    }
    return {int32, Res::Ok()};
}

static ResVal<int32_t> VerifyPositiveInt32(const std::string& str) {
    int32_t int32;
    if (!ParseInt32(str, &int32) || int32 < 0) {
        return Res::Err("Identifier must be a positive integer");
    }
    return {int32, Res::Ok()};
}

static ResVal<CAttributeValue> VerifyFloat(const std::string& str) {
    CAmount amount = 0;
    if (!ParseFixedPoint(str, 8, &amount) || amount < 0) {
        return Res::Err("Amount must be a positive value");
    }
    return {amount, Res::Ok()};
}

static ResVal<CAttributeValue> VerifyPct(const std::string& str) {
    auto resVal = VerifyFloat(str);
    if (!resVal) {
        return resVal;
    }
    if (CAttributeValue{COIN} < *resVal.val) {
        return Res::Err("Percentage exceeds 100%%");
    }
    return resVal;
}

static ResVal<CAttributeValue> VerifyCurrencyPair(const std::string& str) {
    const auto value = KeyBreaker(str);
    if (value.size() != 2) {
        return Res::Err("Exactly two entires expected for currency pair");
    }
    auto token = trim_all_ws(value[0]).substr(0, CToken::MAX_TOKEN_SYMBOL_LENGTH);
    auto currency = trim_all_ws(value[1]).substr(0, CToken::MAX_TOKEN_SYMBOL_LENGTH);
    if (token.empty() || currency.empty()) {
        return Res::Err("Empty token / currency");
    }
    return {CTokenCurrencyPair{token, currency}, Res::Ok()};
}

static ResVal<CAttributeValue> VerifyBool(const std::string& str) {
    if (str != "true" && str != "false") {
        return Res::Err(R"(Boolean value must be either "true" or "false")");
    }
    return {str == "true", Res::Ok()};
}

static bool VerifyToken(const CCustomCSView& view, const uint32_t id) {
    return view.GetToken(DCT_ID{id}).has_value();
}

static ResVal<CAttributeValue> VerifySplit(const std::string& str) {
    const auto values = KeyBreaker(str, ',');
    if (values.empty()) {
        return Res::Err(R"(No valid values supplied, "id/multiplier, ...")");
    }

    OracleSplits splits;
    for (const auto& item : values) {
        const auto pairs = KeyBreaker(item);
        if (pairs.size() != 2) {
            return Res::Err("Two int values expected for split in id/mutliplier");
        }
        const auto resId = VerifyPositiveInt32(pairs[0]);
        if (!resId) {
            return resId;
        }
        const auto resMultiplier = VerifyInt32(pairs[1]);
        if (!resMultiplier) {
            return resMultiplier;
        }
        if (*resMultiplier == 0) {
            return Res::Err("Mutliplier cannot be zero");
        }
        splits[*resId] = *resMultiplier;
    }

    return {splits, Res::Ok()};
}

const std::map<uint8_t, std::map<uint8_t,
        std::function<ResVal<CAttributeValue>(const std::string&)>>>& ATTRIBUTES::parseValue() {

    static const std::map<uint8_t, std::map<uint8_t,
            std::function<ResVal<CAttributeValue>(const std::string&)>>> parsers{
            {
                AttributeTypes::Token, {
                    {TokenKeys::PaybackDFI,            VerifyBool},
                    {TokenKeys::PaybackDFIFeePCT,      VerifyPct},
                    {TokenKeys::DexInFeePct,           VerifyPct},
                    {TokenKeys::DexOutFeePct,          VerifyPct},
                    {TokenKeys::FixedIntervalPriceId,  VerifyCurrencyPair},
                    {TokenKeys::LoanCollateralEnabled, VerifyBool},
                    {TokenKeys::LoanCollateralFactor,  VerifyPct},
                    {TokenKeys::LoanMintingEnabled,    VerifyBool},
                    {TokenKeys::LoanMintingInterest,   VerifyFloat},
                }
            },
            {
                AttributeTypes::Poolpairs, {
                    {PoolKeys::TokenAFeePCT,      VerifyPct},
                    {PoolKeys::TokenBFeePCT,      VerifyPct},
                }
            },
            {
                AttributeTypes::Param, {
                    {DFIP2201Keys::Active,       VerifyBool},
                    {DFIP2201Keys::Premium,      VerifyPct},
                    {DFIP2201Keys::MinSwap,      VerifyFloat},
                }
            },
            {
                AttributeTypes::Oracles, {
                    {OracleIDs::Splits,          VerifySplit},
                }
            },
    };
    return parsers;
}

static Res ShowError(const std::string& key, const std::map<std::string, uint8_t>& keys) {
    std::string error{"Unrecognised " + key + " argument provided, valid " + key + "s are:"};
    for (const auto& pair : keys) {
        error += ' ' + pair.first + ',';
    }
    return Res::Err(error);
}

Res ATTRIBUTES::ProcessVariable(const std::string& key, const std::string& value,
                                const std::function<Res(const CAttributeType&, const CAttributeValue&)>& applyVariable) const {

    if (key.size() > 128) {
        return Res::Err("Identifier exceeds maximum length (128)");
    }

    const auto keys = KeyBreaker(key);
    if (keys.empty() || keys[0].empty()) {
        return Res::Err("Empty version");
    }

    if (value.empty()) {
        return Res::Err("Empty value");
    }

    const auto& iver = allowedVersions().find(keys[0]);
    if (iver == allowedVersions().end()) {
        return Res::Err("Unsupported version");
    }

    const auto& version = iver->second;
    if (version != VersionTypes::v0) {
        return Res::Err("Unsupported version");
    }

    if (keys.size() != 4 || keys[1].empty() || keys[2].empty() || keys[3].empty()) {
        return Res::Err("Incorrect key for <type>. Object of ['<version>/<type>/ID/<key>','value'] expected");
    }

    auto itype = allowedTypes().find(keys[1]);
    if (itype == allowedTypes().end()) {
        return ::ShowError("type", allowedTypes());
    }

    const auto& type = itype->second;

    uint32_t typeId{0};
    if (type == AttributeTypes::Param) {
        auto id = allowedParamIDs().find(keys[2]);
        if (id == allowedParamIDs().end()) {
            return ::ShowError("param", allowedParamIDs());
        }
        typeId = id->second;
    } else if (type == AttributeTypes::Oracles) {
        auto id = allowedOracleIDs().find(keys[2]);
        if (id == allowedOracleIDs().end()) {
            return ::ShowError("oracles", allowedOracleIDs());
        }
        typeId = id->second;
    } else {
        auto id = VerifyPositiveInt32(keys[2]);
        if (!id) {
            return std::move(id);
        }
        typeId = *id.val;
    }

    uint8_t typeKey;
    uint32_t oracleKey{0};

    if (type != AttributeTypes::Oracles) {
        auto ikey = allowedKeys().find(type);
        if (ikey == allowedKeys().end()) {
            return Res::Err("Unsupported type {%d}", type);
        }

        itype = ikey->second.find(keys[3]);
        if (itype == ikey->second.end()) {
            return ::ShowError("key", ikey->second);
        }

        typeKey = itype->second;
    } else {
        typeKey = OracleIDs::Splits;
        if (const auto keyValue = VerifyPositiveInt32(keys[3])) {
            oracleKey = *keyValue;
        }
    }

    try {
        if (auto parser = parseValue().at(type).at(typeKey)) {
            auto attribValue = parser(value);
            if (!attribValue) {
                return std::move(attribValue);
            }

            if (type == AttributeTypes::Oracles) {
                return applyVariable(CDataStructureV0{type, typeId, oracleKey}, *attribValue);
            }

            return applyVariable(CDataStructureV0{type, typeId, typeKey}, *attribValue);
        }
    } catch (const std::out_of_range&) {
    }
    return Res::Err("No parse function {%d, %d}", type, typeKey);
}

Res ATTRIBUTES::Import(const UniValue & val) {
    if (!val.isObject()) {
        return Res::Err("Object of values expected");
    }

    std::map<std::string, UniValue> objMap;
    val.getObjMap(objMap);

    for (const auto& [key, value] : objMap) {
        auto res = ProcessVariable(
            key, value.get_str(), [this](const CAttributeType& attribute, const CAttributeValue& attrValue) {
                if (auto attrV0 = std::get_if<CDataStructureV0>(&attribute)) {
                    if (attrV0->type == AttributeTypes::Live ||
                        attrV0->key == TokenKeys::Ascendant ||
                        attrV0->key == TokenKeys::Descendant ||
                        attrV0->key == TokenKeys::Epitaph) {
                        return Res::Err("Attribute cannot be set externally");
                    } else if (attrV0->type == AttributeTypes::Oracles && attrV0->typeId == OracleIDs::Splits) {
                        try {
                            auto attrMap = std::get_if<OracleSplits>(&attributes.at(attribute));
                            auto splitValue = std::get_if<OracleSplits>(&attrValue);
                            if (!splitValue) {
                                return Res::Err("Failed to get Oracle split value");
                            }
                            OracleSplits combined{*splitValue};
                            combined.merge(*attrMap);
                            attributes[attribute] = combined;
                            return Res::Ok();
                        } catch (std::out_of_range&) {}
                    }
                }
                attributes[attribute] = attrValue;
                return Res::Ok();
            }
        );
        if (!res) {
            return res;
        }
    }
    return Res::Ok();
}

UniValue ATTRIBUTES::Export() const {
    UniValue ret(UniValue::VOBJ);
    for (const auto& attribute : attributes) {
        auto attrV0 = std::get_if<CDataStructureV0>(&attribute.first);
        std::string key;
        if (attrV0) {
            std::string id;
            if (attrV0->type == AttributeTypes::Param || attrV0->type == AttributeTypes::Live) {
                id = displayParamsIDs().at(attrV0->typeId);
            } else if (attrV0->type == AttributeTypes::Oracles) {
                id = displayOracleIDs().at(attrV0->typeId);
            } else {
                id = KeyBuilder(attrV0->typeId);
            }

            auto const v0Key = attrV0->type == AttributeTypes::Oracles ? KeyBuilder(attrV0->key) : displayKeys().at(attrV0->type).at(attrV0->key);

            key = KeyBuilder(displayVersions().at(VersionTypes::v0),
                                  displayTypes().at(attrV0->type),
                                  id,
                                  v0Key);
        }
        try {
            if (auto bool_val = std::get_if<bool>(&attribute.second)) {
                ret.pushKV(key, *bool_val ? "true" : "false");
            } else if (auto amount = std::get_if<CAmount>(&attribute.second)) {
                auto uvalue = ValueFromAmount(*amount);
                ret.pushKV(key, KeyBuilder(uvalue.get_real()));
            } else if (auto balances = std::get_if<CBalances>(&attribute.second)) {
                ret.pushKV(key, AmountsToJSON(balances->balances));
            } else if (auto currencyPair = std::get_if<CTokenCurrencyPair>(&attribute.second)) {
                ret.pushKV(key, currencyPair->first + '/' + currencyPair->second);
            } else if (const auto splitValues = std::get_if<OracleSplits>(&attribute.second)) {
                std::string keyValue;
                for (const auto& [tokenId, multiplier] : *splitValues) {
                    keyValue += KeyBuilder(tokenId, multiplier) + ',';
                }
                ret.pushKV(key, keyValue);
            } else if (const auto& descendantPair = std::get_if<DescendantValue>(&attribute.second)) {
                ret.pushKV(key, KeyBuilder(descendantPair->first, descendantPair->second));
            } else if (const auto& ascendantPair = std::get_if<AscendantValue>(&attribute.second)) {
                ret.pushKV(key, KeyBuilder(ascendantPair->first, ascendantPair->second));
            }
        } catch (const std::out_of_range&) {
            // Should not get here, that's mean maps are mismatched
        }
    }
    return ret;
}

Res ATTRIBUTES::Validate(const CCustomCSView & view) const
{
    if (view.GetLastHeight() < Params().GetConsensus().FortCanningHillHeight)
        return Res::Err("Cannot be set before FortCanningHill");

    for (const auto& attribute : attributes) {
        auto attrV0 = std::get_if<CDataStructureV0>(&attribute.first);
        if (!attrV0) {
            return Res::Err("Unsupported version");
        }
        switch (attrV0->type) {
            case AttributeTypes::Token:
                switch (attrV0->key) {
                    case TokenKeys::PaybackDFI:
                    case TokenKeys::PaybackDFIFeePCT:
                        if (!view.GetLoanTokenByID({attrV0->typeId})) {
                            return Res::Err("No such loan token (%d)", attrV0->typeId);
                        }
                        break;
                    case TokenKeys::DexInFeePct:
                    case TokenKeys::DexOutFeePct:
                        if (view.GetLastHeight() < Params().GetConsensus().GreatWorldHeight) {
                            return Res::Err("Cannot be set before GreatWorld");
                        }
                        if (!view.GetToken(DCT_ID{attrV0->typeId})) {
                            return Res::Err("No such token (%d)", attrV0->typeId);
                        }
                        break;
                    case TokenKeys::LoanCollateralEnabled:
                    case TokenKeys::LoanCollateralFactor:
                    case TokenKeys::LoanMintingEnabled:
                    case TokenKeys::LoanMintingInterest: {
                        if (view.GetLastHeight() < Params().GetConsensus().GreatWorldHeight) {
                            return Res::Err("Cannot be set before GreatWorld");
                        }
                        if (!VerifyToken(view, attrV0->typeId)) {
                            return Res::Err("No such token (%d)", attrV0->typeId);
                        }
                        CDataStructureV0 intervalPriceKey{AttributeTypes::Token, attrV0->typeId,
                                                          TokenKeys::FixedIntervalPriceId};
                        if (GetValue(intervalPriceKey, CTokenCurrencyPair{}) == CTokenCurrencyPair{}) {
                            return Res::Err("Fixed interval price currency pair must be set first");
                        }
                        break;
                    }
                    case TokenKeys::FixedIntervalPriceId:
                        if (view.GetLastHeight() < Params().GetConsensus().GreatWorldHeight) {
                            return Res::Err("Cannot be set before GreatWorld");
                        }
                        if (!VerifyToken(view, attrV0->typeId)) {
                            return Res::Err("No such token (%d)", attrV0->typeId);
                        }
                        break;
                    case TokenKeys::Ascendant:
                    case TokenKeys::Descendant:
                    case TokenKeys::Epitaph:
                        break;
                    default:
                        return Res::Err("Unsupported key");
                }
                break;

            case AttributeTypes::Oracles:
                if (view.GetLastHeight() < Params().GetConsensus().GreatWorldHeight) {
                    return Res::Err("Cannot be set before GreatWorld");
                }
                if (attrV0->type == AttributeTypes::Oracles && attrV0->typeId == OracleIDs::Splits) {
                    const auto value = std::get_if<OracleSplits>(&attribute.second);
                    if (!value) {
                        return Res::Err("Unsupported value");
                    }
                    for (const auto& [tokenId, multipler] : *value) {
                        if (tokenId == 0) {
                            return Res::Err("Tokenised DFI cannot be split");
                        }
                        if (view.HasPoolPair({tokenId})) {
                            return Res::Err("Pool tokens cannot be split");
                        }
                        const auto token = view.GetToken(DCT_ID{tokenId});
                        if (!token) {
                            return Res::Err("Token (%d) does not exist", tokenId);
                        }
                        if (!token->IsDAT()) {
                            return Res::Err("Only DATs can be split");
                        }
                    }
                } else {
                    return Res::Err("Unsupported key");
                }
                break;

            case AttributeTypes::Poolpairs:
                if (!std::get_if<CAmount>(&attribute.second)) {
                    return Res::Err("Unsupported value");
                }
                switch (attrV0->key) {
                    case PoolKeys::TokenAFeePCT:
                    case PoolKeys::TokenBFeePCT:
                        if (!view.GetPoolPair({attrV0->typeId})) {
                            return Res::Err("No such pool (%d)", attrV0->typeId);
                        }
                        break;
                    default:
                        return Res::Err("Unsupported key");
                }
                break;

            case AttributeTypes::Param:
                if (attrV0->typeId != ParamIDs::DFIP2201) {
                    return Res::Err("Unrecognised param id");
                }
                break;

                // Live is set internally
            case AttributeTypes::Live:
                break;

            default:
                return Res::Err("Unrecognised type (%d)", attrV0->type);
        }
    }

    return Res::Ok();
}

Res ATTRIBUTES::Apply(CCustomCSView & mnview, const uint32_t height)
{
    for (const auto& attribute : attributes) {
        auto attrV0 = std::get_if<CDataStructureV0>(&attribute.first);
        if (!attrV0) {
            continue;
        }
        if (attrV0->type == AttributeTypes::Poolpairs) {
            auto poolId = DCT_ID{attrV0->typeId};
            auto pool = mnview.GetPoolPair(poolId);
            if (!pool) {
                return Res::Err("No such pool (%d)", poolId.v);
            }
            auto tokenId = attrV0->key == PoolKeys::TokenAFeePCT ?
                           pool->idTokenA : pool->idTokenB;

            auto valuePct = std::get<CAmount>(attribute.second);
            if (auto res = mnview.SetDexFeePct(poolId, tokenId, valuePct); !res) {
                return res;
            }
        } else if (attrV0->type == AttributeTypes::Token) {
            if (attrV0->key == TokenKeys::DexInFeePct
                ||  attrV0->key == TokenKeys::DexOutFeePct) {
                DCT_ID tokenA{attrV0->typeId}, tokenB{~0u};
                if (attrV0->key == TokenKeys::DexOutFeePct) {
                    std::swap(tokenA, tokenB);
                }
                auto valuePct = std::get<CAmount>(attribute.second);
                if (auto res = mnview.SetDexFeePct(tokenA, tokenB, valuePct); !res) {
                    return res;
                }
            } else if (attrV0->key == TokenKeys::FixedIntervalPriceId) {
                if (const auto& currencyPair = std::get_if<CTokenCurrencyPair>(&attribute.second)) {
                    // Already exists, skip.
                    if (mnview.GetFixedIntervalPrice(*currencyPair)) {
                        continue;
                    } else if (!OraclePriceFeed(mnview, *currencyPair)) {
                        return Res::Err("Price feed %s/%s does not belong to any oracle", currencyPair->first, currencyPair->second);
                    }
                    CFixedIntervalPrice fixedIntervalPrice;
                    fixedIntervalPrice.priceFeedId = *currencyPair;
                    fixedIntervalPrice.timestamp = time;
                    fixedIntervalPrice.priceRecord[1] = -1;
                    const auto aggregatePrice = GetAggregatePrice(mnview,
                                                                  fixedIntervalPrice.priceFeedId.first,
                                                                  fixedIntervalPrice.priceFeedId.second,
                                                                  time);
                    if (aggregatePrice) {
                        fixedIntervalPrice.priceRecord[1] = aggregatePrice;
                    }
                    mnview.SetFixedIntervalPrice(fixedIntervalPrice);
                } else {
                    return Res::Err("Unrecognised value for FixedIntervalPriceId");
                }
            }
        }
    }
    return Res::Ok();
}
