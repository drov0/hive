import time

import pytest

from test_tools import Account, Asset, logger, exceptions, Wallet, World


def test_get_active_witnesses(world: World):
    network = world.create_network()
    init_node = network.create_init_node()

    witness_names = [f'witness-{i}' for i in range(20)]
    for name in witness_names:
        witness = Account(name)
        init_node.config.witness.append(witness.name)
        init_node.config.private_key.append(witness.private_key)

    init_node.run()
    wallet = Wallet(attach_to=init_node)

    with wallet.in_single_transaction():
        for name in witness_names:
            wallet.api.create_account('initminer', name, '')

    with wallet.in_single_transaction():
        for name in witness_names:
            wallet.api.transfer_to_vesting("initminer", name, Asset.Test(1000))

    with wallet.in_single_transaction():
        for name in witness_names:
            wallet.api.update_witness(
                name, "https://" + name,
                Account(name).public_key,
                {"account_creation_fee": Asset.Test(3), "maximum_block_size": 65536, "sbd_interest_rate": 0}
            )
    init_node.wait_number_of_blocks(22)
    get_active_witnesses = init_node.api.wallet_bridge.get_active_witnesses()

    for name in witness_names:
        assert name in str(get_active_witnesses['witnesses'])

def test_get_witness(node):
    get_witness = node.api.wallet_bridge.get_witness('initminer')
    pass

def test_get_witness_schedule():
    # bez argumentów
    pass

def test_list_witnesses():#w innym pliku
    pass

