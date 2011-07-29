/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2011 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GRN_DAT_TRIE_HPP_
#define GRN_DAT_TRIE_HPP_

#include "header.hpp"
#include "node.hpp"
#include "block.hpp"
#include "key-info.hpp"
#include "key.hpp"
#include "memory-mapped-file.hpp"

namespace grn {
namespace dat {

class Trie {
 public:
  Trie();
  ~Trie();

  void create(const char *file_name = NULL,
              UInt64 file_size = 0,
              UInt32 max_num_keys = 0,
              double num_nodes_per_key = 0.0,
              double average_key_length = 0.0);

  void create(const Trie &src_trie,
              const char *file_name = NULL,
              UInt64 file_size = 0,
              UInt32 max_num_keys = 0,
              double num_nodes_per_key = 0.0,
              double average_key_length = 0.0);

  void open(const char *file_name);
  void close();

  void swap(Trie *trie);

  void ith_key(UInt32 key_id, Key *key) const {
    GRN_DAT_DEBUG_THROW_IF(key_id < min_key_id());
    GRN_DAT_DEBUG_THROW_IF(key_id > max_key_id());
    GRN_DAT_DEBUG_THROW_IF(key == NULL);

    key->set_ptr(key_buf_ + ith_key_info(key_id).offset());
    key->set_length(ith_key_info(key_id + 1).offset()
        - ith_key_info(key_id).offset());
    key->set_id(key_id);
  }

  bool search(const void *ptr, UInt32 length, Key *key = NULL) const {
    GRN_DAT_DEBUG_THROW_IF((ptr == NULL) && (length != 0));
    return search_from_root(static_cast<const UInt8 *>(ptr), length, key);
  }
  bool insert(const void *ptr, UInt32 length, Key *key = NULL) {
    GRN_DAT_DEBUG_THROW_IF((ptr == NULL) && (length != 0));
    return insert_from_root(static_cast<const UInt8 *>(ptr), length, key);
  }

  const Node &ith_node(UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i >= num_nodes());
    return nodes_[i];
  }
  const Block &ith_block(UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i >= num_blocks());
    return blocks_[i];
  }
  const KeyInfo &ith_key_info(UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i < min_key_id());
    GRN_DAT_DEBUG_THROW_IF(i > (max_key_id() + 1));
    return key_infos_[i - MIN_KEY_ID];
  }

  const Header &header() const {
    return *header_;
  }

  UInt64 file_size() const {
    return header_->file_size();
  }
  UInt64 virtual_size() const {
    return sizeof(Header)
        + (sizeof(KeyInfo) * (num_keys() + 1))
        + (sizeof(Block) * num_blocks())
        + (sizeof(Node) * num_nodes())
        + total_key_length();
  }
  UInt32 total_key_length() const {
    return ith_key_info(num_keys() + 1).offset();
  }
  UInt32 num_keys() const {
    return header_->num_keys();
  }
  UInt32 min_key_id() const {
    return header_->min_key_id();
  }
  UInt32 max_key_id() const {
    return header_->max_key_id();
  }
  UInt32 max_num_keys() const {
    return header_->max_num_keys();
  }
  UInt32 num_nodes() const {
    return header_->num_nodes();
  }
  UInt32 num_phantoms() const {
    return header_->num_phantoms();
  }
  UInt32 num_zombies() const {
    return header_->num_zombies();
  }
  UInt32 max_num_nodes() const {
    return header_->max_num_nodes();
  }
  UInt32 num_blocks() const {
    return header_->num_blocks();
  }
  UInt32 max_num_blocks() const {
    return header_->max_num_blocks();
  }
  UInt32 key_buf_size() const {
    return header_->key_buf_size();
  }

 private:
  MemoryMappedFile memory_mapped_file_;
  Header *header_;
  Node *nodes_;
  Block *blocks_;
  KeyInfo *key_infos_;
  char *key_buf_;

  void create_file(const char *file_name,
                   UInt64 file_size,
                   UInt32 max_num_keys,
                   double num_nodes_per_key,
                   double average_key_length);
  void create_file(const char *file_name,
                   UInt64 file_size,
                   UInt32 max_num_keys,
                   UInt32 max_num_blocks,
                   UInt32 key_buf_size);
  void create_root();

  void open_file(const char *file_name);

  void map_address(void *address);

  void build_from_trie(const Trie &trie);
  void build_from_trie(const Trie &trie,
                       UInt32 src,
                       UInt32 dest);

  bool search_from_root(const UInt8 *ptr,
                        UInt32 length,
                        Key *key) const;
  bool search_from_terminal(const UInt8 *ptr,
                            UInt32 length,
                            Key *key,
                            UInt32 key_id,
                            UInt32 i) const;

  bool insert_from_root(const UInt8 *ptr,
                        UInt32 length,
                        Key *key);
  bool insert_from_terminal(const UInt8 *ptr,
                            UInt32 length,
                            Key *key,
                            UInt32 node_id,
                            UInt32 i);
  bool insert_from_nonterminal(const UInt8 *ptr,
                               UInt32 length,
                               Key *key,
                               UInt32 node_id,
                               UInt32 i);

  UInt32 insert_node(UInt32 node_id,
                     UInt16 label);

  UInt32 separate(const UInt8 *ptr,
                  UInt32 length,
                  UInt32 node_id,
                  UInt32 i);
  void resolve(UInt32 node_id,
               UInt16 label);
  void move_nodes(UInt32 node_id,
                  UInt32 dest_offset,
                  const UInt16 *labels,
                  UInt32 num_labels);

  UInt32 find_offset(const UInt16 *labels,
                     UInt32 num_labels);

  void reserve_node(UInt32 node_id);
  void reserve_block(UInt32 block_id);

  void update_block_level(UInt32 block_id,
                          UInt32 level);
  void set_block_level(UInt32 block_id,
                       UInt32 level);
  void clear_block_level(UInt32 block_id);

  Node &ith_node(UInt32 i) {
    GRN_DAT_DEBUG_THROW_IF(i >= num_nodes());
    return nodes_[i];
  }
  Block &ith_block(UInt32 i) {
    GRN_DAT_DEBUG_THROW_IF(i >= num_blocks());
    return blocks_[i];
  }
  KeyInfo &ith_key_info(UInt32 i) {
    GRN_DAT_DEBUG_THROW_IF(i < min_key_id());
    GRN_DAT_DEBUG_THROW_IF(i > (max_key_id() + 2));
    return key_infos_[i - MIN_KEY_ID];
  }

  // Disallows copy and assignment.
  Trie(const Trie &);
  Trie &operator=(const Trie &);
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_TRIE_HPP_
