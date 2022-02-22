import time

from deepdiff import DeepDiff
import pytest

import test_tools.exceptions
from test_tools import Account, Asset, logger, exceptions, Wallet, World

# dupa = wallet.api.list_proposals([""], 100, "by_creator", "ascending", "all", 5)

def test_list_proposals_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)

    order_type = {
        'by_creator': 29,
        'by_start_date': 30,
        'by_end_date': 31,
    }

    order_direction = {
        'ascending': 0,
        'descending': 1,
    }

    status = {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }

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


def test_list_proposals_out_of_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)
    order_type = {
        'by_creator': 29,
        'by_start_date': 30,
        'by_end_date': 31,
    }

    order_direction = {
        'ascending': 0,
        'descending': 1,
    }

    status = {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }

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


def test_list_proposal_votes_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)
    with wallet.in_single_transaction():
        wallet.api.update_proposal_votes(accounts[0], [3], 1)
        wallet.api.update_proposal_votes(accounts[1], [3], 1)
        wallet.api.update_proposal_votes(accounts[2], [3], 1)
        wallet.api.update_proposal_votes(accounts[3], [3], 1)
        wallet.api.update_proposal_votes(accounts[4], [3], 1)

    order_type = {
        'by_voter_proposal': 33,
        'by_proposal_voter': 34,
    }

    order_direction = {
        'ascending': 0,
        'descending': 1,
    }

    status = {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }

    # ON LIMITS: max and min
    node.api.wallet_bridge.list_proposal_votes([""], 0, order_type['by_voter_proposal'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 1000, order_type['by_voter_proposal'], order_direction['ascending'],
                                          status['all'])

    # ON LIMITS: order_type
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_proposal_voter'], order_direction['ascending'],
                                          status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['descending'],
                                          status['all'])

    # ON LIMITS: order_direction
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'],
                                          status['all'])
    node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['descending'],
                                          status['votable'])

def test_list_proposal_votes_out_off_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)
    with wallet.in_single_transaction():
        wallet.api.update_proposal_votes(accounts[0], [3], 1)
        wallet.api.update_proposal_votes(accounts[1], [3], 1)
        wallet.api.update_proposal_votes(accounts[2], [3], 1)
        wallet.api.update_proposal_votes(accounts[3], [3], 1)
        wallet.api.update_proposal_votes(accounts[4], [3], 1)

    order_type = {
        'by_voter_proposal': 33,
        'by_proposal_voter': 34,
    }

    order_direction = {
        'ascending': 0,
        'descending': 1,
    }

    status = {
        'all': 0,
        'inactive': 1,
        'active': 2,
        'expired': 3,
        'votable': 4,
    }

    # OUT OFF LIMITS: max and min
    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], -1, order_type['by_voter_proposal'], order_direction['ascending'],
                                              status['all'])

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 1001, order_type['by_voter_proposal'], order_direction['ascending'],
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
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['ascending'], -1)

    with pytest.raises(test_tools.exceptions.CommunicationError):
        node.api.wallet_bridge.list_proposal_votes([""], 100, order_type['by_voter_proposal'], order_direction['descending'], 6)


def test_find_proposals_on_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)

    node.api.wallet_bridge.find_proposals([0, 1, 2, 3, 4, 5])

def test_find_proposals_out_off_limited_values(node):
    wallet = Wallet(attach_to=node)
    accounts = get_powered_accounts(wallet)
    preprare_5_proposals(wallet, accounts)

    #PROBLEM
    a = node.api.wallet_bridge.find_proposals([-1])
    b = node.api.wallet_bridge.find_proposals([6])

    pass

def get_powered_accounts(wallet):
    accounts = get_accounts_name(wallet.create_accounts(5, 'account'))
    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.transfer_to_vesting('initminer', account, Asset.Test(100))

    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.post_comment(account, 'hello-world', '', 'xyz', 'something about world', 'just nothing', '{}')

    with wallet.in_single_transaction():
        for account in accounts:
            wallet.api.transfer('initminer', account, Asset.Tbd(788.543), 'avocado')
    return accounts

def preprare_5_proposals(wallet, accounts):
    with wallet.in_single_transaction():
        wallet.api.create_proposal(accounts[1], accounts[0], '2031-01-01T00:00:00', '2031-06-01T00:00:00',
                                   Asset.Tbd(111), 'this is proposal1', 'hello-world')
        wallet.api.create_proposal(accounts[2], accounts[1], '2031-01-01T00:00:00', '2031-06-01T00:00:00',
                                   Asset.Tbd(121), 'this is proposal2', 'hello-world')
        wallet.api.create_proposal(accounts[3], accounts[2], '2031-01-01T00:00:00', '2031-06-01T00:00:00',
                                   Asset.Tbd(141), 'this is proposal3', 'hello-world')
        wallet.api.create_proposal(accounts[4], accounts[3], '2031-01-01T00:00:00', '2031-06-01T00:00:00',
                                   Asset.Tbd(511), 'this is proposal4', 'hello-world')
        wallet.api.create_proposal(accounts[0], accounts[4], '2031-01-01T00:00:00', '2031-06-01T00:00:00',
                                   Asset.Tbd(11), 'this is proposal5', 'hello-world')

def get_accounts_name(accounts):
    accounts_names = []
    for account_number in range(len(accounts)):
        accounts_names.append(accounts[account_number].name)
    return accounts_names
