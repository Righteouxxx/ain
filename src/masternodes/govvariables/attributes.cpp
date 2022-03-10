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

static std::vector<std::string> KeyBreaker(const std::string& str){
    std::string section;
    std::istringstream stream(str);
    std::vector<std::string> strVec;

    while (std::getline(stream, section, '/')) {
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
            {"params",      AttributeTypes::Param},
            {"poolpairs",   AttributeTypes::Poolpairs},
            {"token",       AttributeTypes::Token},
    };
    return types;
}

const std::map<uint8_t, std::string>& ATTRIBUTES::displayTypes() {
    static const std::map<uint8_t, std::string> types{
            {AttributeTypes::Live,      "live"},
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

const std::map<uint8_t, std::map<uint8_t, std::string>>& ATTRIBUTES::displayKeys() {
    static const std::map<uint8_t, std::map<uint8_t, std::string>> keys{
            {
                AttributeTypes::Token, {
                    {TokenKeys::PaybackDFI,            "payback_dfi"},
                    {TokenKeys::PaybackDFIFeePCT,      "payback_dfi_fee_pct"},
                    {TokenKeys::DexInFeePct,           "dex_in_fee_pct"},
                    {TokenKeys::DexOutFeePct,          "dex_out_fee_pct"},
                    {TokenKeys::FixedIntervalPriceId,  "fixed_interval_price_id"},
                    {TokenKeys::LoanCollateralEnabled, "loan_collateral_enabled"},
                    {TokenKeys::LoanCollateralFactor,  "loan_collateral_factor"},
                    {TokenKeys::LoanMintingEnabled,    "loan_minting_enabled"},
                    {TokenKeys::LoanMintingInterest,   "loan_minting_interest"},
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

Res ATTRIBUTES::ProcessVariable(const std::string& key, std::optional<std::string> value,
                                std::function<Res(const CAttributeType&, const CAttributeValue&)> applyVariable) const {

    if (key.size() > 128) {
        return Res::Err("Identifier exceeds maximum length (128)");
    }

    const auto keys = KeyBreaker(key);
    if (keys.empty() || keys[0].empty()) {
        return Res::Err("Empty version");
    }

    if (value && value->empty()) {
        return Res::Err("Empty value");
    }

    auto iver = allowedVersions().find(keys[0]);
    if (iver == allowedVersions().end()) {
        return Res::Err("Unsupported version");
    }

    auto version = iver->second;
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

    auto type = itype->second;

    uint32_t typeId{0};
    if (type != AttributeTypes::Param) {
        auto id = VerifyInt32(keys[2]);
        if (!id) {
            return std::move(id);
        }
        typeId = *id.val;
    } else {
        auto id = allowedParamIDs().find(keys[2]);
        if (id == allowedParamIDs().end()) {
            return ::ShowError("param", allowedParamIDs());
        }
        typeId = id->second;
    }

    auto ikey = allowedKeys().find(type);
    if (ikey == allowedKeys().end()) {
        return Res::Err("Unsupported type {%d}", type);
    }

    itype = ikey->second.find(keys[3]);
    if (itype == ikey->second.end()) {
        return ::ShowError("key", ikey->second);
    }

    auto typeKey = itype->second;
    CDataStructureV0 keyV0{type, typeId, typeKey};

    if (!value) {
        return applyVariable(keyV0, {});
    }

    try {
        if (auto parser = parseValue().at(type).at(typeKey)) {
            auto attribValue = parser(*value);
            if (!attribValue) {
                return std::move(attribValue);
            }
            return applyVariable(keyV0, *attribValue.val);
        }
    } catch (const std::out_of_range&) {
    }
    return Res::Err("No parse function {%d, %d}", type, typeKey);
}

bool ATTRIBUTES::IsEmpty() const
{
    return attributes.empty();
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
                        if (attrV0->type == AttributeTypes::Live) {
                            return Res::Err("Live attribute cannot be set externally");
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
        if (!attrV0) {
            continue;
        }
        try {
            const auto id = attrV0->type == AttributeTypes::Param
                            || attrV0->type == AttributeTypes::Live
                          ? displayParamsIDs().at(attrV0->typeId)
                          : KeyBuilder(attrV0->typeId);

            auto key = KeyBuilder(displayVersions().at(VersionTypes::v0),
                                  displayTypes().at(attrV0->type),
                                  id,
                                  displayKeys().at(attrV0->type).at(attrV0->key));

            if (auto bool_val = std::get_if<bool>(&attribute.second)) {
                ret.pushKV(key, *bool_val ? "true" : "false");
            } else if (auto amount = std::get_if<CAmount>(&attribute.second)) {
                auto uvalue = ValueFromAmount(*amount);
                ret.pushKV(key, KeyBuilder(uvalue.get_real()));
            } else if (auto balances = std::get_if<CBalances>(&attribute.second)) {
                ret.pushKV(key, AmountsToJSON(balances->balances));
            } else if (auto currencyPair = std::get_if<CTokenCurrencyPair>(&attribute.second)) {
                ret.pushKV(key, currencyPair->first + '/' + currencyPair->second);
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
                    default:
                        return Res::Err("Unsupported key");
                }
                break;

            case AttributeTypes::Poolpairs:
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

Res ATTRIBUTES::Apply(CCustomCSView & mnview, uint32_t)
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

Res ATTRIBUTES::Erase(CCustomCSView & mnview, uint32_t, std::vector<std::string> const & keys)
{
    for (const auto& key : keys) {
        auto res = ProcessVariable(key, {},
            [&](const CAttributeType& attribute, const CAttributeValue&) {
                auto attrV0 = std::get_if<CDataStructureV0>(&attribute);
                if (!attrV0) {
                    return Res::Ok();
                }
                if (attrV0->type == AttributeTypes::Live) {
                    return Res::Err("Live attribute cannot be deleted");
                }
                if (!attributes.erase(attribute)) {
                    return Res::Err("Attribute {%d} not exists", attrV0->type);
                }
                if (attrV0->type == AttributeTypes::Poolpairs) {
                    auto poolId = DCT_ID{attrV0->typeId};
                    auto pool = mnview.GetPoolPair(poolId);
                    if (!pool) {
                        return Res::Err("No such pool (%d)", poolId.v);
                    }
                    auto tokenId = attrV0->key == PoolKeys::TokenAFeePCT ?
                                                pool->idTokenA : pool->idTokenB;

                    return mnview.EraseDexFeePct(poolId, tokenId);
                } else if (attrV0->type == AttributeTypes::Token) {
                    if (attrV0->key == TokenKeys::DexInFeePct
                    ||  attrV0->key == TokenKeys::DexOutFeePct) {
                        DCT_ID tokenA{attrV0->typeId}, tokenB{~0u};
                        if (attrV0->key == TokenKeys::DexOutFeePct) {
                            std::swap(tokenA, tokenB);
                        }
                        return mnview.EraseDexFeePct(tokenA, tokenB);
                    }
                }
                return Res::Ok();
            }
        );
        if (!res) {
            return res;
        }
    }
    return Res::Ok();
}
