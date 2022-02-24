import pytest

from test_tools import Asset, logger, exceptions, Wallet, World


@pytest.mark.parametrize(
    'wallet_bridge_api_command', [
        'get_block',
        'get_ops_in_block',
        'get_withdraw_routes',
        'list_my_accounts',
        'list_accounts',
        'get_account',
        'get_accounts',
        'get_transaction',
        'list_witnesses',
        'get_witness',
        'get_conversion_requests',
        'get_collateralized_conversion_requests',
        'get_order_book',
        'get_open_orders',
        'get_owner_history',
        'get_account_history',
        'list_proposals',
        'find_proposals',
        'is_known_transaction',
        'list_proposal_votes',
        'get_reward_fund',
        'broadcast_transaction_synchronous',
        'broadcast_transaction',
        'find_recurrent_transfers',
        'find_rc_accounts',
        'list_rc_accounts',
        'list_rc_direct_delegations',
    ]
)
def test_run_command_without_arguments_where_arguments_are_required(node, wallet_bridge_api_command):
    getattr(node.api.wallet_bridge, wallet_bridge_api_command)()

    with pytest.raises(exceptions.CommunicationError):
        getattr(node.api.wallet_bridge, wallet_bridge_api_command)()


def get_accounts_name(accounts):
    accounts_names = []
    for account_number in range(len(accounts)):
        accounts_names.append(accounts[account_number].name)
    return accounts_names


def test_find_rc_accounts(node, wallet):
    get_accounts_name(wallet.create_accounts(5, 'account'))

    #   wrong input data
    number = -1
    array = [1]
    tup = (2, "tup", -1)

    #   correct function parameters
    correct_parameter = node.api.wallet_bridge.find_rc_accounts(['account-0', 'account-1'])

    #   wrong function parameters response

    # empty_list: []
    empty_list = node.api.wallet_bridge.find_rc_accounts([])
    # number_parameter: []
    number_parameter = node.api.wallet_bridge.find_rc_accounts([number])

    array_parameter = node.api.wallet_bridge.find_rc_accounts([array[0], 'account-0'])
    x = node.api.wallet_bridge.find_rc_accounts([('account-0'), 'account-1'])
    tuple_parameter = node.api.wallet_bridge.find_rc_accounts([tup[0], 'account-0'])
    bool_parameter = node.api.wallet_bridge.find_rc_accounts([False])

    list_accounts = node.api.wallet_bridge.list_accounts('', 1000)

    acc0rc = node.api.wallet_bridge.find_rc_accounts(['account-0'])
    acc0 = node.api.wallet_bridge.get_account('account-0')
    print()


def test_list_rc_accounts(node, wallet):
    accounts = get_accounts_name(wallet.create_accounts(5, 'account'))
    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))
    wallet.api.delegate_rc(accounts[0], [accounts[1]], 10)

    node_rc = node.api.wallet_bridge.list_rc_accounts('', 10)
    print()


def test_list_rc_direct_delegations(node, wallet):
    accounts = get_accounts_name(wallet.create_accounts(5, 'account'))
    wallet.api.create_account('initminer', 'alice', '{}')

    wallet.api.transfer_to_vesting('initminer', accounts[0], Asset.Test(0.1))
    wallet.api.transfer_to_vesting('initminer', accounts[1], Asset.Test(0.1))
    wallet.api.delegate_rc(accounts[0], [accounts[1], accounts[2]], 100)
    wallet.api.delegate_rc(accounts[0], [accounts[1]], 12)

    #   list_rc_delegations([delegator, receiver], limit(int))
    acc0_to_acc1 = node.api.wallet_bridge.list_rc_direct_delegations([accounts[0], accounts[1]], 10)
    acc0_to_acc0 = node.api.wallet_bridge.list_rc_direct_delegations([accounts[0], accounts[0]], 10)
    acc0_to_acc2 = node.api.wallet_bridge.list_rc_direct_delegations([accounts[0], accounts[2]], 10)
    acc0_to_acc3 = node.api.wallet_bridge.list_rc_direct_delegations([accounts[0], accounts[3]], 100)
    acc1_to_acc0 = node.api.wallet_bridge.list_rc_direct_delegations([accounts[1], accounts[0]], 100)
    # acc4_to_alice = node.api.wallet_bridge.list_rc_direct_delegations([])

    acc0 = node.api.wallet_bridge.get_account('account-0')
    acc00 = node.api.wallet_bridge.get_account('account-0')['name']
    la = wallet.api.list_accounts('', 1000)
    print()


# def test_broadcast_transaction(node, wallet):
#     creator = 'initminer'
#     transaction = wallet.api.create_account(creator, 'jonas', '{}', broadcast=False)
#     transaction = wallet.api.sign_transaction(transaction, broadcast=False)
#
#     node.api.wallet_bridge.broadcast_transaction(transaction)
#     print()
#
#
# def test_broadcast_transaction_synchronous(node, wallet):
#     creator = 'initminer'
#     transaction = wallet.api.create_account(creator, 'jonas', '{}', broadcast=False)
#     transaction = wallet.api.sign_transaction(transaction, broadcast=False)
#
#     node.api.wallet_bridge.broadcast_transaction_synchronous(transaction)
#     print()
