#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <hive/chain/account_object.hpp>
#include <hive/chain/comment_object.hpp>
#include <hive/chain/database_exceptions.hpp>
#include <hive/chain/smt_objects.hpp>
#include <hive/protocol/hive_operations.hpp>

#include <hive/plugins/rc/rc_objects.hpp>
#include <hive/plugins/rc/rc_config.hpp>
#include <hive/plugins/rc/rc_operations.hpp>
#include <hive/plugins/rc/rc_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace hive::chain;
using namespace hive::protocol;
using namespace hive::plugins::rc;

BOOST_FIXTURE_TEST_SUITE( rc_direct_delegation, clean_database_fixture )

BOOST_AUTO_TEST_CASE( delegate_rc_operation_validate )
{
  try{
    delegate_rc_operation op;
    op.from = "alice";
    op.delegatees = {"eve", "bob"};
    op.max_rc = 10;
    op.validate();

    // alice- is an invalid accoutn name
    op.from = "alice-";
    BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

    // Bob- is an invalid account name
    op.from = "alice";
    op.delegatees = {"eve", "bob-"};
    BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

    // Alice is already the delegator
    op.delegatees = {"alice", "bob"};
    BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

    // Empty vector
    op.delegatees = {};
    BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

    // bob is defined twice, handled by the flat_set
    op.delegatees = {"eve", "bob", "bob"};
    op.validate();

    // There is more than HIVE_RC_MAX_ACCOUNTS_PER_DELEGATION_OP accounts in delegatees
    flat_set<account_name_type> set_too_big;
    for (int i = 0; i < HIVE_RC_MAX_ACCOUNTS_PER_DELEGATION_OP + 1; i++) {
      set_too_big.insert( "actor" + std::to_string(i) );
    }
    op.delegatees = set_too_big;
    BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_rc_operation_apply_single )
{
  try
  {
    BOOST_TEST_MESSAGE( "Testing:  delegate_rc_operation_apply_single to a single account" );
    ACTORS( (alice)(bob)(dave) )
    vest( HIVE_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );
    int64_t alice_vests = alice.vesting_shares.amount.value;

    // Delegating more rc than I have should fail
    delegate_rc_operation op;
    op.from = "alice";
    op.delegatees = {"bob"};
    op.max_rc = alice_vests + 1;

    custom_json_operation custom_op;
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.id = HIVE_RC_PLUGIN_NAME;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Delegating to a non-existing account should fail
    op.delegatees = {"eve"};
    op.max_rc = 10;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Delegating 0 shouldn't work if there isn't already a delegation that exists (since 0 deletes the delegation)
    op.delegatees = {"bob"};
    op.max_rc = 0;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Successful delegation
    op.delegatees = {"bob"};
    op.max_rc = 10;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation->from == alice_id );
    BOOST_REQUIRE( delegation->to == bob_id );
    BOOST_REQUIRE( delegation->delegated_rc == op.max_rc );

    const rc_account_object& from_rc_account = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& to_rc_account = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( from_rc_account.delegated_rc == 10 );
    BOOST_REQUIRE( from_rc_account.received_delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account.delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account.received_delegated_rc == 10 );


    // Delegating the same amount shouldn't work
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );
    
    // Decrease the delegation
    op.from = "alice";
    op.delegatees = {"bob"};
    op.max_rc = 5;
    custom_op.required_posting_auths.clear();
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation_decreased = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation_decreased->from == alice_id );
    BOOST_REQUIRE( delegation_decreased->to == bob_id );
    BOOST_REQUIRE( delegation_decreased->delegated_rc == op.max_rc );

    const rc_account_object& from_rc_account_decreased = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& to_rc_account_decreased = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( from_rc_account_decreased.delegated_rc == 5 );
    BOOST_REQUIRE( from_rc_account_decreased.received_delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_decreased.delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_decreased.received_delegated_rc == 5 );

    // Increase the delegation
    op.from = "alice";
    op.delegatees = {"bob"};
    op.max_rc = 50;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation_increased = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation_increased->from == alice_id );
    BOOST_REQUIRE( delegation_increased->to == bob_id );
    BOOST_REQUIRE( delegation_increased->delegated_rc == op.max_rc );

    const rc_account_object& from_rc_account_increased = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& to_rc_account_increased = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( from_rc_account_increased.delegated_rc == 50 );
    BOOST_REQUIRE( from_rc_account_increased.received_delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_increased.delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_increased.received_delegated_rc == 50 );

    // Delete the delegation
    op.from = "alice";
    op.delegatees = {"bob"};
    op.max_rc = 0;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation_deleted == nullptr );

    const rc_account_object& from_rc_account_deleted = db->get< rc_account_object, by_name >( "alice" );
    const rc_account_object& to_rc_account_deleted = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( from_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( from_rc_account_deleted.received_delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account_deleted.received_delegated_rc == 0 );
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_rc_operation_apply_many )
{
  try
  {
    BOOST_TEST_MESSAGE( "Testing:  delegate_rc_operation_apply_many to many accounts" );
    ACTORS( (alice)(bob)(dave)(dan) )
    vest( HIVE_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );
    int64_t alice_vests = alice.vesting_shares.amount.value;

    // Delegating more rc than alice has should fail
    delegate_rc_operation op;
    op.from = "alice";
    op.delegatees = {"bob", "dave"};
    op.max_rc = alice_vests;

    custom_json_operation custom_op;
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.id = HIVE_RC_PLUGIN_NAME;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Delegating to a non-existing account should fail
    op.delegatees = {"bob", "eve"};
    op.max_rc = 10;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Delegating 0 shouldn't work if there isn't already a delegation that exists (since 0 deletes the delegation)
    op.delegatees = {"dave", "bob"};
    op.max_rc = 0;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Successful delegations
    op.delegatees = {"bob", "dave"};
    op.max_rc = 10;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation->from == alice_id );
    BOOST_REQUIRE( delegation->to == bob_id );
    BOOST_REQUIRE( delegation->delegated_rc == op.max_rc );

    const rc_account_object& from_rc_account = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& to_rc_account = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( from_rc_account.delegated_rc == 20 );
    BOOST_REQUIRE( from_rc_account.received_delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account.delegated_rc == 0 );
    BOOST_REQUIRE( to_rc_account.received_delegated_rc == 10 );

    // Delegating the same amount shouldn't work
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );


    // Delegating 0 shouldn't work if there isn't already a delegation that exists (since 0 deletes the delegation)
    // dave/bob got a delegation but not dan so it should fail
    op.delegatees = {"dave", "bob", "dan"};
    op.max_rc = 0;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    BOOST_CHECK_THROW( push_transaction(custom_op, alice_private_key);, fc::exception );

    // Decrease the delegations
    op.from = "alice";
    op.delegatees = {"bob", "dave"};
    op.max_rc = 5;
    custom_op.required_posting_auths.clear();
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);
    generate_block();

    const rc_direct_delegation_object* delegation_decreased_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation_decreased_bob->from == alice_id );
    BOOST_REQUIRE( delegation_decreased_bob->to == bob_id );
    BOOST_REQUIRE( delegation_decreased_bob->delegated_rc == op.max_rc );

    const rc_direct_delegation_object* delegation_decreased_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
    BOOST_REQUIRE( delegation_decreased_dave->from == alice_id );
    BOOST_REQUIRE( delegation_decreased_dave->to == dave_id );
    BOOST_REQUIRE( delegation_decreased_dave->delegated_rc == op.max_rc );

    const rc_account_object& from_rc_account_decreased = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& bob_rc_account_decreased = db->get< rc_account_object, by_name >( "bob" );
    const rc_account_object& dave_rc_account_decreased = db->get< rc_account_object, by_name >( "dave" );

    BOOST_REQUIRE( from_rc_account_decreased.delegated_rc == 10 );
    BOOST_REQUIRE( from_rc_account_decreased.received_delegated_rc == 0 );
    BOOST_REQUIRE( bob_rc_account_decreased.delegated_rc == 0 );
    BOOST_REQUIRE( bob_rc_account_decreased.received_delegated_rc == 5 );
    BOOST_REQUIRE( dave_rc_account_decreased.delegated_rc == 0 );
    BOOST_REQUIRE( dave_rc_account_decreased.received_delegated_rc == 5 );

    // Increase and create a delegation
    op.from = "alice";
    op.delegatees = {"bob", "dan"};
    op.max_rc = 50;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* bob_delegation_increased = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    const rc_direct_delegation_object* dan_delegation_created = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dan_id ) );

    BOOST_REQUIRE( bob_delegation_increased->from == alice_id );
    BOOST_REQUIRE( bob_delegation_increased->to == bob_id );
    BOOST_REQUIRE( bob_delegation_increased->delegated_rc == op.max_rc );
    BOOST_REQUIRE( dan_delegation_created->from == alice_id );
    BOOST_REQUIRE( dan_delegation_created->to == dan_id );
    BOOST_REQUIRE( dan_delegation_created->delegated_rc == op.max_rc );

    const rc_account_object& alice_rc_account_increased = db->get< rc_account_object, by_name >( op.from );
    const rc_account_object& dan_rc_account_created = db->get< rc_account_object, by_name >( "dan" );
    const rc_account_object& bob_rc_account_increased = db->get< rc_account_object, by_name >( "bob" );

    BOOST_REQUIRE( alice_rc_account_increased.delegated_rc == 105 );
    BOOST_REQUIRE( alice_rc_account_increased.received_delegated_rc == 0 );

    BOOST_REQUIRE( dan_rc_account_created.delegated_rc == 0 );
    BOOST_REQUIRE( dan_rc_account_created.received_delegated_rc == 50 );
    BOOST_REQUIRE( bob_rc_account_increased.delegated_rc == 0 );
    BOOST_REQUIRE( bob_rc_account_increased.received_delegated_rc == 50 );

    // Delete the delegations
    op.from = "alice";
    op.delegatees = {"bob", "dan", "dave"};
    op.max_rc = 0;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_direct_delegation_object* delegation_bob_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    const rc_direct_delegation_object* delegation_dan_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dan_id ) );
    const rc_direct_delegation_object* delegation_dave_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dan_id ) );
    BOOST_REQUIRE( delegation_bob_deleted == nullptr );
    BOOST_REQUIRE( delegation_dan_deleted == nullptr );
    BOOST_REQUIRE( delegation_dave_deleted == nullptr );

    const rc_account_object& from_rc_account_deleted = db->get< rc_account_object, by_name >( "alice" );
    const rc_account_object& bob_rc_account_deleted = db->get< rc_account_object, by_name >( "bob" );
    const rc_account_object& dave_rc_account_deleted = db->get< rc_account_object, by_name >( "dave" );
    const rc_account_object& dan_rc_account_deleted = db->get< rc_account_object, by_name >( "dan" );

    BOOST_REQUIRE( from_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( from_rc_account_deleted.received_delegated_rc == 0 );
    BOOST_REQUIRE( bob_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( bob_rc_account_deleted.received_delegated_rc == 0 );
    BOOST_REQUIRE( dave_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( dave_rc_account_deleted.received_delegated_rc == 0 );
    BOOST_REQUIRE( dan_rc_account_deleted.delegated_rc == 0 );
    BOOST_REQUIRE( dan_rc_account_deleted.received_delegated_rc == 0 );
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_outdel_overflow )
{
  try
  {
    BOOST_TEST_MESSAGE( "Testing:  update_outdel_overflow" );
    ACTORS( (alice)(bob)(dave)(eve)(martin) )
    generate_block();

    // We are forced to do this because vests and rc values are bugged when executing tests
    db_plugin->debug_update( [=]( database& db )
    {
      db.modify( db.get_account( "alice" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 90, VESTS_SYMBOL );
      });

      db.modify( db.get_account( "bob" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });

      db.modify( db.get_account( "dave" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "alice" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 100;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.last_max_rc = 100;
      });

      db.modify( db.get< rc_account_object, by_name >( "bob" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
      db.modify( db.get< rc_account_object, by_name >( "dave" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
    });


    // Delegate 10 rc to bob, 80 to dave, alice has 10 remaining rc
    delegate_rc_operation op;
    op.from = "alice";
    op.delegatees = {"bob"};
    op.max_rc = 10;
    custom_json_operation custom_op;
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.id = HIVE_RC_PLUGIN_NAME;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    op.delegatees = {"dave"};
    op.max_rc = 80;
    custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
    push_transaction(custom_op, alice_private_key);

    generate_block();

    const rc_account_object& bob_rc_account_before = db->get< rc_account_object, by_name >("bob");
    const rc_account_object& dave_rc_account_before = db->get< rc_account_object, by_name >("dave");
    const rc_account_object& alice_rc_before = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_before.delegated_rc == 90 );
    BOOST_REQUIRE( alice_rc_before.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_before.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_before.last_max_rc == 10 );

    BOOST_REQUIRE( bob_rc_account_before.rc_manabar.current_mana == 20 );
    BOOST_REQUIRE( bob_rc_account_before.last_max_rc == 20 );
    BOOST_REQUIRE( bob_rc_account_before.received_delegated_rc == 10 );

    BOOST_REQUIRE( dave_rc_account_before.rc_manabar.current_mana == 90 );
    BOOST_REQUIRE( dave_rc_account_before.last_max_rc == 90 );
    BOOST_REQUIRE( dave_rc_account_before.received_delegated_rc == 80 );

    // we delegate and it affects one delegation
    // Delegate 5 vests out, alice has 5 remaining rc, it's lower than the max_rc_creation_adjustment which is 10
    // So the first delegation (to bob) is lowered to 5
    delegate_vesting_shares_operation dvso;
    dvso.vesting_shares = ASSET( "0.000005 VESTS");
    dvso.delegator = "alice";
    dvso.delegatee = {"eve"};
    push_transaction(dvso, alice_private_key);

    const rc_account_object& bob_rc_account_after = db->get< rc_account_object, by_name >("bob");
    const rc_account_object& dave_rc_account_after = db->get< rc_account_object, by_name >("dave");
    const rc_account_object& alice_rc_after = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_after.delegated_rc == 85 );
    BOOST_REQUIRE( alice_rc_after.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_after.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_after.last_max_rc == 10 );

    BOOST_REQUIRE( bob_rc_account_after.rc_manabar.current_mana == 15 );
    BOOST_REQUIRE( bob_rc_account_after.last_max_rc == 15 );
    BOOST_REQUIRE( bob_rc_account_after.received_delegated_rc == 5 );

    BOOST_REQUIRE( dave_rc_account_after.rc_manabar.current_mana == 90 );
    BOOST_REQUIRE( dave_rc_account_after.last_max_rc == 90 );
    BOOST_REQUIRE( dave_rc_account_after.received_delegated_rc == 80 );
    
    // We delegate and we don't have enough rc to sustain bob's delegation
    dvso.vesting_shares = ASSET( "0.000045 VESTS");
    dvso.delegator = "alice";
    dvso.delegatee = "martin";
    push_transaction(dvso, alice_private_key);

    const rc_account_object& bob_rc_account_after_two = db->get< rc_account_object, by_name >("bob");
    const rc_account_object& dave_rc_account_after_two = db->get< rc_account_object, by_name >("dave");
    const rc_account_object& alice_rc_after_two = db->get< rc_account_object, by_name >( "alice" );

    const rc_direct_delegation_object* delegation_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
    BOOST_REQUIRE( delegation_deleted == nullptr );
   
    BOOST_REQUIRE( bob_rc_account_after_two.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( bob_rc_account_after_two.last_max_rc == 10 );
    BOOST_REQUIRE( bob_rc_account_after_two.received_delegated_rc == 0 );

    BOOST_REQUIRE( alice_rc_after_two.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_after_two.delegated_rc == 40 );
    BOOST_REQUIRE( alice_rc_after_two.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_after_two.last_max_rc == 10 );

    BOOST_REQUIRE( dave_rc_account_after_two.rc_manabar.current_mana == 50 );
    BOOST_REQUIRE( dave_rc_account_after_two.last_max_rc == 50 );
    BOOST_REQUIRE( dave_rc_account_after_two.received_delegated_rc == 40 );
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_outdel_overflow_many_accounts )
{
  try
  {
    BOOST_TEST_MESSAGE( "Testing:  update_outdel_overflow with many actors" );
    #define NUM_ACTORS 250
    #define CREATE_ACTORS(z, n, text) ACTORS( (actor ## n) );
    BOOST_PP_REPEAT(NUM_ACTORS, CREATE_ACTORS, )
    ACTORS( (alice)(bob) )
    generate_block();

    // We are forced to do this because vests and rc values are bugged when executing tests
    db_plugin->debug_update( [=]( database& db )
    {
      db.modify( db.get_account( "alice" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( NUM_ACTORS * 10, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "alice" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = NUM_ACTORS * 10 + 10; // vests + max_rc_creation_adjustment
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.last_max_rc = NUM_ACTORS * 10 + 10; // vests + max_rc_creation_adjustment
      });


      // Set the values for every actor
      for (int i = 0; i < NUM_ACTORS; i++) {
        db.modify( db.get_account( "actor" + std::to_string(i) ), [&]( account_object& a )
        {
          a.vesting_shares = asset( 0, VESTS_SYMBOL );
        });

        db.modify( db.get< rc_account_object, by_name >( "actor" + std::to_string(i) ), [&]( rc_account_object& rca )
        {
          rca.max_rc_creation_adjustment.amount.value = 10;
          rca.rc_manabar.current_mana = 10;
          rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
          rca.max_rc_creation_adjustment.amount.value = 10;
          rca.last_max_rc = 10;
        });
      }
    });

    int count = 0;

    delegate_rc_operation op;
    op.from = "alice";
    op.max_rc = 10;
    // Delegate 10 rc to every actor account
    for (int i = 0; i < NUM_ACTORS; i++) {
      op.delegatees.insert( "actor" + std::to_string(i) );
      if (count == 50 || i == NUM_ACTORS -1 ) {
        custom_json_operation custom_op;
        custom_op.required_posting_auths.insert( "alice" );
        custom_op.id = HIVE_RC_PLUGIN_NAME;
        custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
        push_transaction(custom_op, alice_private_key);
        generate_block();
        op.delegatees = {};
        count = 0;
      }
      count++;
    }

    const rc_account_object& actor0_rc_account_before = db->get< rc_account_object, by_name >("actor0");
    const rc_account_object& actor2_rc_account_before = db->get< rc_account_object, by_name >("actor2");
    const rc_account_object& alice_rc_before = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_before.delegated_rc == NUM_ACTORS * 10 );
    BOOST_REQUIRE( alice_rc_before.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_before.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_before.last_max_rc == 10 );

    BOOST_REQUIRE( actor0_rc_account_before.rc_manabar.current_mana == 20 );
    BOOST_REQUIRE( actor0_rc_account_before.last_max_rc == 20 );
    BOOST_REQUIRE( actor0_rc_account_before.received_delegated_rc == 10 );

    BOOST_REQUIRE( actor2_rc_account_before.rc_manabar.current_mana == 20 );
    BOOST_REQUIRE( actor2_rc_account_before.last_max_rc == 20 );
    BOOST_REQUIRE( actor2_rc_account_before.received_delegated_rc == 10 );

    // we delegate 25 vests and it affects the first three delegations
    // Delegate 25 vests out, alice has -15 remaining rc, it's lower than the max_rc_creation_adjustment which is 10
    // So the first two delegations (to actor0 and actor2) are deleted while the delegation to actor2 is deleted
    delegate_vesting_shares_operation dvso;
    dvso.vesting_shares = ASSET( "0.000025 VESTS");
    dvso.delegator = "alice";
    dvso.delegatee = "bob";
    push_transaction(dvso, alice_private_key);

    const rc_account_object& actor0_rc_account_after = db->get< rc_account_object, by_name >("actor0");
    const rc_account_object& actor2_rc_account_after = db->get< rc_account_object, by_name >("actor2");
    const rc_account_object& alice_rc_after = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_after.delegated_rc == NUM_ACTORS * 10 - 25 ); // total amount minus what was delegated
    BOOST_REQUIRE( alice_rc_after.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_after.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_after.last_max_rc == 10 );

    BOOST_REQUIRE( actor0_rc_account_after.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( actor0_rc_account_after.last_max_rc == 10 );
    BOOST_REQUIRE( actor0_rc_account_after.received_delegated_rc == 0 );

    BOOST_REQUIRE( actor2_rc_account_after.rc_manabar.current_mana == 15 );
    BOOST_REQUIRE( actor2_rc_account_after.last_max_rc == 15 );
    BOOST_REQUIRE( actor2_rc_account_after.received_delegated_rc == 5 );

    const rc_direct_delegation_object* delegation_actor0_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, actor0_id ) );
    BOOST_REQUIRE( delegation_actor0_deleted == nullptr );
    const rc_direct_delegation_object* delegation_actor1_deleted = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, actor1_id ) );
    BOOST_REQUIRE( delegation_actor1_deleted == nullptr );

    // We check that the rest of the delegations weren't affected
    for (int i = 4; i < NUM_ACTORS; i++) {
      const rc_account_object& actor_rc_account = db->get< rc_account_object, by_name >( "actor" + std::to_string(i) );
      BOOST_REQUIRE( actor_rc_account.rc_manabar.current_mana == 20 );
      BOOST_REQUIRE( actor_rc_account.last_max_rc == 20 );
      BOOST_REQUIRE( actor_rc_account.received_delegated_rc == 10 );
    }

    // We delegate all our vests and we don't have enough to sustain any of our remaining delegations
    const account_object& acct = db->get_account( "alice" );

    dvso.vesting_shares = acct.vesting_shares;
    dvso.delegator = "alice";
    dvso.delegatee = "bob";
    push_transaction(dvso, alice_private_key);

    const rc_account_object& alice_rc_end = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_end.delegated_rc == 0 ); // total amount minus what was delegated
    BOOST_REQUIRE( alice_rc_end.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_end.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_end.last_max_rc == 10 );

    // We check that every delegation got deleted
    for (int i = 0; i < NUM_ACTORS; i++) {
      const rc_account_object& actor_rc_account = db->get< rc_account_object, by_name >( "actor" + std::to_string(i) );
      BOOST_REQUIRE( actor_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( actor_rc_account.last_max_rc == 10 );
      BOOST_REQUIRE( actor_rc_account.received_delegated_rc == 0 );
    }

    // Remove vests delegation and check that we got the rc back
    dvso.vesting_shares = ASSET( "0.000000 VESTS");;
    dvso.delegator = "alice";
    dvso.delegatee = "bob";
    push_transaction(dvso, alice_private_key);

    const rc_account_object& alice_rc_final = db->get< rc_account_object, by_name >( "alice" );

    BOOST_REQUIRE( alice_rc_final.delegated_rc == 0 ); // total amount minus what was delegated
    BOOST_REQUIRE( alice_rc_final.received_delegated_rc == 0 );
    BOOST_REQUIRE( alice_rc_final.rc_manabar.current_mana == 10 );
    BOOST_REQUIRE( alice_rc_final.last_max_rc == 10 );
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( direct_rc_delegation_vesting_withdrawal )
{
  try
  {
    ACTORS( (alice)(bob)(dave) )
    generate_block();

    // We are forced to do this because vests and rc values are bugged when executing tests
    db_plugin->debug_update( [=]( database& db )
    {
      db.modify( db.get_account( "alice" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 100, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "alice" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 110; // vests + max_rc_creation_adjustment
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.last_max_rc = 110; // vests + max_rc_creation_adjustment
      });


      db.modify( db.get_account( "bob" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });
      db.modify( db.get_account( "dave" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "bob" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
      db.modify( db.get< rc_account_object, by_name >( "dave" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
    });

    BOOST_TEST_MESSAGE( "Setting up rc delegations" );

    delegate_rc_operation drc_op;
    drc_op.from = "alice";
    drc_op.delegatees = {"bob", "dave"};
    drc_op.max_rc = 50;
    custom_json_operation custom_op;
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.id = HIVE_RC_PLUGIN_NAME;
    custom_op.json = fc::json::to_string( rc_plugin_operation( drc_op ) );
    push_transaction(custom_op, alice_private_key);
    generate_block();

    BOOST_TEST_MESSAGE( "Setting up withdrawal" );
    const auto& new_alice = db->get_account( "alice" );

    withdraw_vesting_operation op;
    op.account = "alice";
    op.vesting_shares = asset( new_alice.get_vesting().amount, VESTS_SYMBOL );
    push_transaction(op, alice_private_key);

    auto next_withdrawal = db->head_block_time() + HIVE_VESTING_WITHDRAW_INTERVAL_SECONDS;
    asset vesting_shares = new_alice.get_vesting();
    asset withdraw_rate = new_alice.vesting_withdraw_rate;

    BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
    generate_blocks( next_withdrawal - HIVE_BLOCK_INTERVAL );

    BOOST_REQUIRE( get_vesting( "alice" ).amount.value == vesting_shares.amount.value );

    {
      const rc_account_object& alice_rc_account = db->get< rc_account_object, by_name >( "alice" );
      const rc_account_object& bob_rc_account = db->get< rc_account_object, by_name >("bob");
      const rc_account_object& dave_rc_account = db->get< rc_account_object, by_name >("dave");

      BOOST_REQUIRE( alice_rc_account.delegated_rc == uint64_t(100 - withdraw_rate.amount.value) ); // total amount minus what was withdrew
      BOOST_REQUIRE( alice_rc_account.received_delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( alice_rc_account.last_max_rc == 10 );

      // There wasn't enough to sustain the delegation to bob, so it got modified
      BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 60 - withdraw_rate.amount.value );
      BOOST_REQUIRE( bob_rc_account.received_delegated_rc == uint64_t(50 - withdraw_rate.amount.value) );
      BOOST_REQUIRE( bob_rc_account.last_max_rc == 60 - withdraw_rate.amount.value );

      BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 60 );
      BOOST_REQUIRE( dave_rc_account.last_max_rc == 60 );
      BOOST_REQUIRE( dave_rc_account.received_delegated_rc == 50 );

      const rc_direct_delegation_object* delegation_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
      BOOST_REQUIRE( delegation_bob->from == alice_id );
      BOOST_REQUIRE( delegation_bob->to == bob_id );
      BOOST_REQUIRE( delegation_bob->delegated_rc == uint64_t(50 - withdraw_rate.amount.value) );
      const rc_direct_delegation_object* delegation_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
      BOOST_REQUIRE( delegation_dave->from == alice_id );
      BOOST_REQUIRE( delegation_dave->to == dave_id );
      BOOST_REQUIRE( delegation_dave->delegated_rc == 50 );
    }

    BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
    generate_block();

    for( int i = 1; i < HIVE_VESTING_WITHDRAW_INTERVALS - 1; i++ )
    {
      // generate up to just before the withdrawal
      generate_blocks( db->head_block_time() + HIVE_VESTING_WITHDRAW_INTERVAL_SECONDS - HIVE_BLOCK_INTERVAL);

      const rc_account_object& alice_rc_account = db->get< rc_account_object, by_name >( "alice" );
      const rc_account_object& bob_rc_account = db->get< rc_account_object, by_name >("bob");
      const rc_account_object& dave_rc_account = db->get< rc_account_object, by_name >("dave");
      const rc_direct_delegation_object* delegation_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
      const rc_direct_delegation_object* delegation_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
      auto total_vests_withdrew = withdraw_rate.amount.value * (i + 1);

      BOOST_REQUIRE( alice_rc_account.delegated_rc == uint64_t(100 - total_vests_withdrew) ); // total amount minus what was withdrew
      BOOST_REQUIRE( alice_rc_account.received_delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( alice_rc_account.last_max_rc == 10 );

      if (total_vests_withdrew < 50) {
        // Only the first delegation is affected
        BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 60 - total_vests_withdrew);
        BOOST_REQUIRE( bob_rc_account.received_delegated_rc == uint64_t(50 - total_vests_withdrew) );
        BOOST_REQUIRE( bob_rc_account.last_max_rc == 60 - total_vests_withdrew);
        BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 60 );
        BOOST_REQUIRE( dave_rc_account.last_max_rc == 60 );
        BOOST_REQUIRE( dave_rc_account.received_delegated_rc == 50 );

        BOOST_REQUIRE( delegation_bob->from == alice_id );
        BOOST_REQUIRE( delegation_bob->to == bob_id );
        BOOST_REQUIRE( delegation_bob->delegated_rc == uint64_t(50 - total_vests_withdrew));

        BOOST_REQUIRE( delegation_dave->from == alice_id );
        BOOST_REQUIRE( delegation_dave->to == dave_id );
        BOOST_REQUIRE( delegation_dave->delegated_rc == 50 );
      } else {
        // the first delegation is deleted and the second delegation starts to be affected
        BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 10);
        BOOST_REQUIRE( bob_rc_account.received_delegated_rc == 0 );
        BOOST_REQUIRE( bob_rc_account.last_max_rc == 10 );
        BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 60 - (total_vests_withdrew - 50));
        BOOST_REQUIRE( dave_rc_account.received_delegated_rc == uint64_t(50 - (total_vests_withdrew - 50)));
        BOOST_REQUIRE( dave_rc_account.last_max_rc == 60 - (total_vests_withdrew - 50));

        BOOST_REQUIRE( delegation_bob == nullptr );

        BOOST_REQUIRE( delegation_dave->from == alice_id );
        BOOST_REQUIRE( delegation_dave->to == dave_id );
        idump((delegation_dave->delegated_rc));
        idump((uint64_t(100 - total_vests_withdrew)));
        BOOST_REQUIRE( delegation_dave->delegated_rc == uint64_t(100 - total_vests_withdrew) );
      }

      generate_block();
    }

    BOOST_TEST_MESSAGE( "Generating one more block to finish vesting withdrawal" );
    generate_blocks( db->head_block_time() + HIVE_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

    {
      const rc_account_object& alice_rc_account = db->get< rc_account_object, by_name >( "alice" );
      const rc_account_object& bob_rc_account = db->get< rc_account_object, by_name >("bob");
      const rc_account_object& dave_rc_account = db->get< rc_account_object, by_name >("dave");

      BOOST_REQUIRE( alice_rc_account.delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.received_delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( alice_rc_account.last_max_rc == 10 );

      BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( bob_rc_account.last_max_rc == 10 );
      BOOST_REQUIRE( bob_rc_account.received_delegated_rc == 0 );

      BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( dave_rc_account.last_max_rc == 10 );
      BOOST_REQUIRE( dave_rc_account.received_delegated_rc == 0 );

      const rc_direct_delegation_object* delegation_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
      BOOST_REQUIRE( delegation_bob == nullptr );
      const rc_direct_delegation_object* delegation_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
      BOOST_REQUIRE( delegation_dave == nullptr );
    }
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( direct_rc_delegation_vesting_withdrawal_routes )
{
  try
  {
    ACTORS( (alice)(bob)(dave) )
    generate_block();

    // We are forced to do this because vests and rc values are bugged when executing tests
    db_plugin->debug_update( [=]( database& db )
    {
      db.modify( db.get_account( "alice" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 100, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "alice" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 110; // vests + max_rc_creation_adjustment
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.last_max_rc = 110; // vests + max_rc_creation_adjustment
      });


      db.modify( db.get_account( "bob" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });
      db.modify( db.get_account( "dave" ), [&]( account_object& a )
      {
        a.vesting_shares = asset( 0, VESTS_SYMBOL );
      });

      db.modify( db.get< rc_account_object, by_name >( "bob" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
      db.modify( db.get< rc_account_object, by_name >( "dave" ), [&]( rc_account_object& rca )
      {
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.rc_manabar.current_mana = 10;
        rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
        rca.max_rc_creation_adjustment.amount.value = 10;
        rca.last_max_rc = 10;
      });
    });

    BOOST_TEST_MESSAGE( "Setting up vesting routes" );

    {
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "bob";
      op.percent = HIVE_1_PERCENT * 25;
      op.auto_vest = true;
      push_transaction( op, alice_private_key );
    }
    {
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "alice";
      op.percent = HIVE_1_PERCENT * 50;
      op.auto_vest = true;
      push_transaction( op, alice_private_key );
    }
    {
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "dave";
      op.percent = HIVE_1_PERCENT * 25;
      op.auto_vest = false;
      push_transaction( op, alice_private_key );
    }
    BOOST_TEST_MESSAGE( "Setting up rc delegations" );

    delegate_rc_operation drc_op;
    drc_op.from = "alice";
    drc_op.delegatees = {"bob", "dave"};
    drc_op.max_rc = 50;
    custom_json_operation custom_op;
    custom_op.required_posting_auths.insert( "alice" );
    custom_op.id = HIVE_RC_PLUGIN_NAME;
    custom_op.json = fc::json::to_string( rc_plugin_operation( drc_op ) );
    push_transaction(custom_op, alice_private_key);
    generate_block();

    BOOST_TEST_MESSAGE( "Setting up withdrawal" );
    const auto& new_alice = db->get_account( "alice" );

    withdraw_vesting_operation op;
    op.account = "alice";
    op.vesting_shares = asset( new_alice.get_vesting().amount, VESTS_SYMBOL );
    push_transaction(op, alice_private_key);

    auto next_withdrawal = db->head_block_time() + HIVE_VESTING_WITHDRAW_INTERVAL_SECONDS;
    asset vesting_shares = new_alice.get_vesting();
    asset withdraw_rate = new_alice.vesting_withdraw_rate;

    BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
    generate_blocks( next_withdrawal - HIVE_BLOCK_INTERVAL );

    BOOST_REQUIRE( get_vesting( "alice" ).amount.value == vesting_shares.amount.value );

    {
      const rc_account_object& alice_rc_account = db->get< rc_account_object, by_name >( "alice" );
      const rc_account_object& bob_rc_account = db->get< rc_account_object, by_name >("bob");
      const rc_account_object& dave_rc_account = db->get< rc_account_object, by_name >("dave");

      BOOST_REQUIRE( alice_rc_account.delegated_rc == uint64_t(100 - withdraw_rate.amount.value) ); // total amount minus what was withdrew
      BOOST_REQUIRE( alice_rc_account.received_delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.rc_manabar.current_mana == 10 );
      BOOST_REQUIRE( alice_rc_account.last_max_rc == 10 );

      // There wasn't enough to sustain the delegation to bob, so it got modified
      BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 60 - withdraw_rate.amount.value );
      BOOST_REQUIRE( bob_rc_account.received_delegated_rc == uint64_t(50 - withdraw_rate.amount.value) );
      BOOST_REQUIRE( bob_rc_account.last_max_rc == 60 - withdraw_rate.amount.value );

      BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 60 );
      BOOST_REQUIRE( dave_rc_account.last_max_rc == 60 );
      BOOST_REQUIRE( dave_rc_account.received_delegated_rc == 50 );

      const rc_direct_delegation_object* delegation_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
      BOOST_REQUIRE( delegation_bob->from == alice_id );
      BOOST_REQUIRE( delegation_bob->to == bob_id );
      BOOST_REQUIRE( delegation_bob->delegated_rc == uint64_t(50 - withdraw_rate.amount.value) );
      const rc_direct_delegation_object* delegation_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
      BOOST_REQUIRE( delegation_dave->from == alice_id );
      BOOST_REQUIRE( delegation_dave->to == dave_id );
      BOOST_REQUIRE( delegation_dave->delegated_rc == 50 );
    }

    BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
    generate_block();

    {
      const rc_account_object& alice_rc_account = db->get< rc_account_object, by_name >( "alice" );
      const rc_account_object& bob_rc_account = db->get< rc_account_object, by_name >("bob");
      const rc_account_object& dave_rc_account = db->get< rc_account_object, by_name >("dave");

      idump(( alice_rc_account.delegated_rc));
      idump((  alice_rc_account.rc_manabar.current_mana));
      idump((  alice_rc_account.last_max_rc));

      BOOST_REQUIRE( alice_rc_account.delegated_rc == 92 ); // 8 was withdrew
      BOOST_REQUIRE( alice_rc_account.received_delegated_rc == 0 );
      BOOST_REQUIRE( alice_rc_account.rc_manabar.current_mana == 14 ); // 8 is withdrew, but 4 is auto vested back
      BOOST_REQUIRE( alice_rc_account.last_max_rc == 14 );

      // There wasn't enough to sustain the delegation to bob, so it got modified
      BOOST_REQUIRE( bob_rc_account.rc_manabar.current_mana == 54 ); // 60 rc starting (50 delegated, 10 base), - 8 from the vesting withdraw, + 2 rc due to the auto vest
      BOOST_REQUIRE( bob_rc_account.received_delegated_rc == 42);
      BOOST_REQUIRE( bob_rc_account.last_max_rc == 54 );

      BOOST_REQUIRE( dave_rc_account.rc_manabar.current_mana == 60 );
      BOOST_REQUIRE( dave_rc_account.last_max_rc == 60 );
      BOOST_REQUIRE( dave_rc_account.received_delegated_rc == 50 );

      const rc_direct_delegation_object* delegation_bob = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, bob_id ) );
      BOOST_REQUIRE( delegation_bob->from == alice_id );
      BOOST_REQUIRE( delegation_bob->to == bob_id );
      BOOST_REQUIRE( delegation_bob->delegated_rc == 42 );
      const rc_direct_delegation_object* delegation_dave = db->find< rc_direct_delegation_object, by_from_to >( boost::make_tuple( alice_id, dave_id ) );
      BOOST_REQUIRE( delegation_dave->from == alice_id );
      BOOST_REQUIRE( delegation_dave->to == dave_id );
      BOOST_REQUIRE( delegation_dave->delegated_rc == 50 );
    }

  }
  FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

#endif
