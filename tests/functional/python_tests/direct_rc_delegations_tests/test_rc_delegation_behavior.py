import pytest

from test_tools import Asset, exceptions, Wallet


def test_delegated_rc_account_execute_ops(wallet: Wallet):
    accounts = []
    number_of_accounts_in_one_transaction = 10
    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts_in_one_transaction):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))
    wallet.api.delegate_rc(accounts[0], [accounts[1]], 100)
    wallet.api.create_account(accounts[1], 'alice', '{}')


def test_undelegated_rc_account_reject_execute_ops(wallet: Wallet):
    accounts = []
    number_of_accounts_in_one_transaction = 10
    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts_in_one_transaction):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))
    wallet.api.transfer('initminer', accounts[0], Asset.Test(1), '')

    wallet.api.delegate_rc(accounts[0], [accounts[1]], 100)
    wallet.api.create_account(accounts[1], 'alice', '{}')

    wallet.api.delegate_rc(accounts[0], [accounts[1]], 0)

    with pytest.raises(exceptions.CommunicationError):
        wallet.api.create_account(accounts[1], 'bob', '{}')


def test_delegations_when_delegator_lost_power(wallet: Wallet):
    accounts = []
    number_of_accounts_in_one_transaction = 10
    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts_in_one_transaction):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.01))
    state1 = wallet.api.find_rc_accounts([accounts[0]])
    state2 = wallet.api.get_account(accounts[0])
    number_of_rc = rc_account_info(accounts[0], 'max_rc', wallet)
    wallet.api.delegate_rc(accounts[0], [accounts[1]], number_of_rc - 4)

    assert rc_account_info(accounts[0], 'rc_manabar', wallet)['current_mana'] > 3

    state3 = wallet.api.find_rc_accounts([accounts[0]])
    vests_to_withdraw = account_info(accounts[0], 'vesting_shares', wallet)
    wallet.api.withdraw_vesting(accounts[0], vests_to_withdraw)

    state4 = wallet.api.find_rc_accounts([accounts[0]])
    state5 = wallet.api.get_account(accounts[0])

    pass


def test_same_value_rc_delegation(node, wallet: Wallet):

    accounts = []
    number_of_accounts_in_one_transaction = 10
    number_of_transactions = 1
    for number_of_transaction in range(number_of_transactions):
        with wallet.in_single_transaction():
            for account_number in range(number_of_accounts_in_one_transaction):
                wallet.api.create_account('initminer', f'account-{account_number}', '{}')
                accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(10))
    wallet.api.transfer_to_vesting('initminer', accounts[1], Asset.Test(10))

    wallet.api.delegate_rc(accounts[0], [accounts[2]], 10)
    wallet.api.delegate_rc(accounts[1], [accounts[2]], 10)

    with pytest.raises(exceptions.CommunicationError):
        # Can not make same delegation RC two times
        wallet.api.delegate_rc(accounts[0], [accounts[2]], 10)

    node.wait_number_of_blocks(3)

    with pytest.raises(exceptions.CommunicationError):
        # Can not make same delegation RC two times
        wallet.api.delegate_rc(accounts[0], [accounts[2]], 10)


def test_delegations_rc_to_one_receiver(wallet: Wallet):
    accounts = []
    number_of_accounts = 120

    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    number_of_transfers = 100
    account_number_absolute = 1
    with wallet.in_single_transaction():
        for transfer_number in range(number_of_transfers):
            wallet.api.transfer_to_vesting('initminer', accounts[account_number_absolute], Asset.Test(100000))
            wallet.api.transfer('initminer', accounts[account_number_absolute], Asset.Test(10), '')
            account_number_absolute = account_number_absolute + 1

    number_of_delegations = 100
    account_number_absolute = 1
    with wallet.in_single_transaction():
        for delegation_number in range(number_of_delegations):
            wallet.api.delegate_rc(accounts[account_number_absolute], [accounts[0]], 10)
            account_number_absolute = account_number_absolute + 1


def test_reject_of_delegation_of_delegated_rc(wallet: Wallet):
    accounts = []
    number_of_accounts = 10

    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))
    wallet.api.delegate_rc(accounts[0], [accounts[1]], 100)

    with pytest.raises(exceptions.CommunicationError):
        wallet.api.delegate_rc(accounts[1], [accounts[2]], 50)


def test_cost_of_doing_transaction(wallet: Wallet):
    accounts = []
    number_of_accounts = 10

    with wallet.in_single_transaction():
        for account_number in range(number_of_accounts):
            wallet.api.create_account('initminer', f'account-{account_number}', '{}')
            accounts.append(f'account-{account_number}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))

    wallet.api.delegate_rc(accounts[0], [accounts[1]], 100)
    number_of_mana = rc_account_info(accounts[0], 'rc_manabar', wallet)['current_mana']
    number_of_rc = rc_account_info(accounts[0], 'max_rc', wallet)
    assert number_of_mana == number_of_rc - 3  # cost of operation is 3

    wallet.api.delegate_rc(accounts[0], [accounts[1]], 3000)
    number_of_mana = rc_account_info(accounts[0], 'rc_manabar', wallet)['current_mana']
    number_of_rc = rc_account_info(accounts[0], 'max_rc', wallet)
    assert number_of_mana == number_of_rc - 3 - 3


def rc_account_info(account, name_of_data, wallet):
    data_set = wallet.api.find_rc_accounts([account])['result'][0]
    specyfic_data = data_set[name_of_data]
    return specyfic_data


def account_info(account, name_of_data, wallet):
    data_set = wallet.api.get_account(account)['result']
    specyfic_data = data_set[name_of_data]
    return specyfic_data
