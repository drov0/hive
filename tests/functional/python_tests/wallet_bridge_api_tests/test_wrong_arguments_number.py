import pytest

from test_tools import exceptions


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
    'wallet_bridge_api_command, false_argument', [
        ['get_active_witnesses', 'string_argument'],
        ['get_current_median_history_price', 'string_argument'],
        ['get_dynamic_global_properties', 'string_argument'],
        ['get_feed_history', 'string_argument'],
        ['get_hardfork_version', 'string_argument'],
        ['get_witness_schedule', 'string_argument'],
        ['get_active_witnesses', 10],
        ['get_current_median_history_price', 10],
        ['get_dynamic_global_properties', 10],
        ['get_feed_history', 10],
        ['get_hardfork_version', 10],
        ['get_witness_schedule', 10],
        ['get_active_witnesses', True],
        ['get_current_median_history_price', True],
        ['get_dynamic_global_properties', True],
        ['get_feed_history', True],
        ['get_hardfork_version', True],
        ['get_witness_schedule', True],
    ]
)
def test_run_command_with_arguments_where_arguments_should_not_exist(node, wallet_bridge_api_command, false_argument):
    with pytest.raises(TypeError):
        getattr(node.api.wallet_bridge, wallet_bridge_api_command)(false_argument)
