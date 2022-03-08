#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) DeFi Blockchain Developers
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.
"""Test smart contracts future"""

from test_framework.test_framework import DefiTestFramework

from test_framework.authproxy import JSONRPCException
from test_framework.util import assert_equal, assert_raises_rpc_error
from decimal import Decimal
import time
import calendar

class SmartContractFutureTest(DefiTestFramework):
    account0 = None
    oracle_id1 = None
    symbolDFI = "DFI"
    symbolDOGE = "DOGE"
    symboldUSD = "DUSD"
    idDFI = 0
    iddUSD = 0
    idDOGE = 0
    dfipDeposit = 'futureswap/deposit'
    dfipList = 'futureswap/list'
    dfipxxxAddress = "bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpsqgljc"
    period_blocks = 10
    reward_pct = 0.1

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-txnotokens=0', '-amkheight=1', '-bayfrontheight=1', '-eunosheight=1', '-fortcanningheight=1', '-fortcanninghillheight=1010', '-subsidytest=1', '-txindex=1', '-jellyfish_regtest=1']]

    def rollback(self, count):
        block = self.nodes[0].getblockhash(count)
        self.nodes[0].invalidateblock(block)
        self.nodes[0].clearmempool()

    def test_load_account0_with_DFI(self):
        print('loading up account0 with DFI token...')
        self.nodes[0].generate(100) # get initial UTXO balance from immature to trusted -> check getbalances()
        self.account0 = self.nodes[0].get_genesis_keys().ownerAuthAddress
        # UTXO -> token
        self.nodes[0].utxostoaccount({self.account0: "199999900@" + self.symbolDFI})
        self.nodes[0].generate(1)
        account0_balance = self.nodes[0].getaccount(self.account0)
        assert_equal(account0_balance, ['199999900.00000000@DFI'])

    def test_setup_oracles(self):
        print('setting up oracles...')
        oracle_address1 = self.nodes[0].getnewaddress("", "legacy")
        price_feeds1 = [{"currency": "USD", "token": "DFI"},
                        {"currency": "USD", "token": "DOGE"}]
        self.oracle_id1 = self.nodes[0].appointoracle(oracle_address1, price_feeds1, 10)
        self.nodes[0].generate(1)
        oracle1_prices = [{"currency": "USD", "tokenAmount": "1@DOGE"},
                          {"currency": "USD", "tokenAmount": "10@DFI"}]
        timestamp = calendar.timegm(time.gmtime())
        self.nodes[0].setoracledata(self.oracle_id1, timestamp, oracle1_prices)
        self.nodes[0].generate(120) # let active price update
        oracle_data = self.nodes[0].getoracledata(self.oracle_id1)
        assert_equal(len(oracle_data["priceFeeds"]), 2)
        assert_equal(len(oracle_data["tokenPrices"]), 2)

    def test_setup_tokens(self):
        print('setting up loan and collateral tokens...')
        self.nodes[0].setloantoken({
                    'symbol': self.symboldUSD,
                    'name': "DUSD stable token",
                    'fixedIntervalPriceId': "DUSD/USD",
                    'mintable': True,
                    'interest': 0})

        self.tokenInterest = Decimal(1)
        self.nodes[0].setloantoken({
                    'symbol': self.symbolDOGE,
                    'name': "DOGE token",
                    'fixedIntervalPriceId': "DOGE/USD",
                    'mintable': True,
                    'interest': Decimal(self.tokenInterest*100)})
        self.nodes[0].generate(1)

        # Set token ids
        self.iddUSD = list(self.nodes[0].gettoken(self.symboldUSD).keys())[0]
        self.idDFI = list(self.nodes[0].gettoken(self.symbolDFI).keys())[0]
        self.idDOGE = list(self.nodes[0].gettoken(self.symbolDOGE).keys())[0]

        # Mint tokens
        self.nodes[0].minttokens("1000000@DOGE")
        self.nodes[0].generate(1)
        self.nodes[0].minttokens("2000000@" + self.symboldUSD) # necessary for pools
        self.nodes[0].generate(1)

        # Setup collateral tokens
        self.nodes[0].setcollateraltoken({
                    'token': self.idDFI,
                    'factor': 1,
                    'fixedIntervalPriceId': "DFI/USD"})
        self.nodes[0].generate(300)

        assert_equal(len(self.nodes[0].listtokens()), 3)
        assert_equal(len(self.nodes[0].listloantokens()), 2)
        assert_equal(len(self.nodes[0].listcollateraltokens()), 1)

    def test_setup_poolpairs(self):
        print("setting up pool pairs...")
        poolOwner = self.nodes[0].getnewaddress("", "legacy")
        self.nodes[0].createpoolpair({
            "tokenA": self.iddUSD,
            "tokenB": self.idDFI,
            "commission": Decimal('0.002'),
            "status": True,
            "ownerAddress": poolOwner,
            "pairSymbol": "DUSD-DFI",
        }, [])

        self.nodes[0].createpoolpair({
            "tokenA": self.iddUSD,
            "tokenB": self.idDOGE,
            "commission": Decimal('0.002'),
            "status": True,
            "ownerAddress": poolOwner,
            "pairSymbol": "DUSD-DOGE",
        }, [])
        self.nodes[0].generate(1)

        self.nodes[0].addpoolliquidity({
            self.account0: ["1000000@" + self.symboldUSD, "1000000@" + self.symbolDFI]
        }, self.account0, [])
        self.nodes[0].generate(1)

        self.nodes[0].addpoolliquidity({
            self.account0: ["1000000@" + self.symboldUSD, "1000@" + self.symbolDOGE]
        }, self.account0, [])
        self.nodes[0].generate(1)
        assert_equal(len(self.nodes[0].listpoolpairs()), 2)

    def setup(self):
        print('Generating initial chain...')
        self.test_load_account0_with_DFI()
        self.test_setup_oracles()
        self.test_setup_tokens()
        self.test_setup_poolpairs()

    def test_beforeFCH(self):
        # Check invalid calls
        assert_raises_rpc_error(-32600, 'called before FortCanningHill height', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE, self.account0)

    def test_activate_dfip(self):
        assert_raises_rpc_error(-32600, 'DFIPXXXX smart contract is not enabled', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE, self.account0)
        self.nodes[0].setgov({"ATTRIBUTES":{'v0/params/dfipxxxx/active':'false'}})
        self.nodes[0].generate(1)

        assert_raises_rpc_error(-32600, 'DFIPXXXX smart contract is not enabled', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE, self.account0)
        self.nodes[0].setgov({"ATTRIBUTES":{'v0/params/dfipxxxx/active':'true'}})
        self.nodes[0].generate(1)

    def test_invalid_input(self):
        assert_raises_rpc_error(-3, 'Amount out of range', self.nodes[0].executesmartcontract, self.dfipDeposit, '-1@2', self.account0)
        assert_raises_rpc_error(-5, 'Invalid address', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE, "000000000000000000000000000000000000000000")
        assert_raises_rpc_error(-8, 'Loan token source address must be provided for DFIPXXXX', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE)
        assert_raises_rpc_error(-32600, 'No such loan token', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@9999', self.account0)
        assert_raises_rpc_error(-32600, 'Token DUSD is not allowed in DFIPXXXXContract', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.iddUSD, self.account0)
        assert_raises_rpc_error(-32600, 'No such loan token id 0', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@0', self.account0)

    def test_list_smart_contracts(self):
        result = self.nodes[0].listsmartcontracts()
        assert_equal(result[0]['name'], 'DFIP2201')
        assert_equal(result[0]['call'], 'dbtcdfiswap')
        assert_equal(result[0]['address'], 'bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqdku202')
        assert_equal(result[1]['name'], 'DFIPXXXX')
        assert_equal(result[1]['call'], 'futureswap')
        assert_equal(result[1]['address'], self.dfipxxxAddress)

    def test_fund_smart_contract(self):
        balanceBefore = self.nodes[0].getbalance()
        accountBefore = self.nodes[0].getaccount(self.account0)
        [amountDogeBefore, _] = accountBefore[1].split("@")

        tx = self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, self.account0)
        fee = self.nodes[0].gettransaction(tx)['fee']
        assert_equal(balanceBefore + fee, self.nodes[0].getbalance())
        self.nodes[0].generate(1)

        result = self.nodes[0].listsmartcontracts()
        assert_equal(result[1][self.idDOGE], Decimal('100'))

        accountAfter = self.nodes[0].getaccount(self.account0)
        [amountDogeAfter, _] = accountAfter[1].split("@")
        assert_equal(float(amountDogeBefore), float(amountDogeAfter) + 100)

    def test_desactivated_token(self):
        self.nodes[0].setgov({'ATTRIBUTES': {'v0/token/' + self.idDOGE +'/futureswap': 'false' }})
        self.nodes[0].generate(1)
        assert_raises_rpc_error(-32600, 'DOGE is not an active DFIPXXX token!', self.nodes[0].executesmartcontract, self.dfipDeposit, '10@' + self.idDOGE, self.account0)

        self.nodes[0].setgov({'ATTRIBUTES': {'v0/token/' + self.idDOGE +'/futureswap': 'true' }})
        self.nodes[0].generate(1)
        self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, self.account0)
        self.nodes[0].generate(1)

    def test_automatic_swap(self):
        self.nodes[0].setgov({"ATTRIBUTES":{'v0/params/dfipxxxx/period_blocks': self.period_blocks}})
        self.nodes[0].generate(10)

        newAddress = self.nodes[0].getnewaddress()
        self.nodes[0].accounttoaccount(self.account0, { newAddress: "100@" + self.idDOGE })
        self.nodes[0].generate(1)

        self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, newAddress)
        self.nodes[0].generate(1)

        balancesBefore = self.nodes[0].getaccount(newAddress)
        balancesContractBefore = self.nodes[0].getaccount(self.dfipxxxAddress)
        self.nodes[0].generate(10)

        balancesAfter = self.nodes[0].getaccount(newAddress)
        balancesContractAfter = self.nodes[0].getaccount(self.dfipxxxAddress)

        assert_equal(balancesBefore, [])
        assert_equal(balancesAfter, ["105.00000000@DUSD"]) # 5% default premium

        assert_equal(balancesContractBefore, ["100.00000000@DOGE"])
        assert_equal(balancesContractAfter, [])

    def test_token_tracking(self):
        gettokendusd = self.nodes[0].gettoken(self.iddUSD)[self.iddUSD]
        assert_equal(gettokendusd['minted'], 2000315)

        getburninfo = self.nodes[0].getburninfo()
        assert_equal(getburninfo['tokens'], ['300.00000000@DOGE'])

        history = self.nodes[0].listburnhistory()
        assert_equal(len(history), 2)
        assert_equal(history[0]['amounts'], ['100.00000000@DOGE'])
        assert_equal(history[1]['amounts'], ['200.00000000@DOGE'])
        assert_equal(history[0]['type'], 'SmartContract')
        assert_equal(history[1]['type'], 'SmartContract')
        assert_equal(history[0]['blockHeight'], history[1]['blockHeight'] + self.period_blocks)

    def test_account_history(self):
        smartContractHistory = self.nodes[0].listaccounthistory("mine", {"txtype":"K"})
        assert_equal(smartContractHistory[0]["amounts"], ['105.00000000@DUSD'])
        assert_equal(smartContractHistory[1]["amounts"], ['-100.00000000@DOGE'])
        assert_equal(smartContractHistory[2]["amounts"], ['210.00000000@DUSD'])
        assert_equal(smartContractHistory[3]["amounts"], ['-100.00000000@DOGE'])
        assert_equal(smartContractHistory[4]["amounts"], ['-100.00000000@DOGE'])

    def test_reward_pct(self):
        self.nodes[0].setgov({"ATTRIBUTES":{'v0/params/dfipxxxx/reward_pct': self.reward_pct}})
        self.nodes[0].generate(1)

        firstAddress = self.nodes[0].getnewaddress()
        self.nodes[0].accounttoaccount(self.account0, { firstAddress: "100@" + self.idDOGE })
        self.nodes[0].generate(1)

        self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, firstAddress)
        self.nodes[0].generate(1)

        self.nodes[0].generate(10)

        balances = self.nodes[0].getaccount(firstAddress)
        assert_equal(balances, ["110.00000000@DUSD"]) # 10% reward pct

        self.nodes[0].setgov({'ATTRIBUTES': {'v0/token/' + self.idDOGE +'/futureswap_reward_pct': 0.1 }}) # set doge token reward pct
        self.nodes[0].generate(1)

        secondAddress = self.nodes[0].getnewaddress()
        self.nodes[0].accounttoaccount(self.account0, { secondAddress: "100@" + self.idDOGE })
        self.nodes[0].generate(1)

        self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, secondAddress)
        self.nodes[0].generate(1)

        self.nodes[0].generate(10)

        balances = self.nodes[0].getaccount(secondAddress)
        assert_equal(balances, ["120.00000000@DUSD"]) # 10% reward pct

    def test_dfip_list(self):
        address = self.nodes[0].getnewaddress()
        self.nodes[0].accounttoaccount(self.account0, { address: "100@" + self.idDOGE })
        self.nodes[0].generate(1)

        self.nodes[0].executesmartcontract(self.dfipDeposit, '100@' + self.idDOGE, address)
        self.nodes[0].generate(1)

        list_balances = self.nodes[0].executesmartcontract(self.dfipList, "*", address)
        assert_equal(list_balances, ["100.00000000@" + self.idDOGE])

        otherAddress = self.nodes[0].getnewaddress()
        other_balances = self.nodes[0].executesmartcontract(self.dfipList, "*", otherAddress)
        assert_equal(other_balances, [])

        assert_raises_rpc_error(-8, 'Amount key should be set to dummy "*" for futureswap/list', self.nodes[0].executesmartcontract, self.dfipList, "not_*")
        assert_raises_rpc_error(-8, 'Loan token source address must be provided to list', self.nodes[0].executesmartcontract, self.dfipList, "*")
        assert_raises_rpc_error(-5, 'Invalid address', self.nodes[0].executesmartcontract, self.dfipList, "*", '000000000000000000000000000000000000000000')
        assert_raises_rpc_error(-5, 'Invalid address', self.nodes[0].executesmartcontract, self.dfipList, "*", 'not_an_address')


    def run_test(self):
        self.setup()

        self.test_beforeFCH()
        # Move to FortCanningHill
        self.nodes[0].generate(1010 - self.nodes[0].getblockcount())

        self.test_activate_dfip()
        self.test_invalid_input()
        self.test_list_smart_contracts()
        self.test_fund_smart_contract()
        self.test_desactivated_token()
        self.test_automatic_swap()
        self.test_token_tracking()
        self.test_account_history()
        self.test_reward_pct()
        self.test_dfip_list()

if __name__ == '__main__':
    SmartContractFutureTest().main()

