#include <hive/chain/index_file.hpp>
#include <hive/chain/file_operation.hpp>

#include <sys/types.h>
#include <fcntl.h>

#include <fstream>

namespace hive { namespace chain {

  base_index::base_index( const storage_description::storage_type val, const std::string& file_name_ext_val )
              : storage( val, file_name_ext_val )
  {
  }

  base_index::~base_index()
  {
    close();
  }

  void base_index::open( const fc::path& file )
  {
    storage.open( file );
  }

  void base_index::open()
  {
    storage.open();
  }
 
  void base_index::prepare( const boost::shared_ptr<signed_block>& head_block, const storage_description& desc )
  {
    if( storage.size )
    {
      ilog( "Index is nonempty" );
      // read the last 8 bytes of the block log to get the offset of the beginning of the head 
      // block
      uint64_t block_pos = 0;
      auto bytes_read = file_operation::pread_with_retry(desc.file_descriptor, &block_pos, sizeof(block_pos), 
        desc.size - sizeof(block_pos));

      FC_ASSERT(bytes_read == sizeof(block_pos));

      read_blocks_number( block_pos );

      if( storage.status == storage_description::status_type::reopen )
      {
        ilog( "block_pos < index_pos, close and reopen index_stream" );
        ddump((block_pos)(storage.pos));
      }
      else if( storage.status == storage_description::status_type::resume )
      {
        ilog( "Index is incomplete" );
      }
    }
    else
    {
      ilog( "Index is empty" );
    }
  }

  void base_index::close()
  {
    storage.close();
  }

  void base_index::non_empty_idx_info()
  {
    ilog( "Index is nonempty, remove and recreate it" );
    if (ftruncate( storage.file_descriptor, 0 ))
      FC_THROW("Error truncating block log: ${error}", ("error", strerror(errno)));
  }

  void base_index::append( const void* buf, size_t nbyte )
  {
    file_operation::write_with_retry( storage.file_descriptor, buf, nbyte );
  }

  void base_index::read( uint32_t block_num, uint64_t& offset, uint64_t& size )
  {
    uint64_t offsets[2] = {0, 0};
    uint64_t offset_in_index = sizeof(uint64_t) * (block_num - 1);
    auto bytes_read = file_operation::pread_with_retry( storage.file_descriptor, &offsets, sizeof(offsets),  offset_in_index );
    FC_ASSERT(bytes_read == sizeof(offsets));

    offset = offsets[0];
    size = offsets[1] - offsets[0] - sizeof(uint64_t);
  }

  vector<signed_block> base_index::read_block_range( uint32_t first_block_num, uint32_t count, int block_log_fd, const boost::shared_ptr<signed_block>& head_block )
  {

    vector<signed_block> result;

    uint32_t last_block_num = first_block_num + count - 1;

    if (!head_block || first_block_num > head_block->block_num())
      return result; // the caller is asking for blocks after the head block, we don't have them

    // if that head block will be our last block, we want it at the end of our vector,
    // so we'll tack it on at the bottom of this function
    bool last_block_is_head_block = last_block_num == head_block->block_num();
    uint32_t last_block_num_from_disk = last_block_is_head_block ? last_block_num - 1 : last_block_num;

    if (first_block_num <= last_block_num_from_disk)
    {
      // then we need to read blocks from the disk
      uint32_t number_of_blocks_to_read = last_block_num_from_disk - first_block_num + 1;
      uint32_t number_of_offsets_to_read = number_of_blocks_to_read + 1;
      // read all the offsets in one go
      std::unique_ptr<uint64_t[]> offsets(new uint64_t[number_of_blocks_to_read + 1]);
      uint64_t offset_of_first_offset = sizeof(uint64_t) * (first_block_num - 1);
      file_operation::pread_with_retry( storage.file_descriptor, offsets.get(), sizeof(uint64_t) * number_of_offsets_to_read,  offset_of_first_offset);

      // then read all the blocks in one go
      uint64_t size_of_all_blocks = offsets[number_of_blocks_to_read] - offsets[0];
      idump((size_of_all_blocks));
      std::unique_ptr<char[]> block_data(new char[size_of_all_blocks]);
      file_operation::pread_with_retry(storage.file_descriptor, block_data.get(), size_of_all_blocks,  offsets[0]);

      // now deserialize the blocks
      for (uint32_t i = 0; i <= last_block_num_from_disk - first_block_num; ++i)
      {
        uint64_t offset_in_memory = offsets[i] - offsets[0];
        uint64_t size = offsets[i + 1] - offsets[i] - sizeof(uint64_t);
        signed_block block;
        fc::raw::unpack_from_char_array(block_data.get() + offset_in_memory, size, block);
        result.push_back(std::move(block));
      }
    }

    if (last_block_is_head_block)
      result.push_back(*head_block);

    return result;
  }

  block_log_index::block_log_index( const storage_description::storage_type val, const std::string& file_name_ext_val )
                  : base_index( val, file_name_ext_val )
  {
  }

  block_log_index::~block_log_index()
  {
  }

  void block_log_index::check_consistency( uint32_t total_size )
  {
    storage.check_consistency( sizeof(uint64_t), total_size );
  }

  void block_log_index::read_blocks_number( uint64_t block_pos )
  {
    // read the last 8 bytes of the block index to get the offset of the beginning of the 
    // head block
    size_t bytes_read = file_operation::pread_with_retry( storage.file_descriptor, &storage.pos, sizeof(storage.pos),
      storage.size - sizeof(storage.pos));

    FC_ASSERT(bytes_read == sizeof(storage.pos));

    storage.calculate_status( block_pos );
  }

  void block_log_index::write( std::fstream& stream, const signed_block& block, uint64_t position )
  {
    stream.write( (char*)&position, sizeof( position ) );
  }

  template<uint32_t ELEMENT_SIZE_VALUE>
  custom_index<ELEMENT_SIZE_VALUE>::custom_index( const storage_description::storage_type val, const std::string& file_name_ext_val )
                  : base_index( val, file_name_ext_val )
  {
  }

  template<uint32_t ELEMENT_SIZE_VALUE>
  custom_index<ELEMENT_SIZE_VALUE>::~custom_index()
  {
  }

  template<uint32_t ELEMENT_SIZE_VALUE>
  void custom_index<ELEMENT_SIZE_VALUE>::check_consistency( uint32_t total_size )
  {
    storage.check_consistency( ELEMENT_SIZE_VALUE, total_size );
  }

  template<uint32_t ELEMENT_SIZE_VALUE>
  void custom_index<ELEMENT_SIZE_VALUE>::read_blocks_number( uint64_t block_pos )
  {
    // read the last 8 bytes of the block index to get the offset of the beginning of the 
    // head block
    size_t bytes_read = file_operation::pread_with_retry( storage.file_descriptor, &storage.pos, sizeof(storage.pos),
      storage.size - ELEMENT_SIZE_VALUE);

    FC_ASSERT(bytes_read == sizeof(storage.pos));

    storage.calculate_status( block_pos );
  }

  block_id_witness_public_key::block_id_witness_public_key( const storage_description::storage_type val, const std::string& file_name_ext_val )
                                :custom_index( val, file_name_ext_val )
  {
  }
  
  block_id_witness_public_key::~block_id_witness_public_key()
  {
  }

  void block_id_witness_public_key::write( std::fstream& stream, const signed_block& block, uint64_t position )
  {
    //fc::ripemd160
    auto _id = block.id();
    char* _block_id = _id.data();

    //fc::array<char,33>
    auto _witness_public_key = block.signee();
    fc::ecc::public_key_data _key = _witness_public_key.serialize();

    FC_ASSERT( sizeof( position ) + _id.data_size() + _key.size() == ELEMENT_SIZE, "sizes are incorrect - calculated: ${sum} given: ${given}",
                                                                                  ("sum", sizeof( position ) + _id.data_size() + _key.size())("given", ELEMENT_SIZE) );

    stream.write( (char*)&position, sizeof( position ) );
    stream.write( _block_id, _id.data_size() );
    stream.write( _key.begin(), _key.size() );
  }
} } // hive::chain
