import time

import pytest

from test_tools import Account, Asset, logger, exceptions, Wallet, World


@pytest.mark.parametrize(
    'wallet_bridge_api_command', [
        'broadcast_transaction',
        'broadcast_transaction_synchronous',
        'find_proposals',
        'find_rc_accounts',
        'find_recurrent_transfers',
        'get_account',
        'get_account_history',
        'get_accounts',
        'get_block',
        'get_collateralized_conversion_requests',
        'get_conversion_requests',
        'get_open_orders',
        'get_ops_in_block',
        'get_order_book',
        'get_owner_history',
        'get_reward_fund',
        'get_transaction',
        'get_withdraw_routes',
        'get_witness',
        'is_known_transaction',
        'list_accounts',
        'list_my_accounts',
        'list_proposal_votes',
        'list_proposals',
        'list_rc_accounts',
        'list_rc_direct_delegations',
        'list_witnesses',
    ]
)
def test_run_command_without_arguments_where_arguments_are_required(node, wallet_bridge_api_command):
    with pytest.raises(exceptions.CommunicationError):
        getattr(node.api.wallet_bridge, wallet_bridge_api_command)()


@pytest.mark.parametrize(
    'wallet_bridge_api_command', [
        'get_active_witnesses',
        'get_chain_properties',
        'get_current_median_history_price',
        'get_dynamic_global_properties',
        'get_feed_history',
        'get_hardfork_version',
        'get_witness_schedule',
    ]
)
def test_run_command_with_arguments_where_arguments_should_not_exist(node, wallet_bridge_api_command):
    with pytest.raises(TypeError):
        getattr(node.api.wallet_bridge, wallet_bridge_api_command)('example_string_argument')


def test_broadcast_transaction(node): #Robi Michał
    pass


def test_broadcast_transaction_synchronous(): #Robi Michał
    pass


def test_find_rc_accounts():#Robi Michał
    pass


def test_find_recurrent_transfers():
    pass


def test_get_account():
    pass


def test_get_account_history():
    pass


def test_get_accounts():
    pass


def test_get_block():
    pass


def test_get_chain_properties(node):
    # bez argumentów

    pass
def test_get_collateralized_conversion_requests():
    pass


def test_get_conversion_requests():
    pass


def test_get_current_median_history_price(node):
    # bez argumentów

    pass
def test_get_dynamic_global_properties(node):
    # bez argumentów
    pass


def test_get_feed_history():
    # bez argumentów
    pass


def test_get_hardfork_version():
    # bez argumentów
    pass


def test_get_open_orders():
    pass


def test_get_ops_in_block():
    pass


def test_get_order_book():
    pass


def test_get_owner_history():
    pass


def test_get_reward_fund():
    pass


def test_get_transaction():
    pass


def test_get_version(node):
    pass


def test_get_withdraw_routes():
    pass


def test_is_known_transaction():
    pass


def test_list_accounts():
    pass


def test_list_my_accounts():
    pass


def test_list_rc_accounts():#Robi Michał
    pass


def test_list_rc_direct_delegations():#Robi Michał
    pass
