from datetime import datetime, timedelta

import pytest

import test_tools.exceptions
from test_tools import Asset

list_proposals_constants = {
    'order_type': {
        'by_creator': 29,
        'by_start_date': 30,
        'by_end_date': 31,
    },

    'order_direction': {
        'ascending': 0,
        'descending': 1,
    },

    'status': {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }
}

list_proposals_votes_constants = {
    'order_type': {
        'by_voter_proposal': 33,
        'by_proposal_voter': 34,
    },

    'order_direction': {
        'ascending': 0,
        'descending': 1,
    },

    'status': {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }
}


def test_list_proposals_with_correct_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)

    order_type = list_proposals_constants['order_type']
    order_direction = list_proposals_constants['order_direction']
    status = list_proposals_constants['status']

    # CORRECT VALUES: start
    node.api.wallet_bridge.list_proposals([f"{accounts[1]}"], 100, order_type['by_creator'],
                                          order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([get_future_data(100)], 100, order_type['by_start_date'],
                                          order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([get_future_data(130)], 100, order_type['by_end_date'],
                                          order_direction['ascending'],
                                          status['all'])

    # ON LIMITS: max and min
    node.api.wallet_bridge.list_proposals([""], 0, order_type['by_creator'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([""], 1000, order_type['by_creator'], order_direction['ascending'],
                                          status['all'])

    # ON LIMITS: order_type
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_end_date'], order_direction['ascending'],
                                          status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['descending'],
                                          status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['descending'],
                                          status['votable'])


def test_list_proposals_with_incorrect_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)

    order_type = list_proposals_constants['order_type']
    order_direction = list_proposals_constants['order_direction']
    status = list_proposals_constants['status']

    # INCORRECT VALUES: start
        # start from unexist account  BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([f"unexist_account"], 100, order_type['by_creator'],
                                              order_direction['ascending'],
                                              status['all'])

        # start from a past date  BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(-20)], 100, order_type['by_start_date'],
                                              order_direction['ascending'],
                                              status['all'])
        # start from a date too early  BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(2)], 100, order_type['by_start_date'],
                                              order_direction['ascending'],
                                              status['all'])

        # start from a date too late   INCORRECT EXCEPTION
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(1000)], 100, order_type['by_start_date'],
                                              order_direction['ascending'],
                                              status['all'])

        # start from a past date   BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(-10)], 100, order_type['by_end_date'],
                                              order_direction['ascending'],
                                              status['all'])

        # start from a date too early   BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(2)], 100, order_type['by_end_date'],
                                              order_direction['ascending'],
                                              status['all'])

        # start from a date too late INCORRECT EXCEPTION
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([get_future_data(10000)], 100, order_type['by_end_date'],
                                              order_direction['ascending'],
                                              status['all'])

    # OUT OFF LIMITS: max and min
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], -1, order_type["by_creator"], order_direction['ascending'],
                                              status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 1001, order_type['by_creator'], order_direction['ascending'],
                                              status['all'])

    # OUT OFF LIMITS: order_type
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, 28, order_direction['ascending'], status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, 32, order_direction['ascending'], status['all'])

    # OUT OFF LIMITS: order_direction
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], -1, status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], 2, status['all'])

    # OUT OFF LIMITS: order_direction
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['ascending'], -1)

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([""], 100, order_type['by_creator'], order_direction['descending'], 5)


def test_list_proposal_votes_with_correct_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)
    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.update_proposal_votes(account, [3], 1)

    order_type = list_proposals_votes_constants['order_type']
    order_direction = list_proposals_votes_constants['order_direction']
    status = list_proposals_votes_constants['status']

    # CORRECT VALUES: start
    node.api.wallet_bridge.list_proposal_votes([f"{accounts[1]}"], 100, order_type['by_voter_proposal'],
                                               order_direction['ascending'],
                                               status['all'])
        #BUG with order type
    node.api.wallet_bridge.list_proposal_votes([3], 100, order_type['by_proposal_voter'],
                                               order_direction['ascending'],
                                               status['all'])

    # ON LIMITS: max and min
    node.api.wallet_bridge.list_proposal_votes([""], 0, order_type['by_voter_proposal'], order_direction['ascending'],
                                               status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 1000, order_type['by_voter_proposal'],
                                               order_direction['ascending'],
                                               status['all'])

    # ON LIMITS: order_type
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                               status['all'])
        # BUG with order type
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_proposal_voter'], order_direction['ascending'],
                                               status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                               status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'],
                                               order_direction['descending'],
                                               status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                               status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'],
                                               order_direction['descending'],
                                               status['votable'])


def test_list_proposal_votes_with_incorrect_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)
    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.update_proposal_votes(account, [3], 1)

    order_type = list_proposals_votes_constants['order_type']
    order_direction = list_proposals_votes_constants['order_direction']
    status = list_proposals_votes_constants['status']

    # INCORRECT VALUES: start
        # start from unexist account  BUG
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([f"unexist_account"], 100, order_type['by_voter_proposal'],
                                                   order_direction['ascending'],
                                                   status['all'])

        # start from a minus id  BUG with order type
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([-2], 100, order_type['by_proposal_voter'],
                                                   order_direction['ascending'],
                                                   status['all'])
        # start from a unexist id  BUG with order type
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals([100], 100, order_type['by_proposal_voter'],
                                              order_direction['ascending'],
                                              status['all'])

    # OUT OFF LIMITS: max and min
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], -1, order_type['by_voter_proposal'],
                                                   order_direction['ascending'],
                                                   status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 1001, order_type['by_voter_proposal'],
                                                   order_direction['ascending'],
                                                   status['all'])

    # OUT OFF LIMITS: order_type
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, 32, order_direction['ascending'],
                                                   status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, 35, order_direction['ascending'],
                                                   status['all'])

    # OUT OFF LIMITS: order_direction
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], -1,
                                                   status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], 2,
                                                   status['all'])

    # OUT OFF LIMITS: order_direction
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'],
                                                   order_direction['ascending'], -1)

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'],
                                                   order_direction['descending'], 6)


def test_find_proposals_with_correct_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)

    node.api.wallet_bridge.find_proposals([0, 1, 2, 3, 4, 5])


def test_find_proposals_with_incorrect_values(node, wallet):
    accounts = create_accounts_with_vests_and_TBD(wallet, 5)
    prepare_proposals(wallet, accounts)

    # OUT OFF LIMITS: too low id
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.find_proposals([-1])

    # OUT OFF LIMITS: too big id (proposal with this id does not exist)
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.find_proposals([6])


@pytest.mark.parametrize(
    'start_limit_order_by_order_type_status', [
        (10, 100, 29, 0, 0),  # 0 OK
        ('sample_string', 100, 29, 0, 0),  # 1 OK
        (True, 100, 29, 0, 0),  # 2 OK
        ([""], 'sample_string', 29, 0, 0),  # 3 OK
        ([""], True, 29, 0, 0),  # 4 BUG
        ([""], 100, 'sample_string', 0, 0),  # 5 OK
        ([""], 100, True, 0, 0),  # 6 WRONG EXCEPTION
        ([""], 100, 29, 'sample_string', 0),  # 7 WRONG EXCEPTION
        ([""], 100, 29, True, 0),  # 8 BUG
        ([""], 100, 29, 0, 'sample_string'),  # 9 WRONG EXCEPTION
        ([""], 100, 29, 0, True),  # 10 BUG
    ]
)
def test_list_proposals_false_type_of_argument(node, start_limit_order_by_order_type_status):
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposals(start_limit_order_by_order_type_status)


@pytest.mark.parametrize(
    'start_limit_order_by_order_type_status', [
        (10, 100, 33, 0, 0),  # 0 OK
        ('sample_string', 100, 33, 0, 0),  # 1 OK
        (True, 100, 33, 0, 0),  # 2 OK
        ([""], 'sample_string', 33, 0, 0),  # 3 OK
        ([""], True, 33, 0, 0),  # 4 BUG
        ([""], 100, 'sample_string', 0, 0),  # 5 OK
        ([""], 100, True, 0, 0),  # 6 OK
        ([""], 100, 33, 'sample_string', 0),  # 7 WRONG EXCEPTION
        ([""], 100, 33, True, 0),  # 8 BUG
        ([""], 100, 33, 0, 'sample_string'),  # 9 WRONG EXCEPTION
        ([""], 100, 33, 0, True),  # 10 BUG
    ]
)
def test_list_proposal_votes_false_type_of_argument(node, start_limit_order_by_order_type_status):
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes(start_limit_order_by_order_type_status)


@pytest.mark.parametrize(
    'id', [
        'wrong_string_id',  # 0 OK
        True,  # 1 BUG
    ]
)
def test_find_proposals_false_type_of_argument(node, id):
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.find_proposals([id])


def create_accounts_with_vests_and_TBD(wallet, number_of_accounts):
    accounts = get_accounts_name(wallet.create_accounts(number_of_accounts, 'account'))
    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.transfer_to_vesting('initminer', account, Asset.Test(10000))

    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.transfer('initminer', account, Asset.Tbd(10000), 'memo')

    return accounts


def prepare_proposals(wallet, accounts):
    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.post_comment(account, 'permlink', '', 'parent-permlonk', 'title', 'body', '{}')

    with wallet.in_single_transaction():
        wallet.api.create_proposal(accounts[1], accounts[0], get_future_data(10), get_future_data(12),
                                   Asset.Tbd(100), 'subject-0', 'permlink')
        wallet.api.create_proposal(accounts[2], accounts[1], get_future_data(55), get_future_data(59),
                                   Asset.Tbd(200), 'subject-1', 'permlink')
        wallet.api.create_proposal(accounts[3], accounts[2], get_future_data(100), get_future_data(130),
                                   Asset.Tbd(500), 'subject-2', 'permlink')
        wallet.api.create_proposal(accounts[4], accounts[3], get_future_data(12), get_future_data(15),
                                   Asset.Tbd(300), 'subject-3', 'permlink')
        wallet.api.create_proposal(accounts[0], accounts[4], get_future_data(7), get_future_data(70),
                                   Asset.Tbd(800), 'subject-4', 'permlink')


def get_accounts_name(accounts):
    return [account.name for account in accounts]


def get_future_data(weeks_in_future):
    future_data = datetime.now() + timedelta(weeks=weeks_in_future)
    return future_data.strftime('%Y-%m-%dT%H:%M:%S')
