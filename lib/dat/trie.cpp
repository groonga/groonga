#include "trie.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

namespace grn {
namespace dat {

Trie::Trie()
    : header_(NULL),
      nodes_(NULL),
      blocks_(NULL),
      key_infos_(NULL),
      key_buf_(NULL) {}

Trie::~Trie() {
  close();
}

void Trie::create(const char *file_name,
                  UInt64 file_size,
                  UInt32 max_num_keys,
                  double num_nodes_per_key,
                  double average_key_length) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (file_size != 0) && (max_num_keys != 0));

  if (num_nodes_per_key == 0.0) {
    num_nodes_per_key = DEFAULT_NUM_NODES_PER_KEY;
  }
  GRN_DAT_THROW_IF(PARAM_ERROR, num_nodes_per_key < 1.0);

  if (average_key_length == 0.0) {
    average_key_length = DEFAULT_AVERAGE_KEY_LENGTH;
  }
  GRN_DAT_THROW_IF(PARAM_ERROR, average_key_length < 1.0);
  GRN_DAT_THROW_IF(PARAM_ERROR, average_key_length > MAX_KEY_LENGTH);

  if (max_num_keys == 0) {
    if (file_size == 0) {
      file_size = DEFAULT_FILE_SIZE;
    }
  } else {
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_keys > MAX_NUM_KEYS);
  }

  Trie new_trie;
  new_trie.create_file(file_name, file_size, max_num_keys,
                       num_nodes_per_key, average_key_length);
  new_trie.swap(this);
}

void Trie::create(const Trie &trie,
                  const char *file_name,
                  UInt64 file_size,
                  UInt32 max_num_keys,
                  double num_nodes_per_key,
                  double average_key_length) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (file_size != 0) && (max_num_keys != 0));

  if (num_nodes_per_key == 0.0) {
    if (trie.num_keys() == 0) {
      num_nodes_per_key = DEFAULT_NUM_NODES_PER_KEY;
    } else {
      num_nodes_per_key = 1.0 * trie.num_nodes() / trie.num_keys();
    }
  }
  GRN_DAT_THROW_IF(PARAM_ERROR, num_nodes_per_key < 1.0);

  if (average_key_length == 0.0) {
    if (trie.num_keys() == 0) {
      average_key_length = DEFAULT_AVERAGE_KEY_LENGTH;
    } else {
      average_key_length = 1.0 * trie.total_key_length() / trie.num_keys();
    }
  }
  GRN_DAT_THROW_IF(PARAM_ERROR, average_key_length < 1.0);
  GRN_DAT_THROW_IF(PARAM_ERROR, average_key_length > MAX_KEY_LENGTH);

  if (max_num_keys == 0) {
    if (file_size == 0) {
      file_size = trie.file_size();
    }
    GRN_DAT_THROW_IF(PARAM_ERROR, file_size < trie.virtual_size());
  } else {
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_keys < trie.num_keys());
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_keys > MAX_KEY_ID);
  }

  Trie new_trie;
  new_trie.create_file(file_name, file_size, max_num_keys,
                       num_nodes_per_key, average_key_length);
  if (trie.num_keys() != 0) {
    new_trie.build_from_trie(trie);
  }
  new_trie.swap(this);
}

void Trie::open(const char *file_name) {
  GRN_DAT_THROW_IF(PARAM_ERROR, file_name == NULL);

  Trie new_trie;
  new_trie.open_file(file_name);
  new_trie.swap(this);
}

void Trie::close() {
  if (header_ != NULL) {
    ::munmap(header_, header_->file_size());
  }
  header_ = NULL;
  nodes_ = NULL;
  blocks_ = NULL;
  key_infos_ = NULL;
  key_buf_ = NULL;
}

void Trie::swap(Trie *trie) {
  std::swap(header_, trie->header_);
  std::swap(nodes_, trie->nodes_);
  std::swap(blocks_, trie->blocks_);
  std::swap(key_infos_, trie->key_infos_);
  std::swap(key_buf_, trie->key_buf_);
}

void Trie::create_file(const char *file_name,
                       UInt64 file_size,
                       UInt32 max_num_keys,
                       double num_nodes_per_key,
                       double average_key_length) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (file_size == 0) && (max_num_keys == 0));
  GRN_DAT_THROW_IF(PARAM_ERROR, (file_size != 0) && (max_num_keys != 0));

  UInt32 max_num_blocks;
  UInt32 key_buf_size;
  if (max_num_keys == 0) {
    const UInt64 avail = file_size - sizeof(Header);
    const double num_bytes_per_key = (sizeof(Node) * num_nodes_per_key)
        + (1.0 * sizeof(Block) / BLOCK_SIZE * num_nodes_per_key)
        + sizeof(KeyInfo) + average_key_length;
    GRN_DAT_THROW_IF(PARAM_ERROR, (avail / num_bytes_per_key) > MAX_NUM_KEYS);
    max_num_keys = (UInt32)(avail / num_bytes_per_key);
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_keys == 0);
  }

  {
    const double max_num_nodes = num_nodes_per_key * max_num_keys;
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_nodes > MAX_NUM_NODES);
    max_num_blocks = ((UInt32)max_num_nodes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_blocks == 0);
    GRN_DAT_THROW_IF(PARAM_ERROR, max_num_blocks > MAX_NUM_BLOCKS);
  }

  if (file_size == 0) {
    const double total_key_length = average_key_length * max_num_keys;
    GRN_DAT_THROW_IF(PARAM_ERROR, total_key_length > MAX_KEY_BUF_SIZE);
    key_buf_size = (UInt32)total_key_length;

    file_size = sizeof(Header)
        + (sizeof(Block) * max_num_blocks)
        + (sizeof(Node) * BLOCK_SIZE * max_num_blocks)
        + (sizeof(KeyInfo) * (max_num_keys + 1))
        + key_buf_size;
  } else {
    const UInt64 avail = file_size - sizeof(Header)
        - (sizeof(Block) * max_num_blocks)
        - (sizeof(Node) * BLOCK_SIZE * max_num_blocks)
        - (sizeof(KeyInfo) * (max_num_keys + 1));
    GRN_DAT_THROW_IF(PARAM_ERROR, avail == 0);
    GRN_DAT_THROW_IF(PARAM_ERROR, avail > MAX_KEY_BUF_SIZE);
    key_buf_size = (UInt32)avail;
  }

  create_file(file_name, file_size, max_num_keys,
              max_num_blocks, key_buf_size);
}

void Trie::create_file(const char *file_name,
                       UInt64 file_size,
                       UInt32 max_num_keys,
                       UInt32 max_num_blocks,
                       UInt32 key_buf_size) {
  GRN_DAT_THROW_IF(PARAM_ERROR, file_size != (sizeof(Header)
      + (sizeof(Block) * max_num_blocks)
      + (sizeof(Node) * BLOCK_SIZE * max_num_blocks)
      + (sizeof(KeyInfo) * (max_num_keys + 1))
      + key_buf_size));

  int fd = -1;
  if ((file_name != NULL) && (file_name[0] != '\0')) {
    fd = ::open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
    GRN_DAT_THROW_IF(IO_ERROR, fd == -1);
    if (::ftruncate(fd, file_size) == -1) {
      ::close(fd);
      GRN_DAT_THROW(IO_ERROR, "::ftruncate(fd, file_size) == -1");
    }
  }

  const int flags = (fd == -1) ? (MAP_PRIVATE | MAP_ANONYMOUS) : MAP_SHARED;
  void * const address = ::mmap(NULL, file_size, PROT_READ | PROT_WRITE,
                                flags, fd, 0);
  if (address == MAP_FAILED) {
    if (fd != -1) {
      ::close(fd);
    }
    GRN_DAT_THROW(IO_ERROR, "address == MAP_FAILED");
  } else if ((fd != -1) && (::close(fd) == -1)) {
    ::munmap(address, file_size);
    GRN_DAT_THROW(IO_ERROR, "(fd != -1) && (::close(fd) == -1)");
  }

  Header * const header = static_cast<Header *>(address);
  *header = Header();
  header->set_file_size(file_size);
  header->set_max_num_keys(max_num_keys);
  header->set_max_num_blocks(max_num_blocks);
  header->set_key_buf_size(key_buf_size);

  map_address(address);

  reserve_node(ROOT_NODE_ID);
  ith_node(INVALID_OFFSET).set_is_offset(true);
  ith_key_info(MIN_KEY_ID).set_offset(0);
}

void Trie::open_file(const char *file_name) {
  GRN_DAT_THROW_IF(PARAM_ERROR, file_name == NULL);

  const int fd = ::open(file_name, O_RDWR);
  GRN_DAT_THROW_IF(IO_ERROR, fd == -1);

  void *address = ::mmap(NULL, sizeof(Header), PROT_READ, MAP_PRIVATE, fd, 0);
  if (address == MAP_FAILED) {
    ::close(fd);
    GRN_DAT_THROW(IO_ERROR, "address == MAP_FAILED");
  }

  const UInt64 file_size = static_cast<const Header *>(address)->file_size();
  if (::munmap(address, sizeof(Header)) == -1) {
    ::close(fd);
    GRN_DAT_THROW(IO_ERROR, "::munmap(address, sizeof(Header)) == -1");
  }

  address = ::mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (address == MAP_FAILED) {
    ::close(fd);
    GRN_DAT_THROW(IO_ERROR, "address == MAP_FAILED");
  } else if (::close(fd) == -1) {
    ::munmap(address, file_size);
    GRN_DAT_THROW(IO_ERROR, "::close(fd) == -1");
  }

  map_address(address);
}

void Trie::map_address(void *address) {
  GRN_DAT_THROW_IF(PARAM_ERROR, address == NULL);

  header_ = static_cast<Header *>(address);
  nodes_ = static_cast<Node *>(static_cast<void *>(header_ + 1));
  blocks_ = static_cast<Block *>(static_cast<void *>(
      nodes_ + max_num_nodes()));
  key_infos_ = static_cast<KeyInfo *>(static_cast<void *>(
      blocks_ + max_num_blocks()));
  key_buf_ = static_cast<char *>(static_cast<void *>(
      key_infos_ + (max_num_keys() + 1)));
}

void Trie::build_from_trie(const Trie &trie) {
  GRN_DAT_DEBUG_THROW_IF(trie.num_keys() == 0);

  header_->set_num_keys(trie.num_keys());
  std::memcpy(key_infos_, trie.key_infos_,
              sizeof(KeyInfo) * (num_keys() + 1));
  std::memcpy(key_buf_, trie.key_buf_,
              ith_key_info(max_key_id() + 1).offset());
  build_from_trie(trie, ROOT_NODE_ID, ROOT_NODE_ID);
}

void Trie::build_from_trie(const Trie &trie,
                           UInt32 src,
                           UInt32 dest) {
  ith_node(dest).set_except_is_offset(trie.ith_node(src).except_is_offset());
  if (trie.ith_node(src).is_terminal()) {
    ith_node(dest).set_key_id(trie.ith_node(src).key_id());
    return;
  }

  const UInt32 src_offset = trie.ith_node(src).offset();
  UInt32 dest_offset;
  {
    UInt16 labels[MAX_LABEL + 1];
    UInt32 num_labels = 0;

    UInt32 label = trie.ith_node(src).child();
    GRN_DAT_DEBUG_THROW_IF(label == INVALID_LABEL);
    while (label != INVALID_LABEL) {
      GRN_DAT_DEBUG_THROW_IF(label > MAX_LABEL);
      labels[num_labels++] = label;
      label = trie.ith_node(src_offset ^ label).sibling();
    }
    GRN_DAT_DEBUG_THROW_IF(num_labels == 0);

    dest_offset = find_offset(labels, num_labels);
    for (UInt32 i = 0; i < num_labels; ++i) {
      reserve_node(dest_offset ^ labels[i]);
    }

    GRN_DAT_DEBUG_THROW_IF(ith_node(dest_offset).is_offset());
    ith_node(dest_offset).set_is_offset(true);
    ith_node(dest).set_offset(dest_offset);
  }

  UInt32 label = trie.ith_node(src).child();
  while (label != INVALID_LABEL) {
    build_from_trie(trie, src_offset ^ label, dest_offset ^ label);
    label = trie.ith_node(src_offset ^ label).sibling();
  }
}

bool Trie::search_from_root(const UInt8 *ptr,
                            UInt32 length,
                            Key *key) const {
  GRN_DAT_DEBUG_THROW_IF((ptr == NULL) && (length != 0));

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < length; ++i) {
    const Base base = ith_node(node_id).base();
    if (base.is_terminal()) {
      return search_from_terminal(ptr, length, key, base.key_id(), i);
    }

    node_id = base.offset() ^ ptr[i];
    if (ith_node(node_id).label() != ptr[i]) {
      return false;
    }
  }

  const Base base = ith_node(node_id).base();
  if (base.is_terminal()) {
    return search_from_terminal(ptr, length, key, base.key_id(), length);
  }

  node_id = base.offset() ^ TERMINAL_LABEL;
  if (ith_node(node_id).label() != TERMINAL_LABEL) {
    return false;
  }
  GRN_DAT_DEBUG_THROW_IF(!ith_node(node_id).is_terminal());

  if (key != NULL) {
    const UInt32 key_id = ith_node(node_id).key_id();
    key->set_ptr(key_buf_ + ith_key_info(key_id).offset());
    key->set_length(ith_key_info(key_id + 1).offset()
        - ith_key_info(key_id).offset());
    key->set_id(key_id);
  }
  return true;
}

bool Trie::search_from_terminal(const UInt8 *ptr,
                                UInt32 length,
                                Key *key,
                                UInt32 key_id,
                                UInt32 i) const {
  GRN_DAT_DEBUG_THROW_IF(i > length);
  GRN_DAT_DEBUG_THROW_IF(key_id < min_key_id());
  GRN_DAT_DEBUG_THROW_IF(key_id > max_key_id());

  const UInt32 key_length = ith_key_info(key_id + 1).offset()
      - ith_key_info(key_id).offset();
  if (key_length != length) {
    return false;
  }

  const char * const key_ptr = key_buf_ + ith_key_info(key_id).offset();
  for ( ; i < length; ++i) {
    if (ptr[i] != (UInt8)key_ptr[i]) {
      return false;
    }
  }

  if (key != NULL) {
    key->set_ptr(key_ptr);
    key->set_length(key_length);
    key->set_id(key_id);
  }
  return true;
}

bool Trie::insert_from_root(const UInt8 *ptr,
                            UInt32 length,
                            Key *key) {
  GRN_DAT_DEBUG_THROW_IF((ptr == NULL) && (length != 0));

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < length; ++i) {
    const Base base = ith_node(node_id).base();
    if (base.is_terminal()) {
      return insert_from_terminal(ptr, length, key, node_id, i);
    }

    const UInt32 next = base.offset() ^ ptr[i];
    if (ith_node(next).label() != ptr[i]) {
      return insert_from_nonterminal(ptr, length, key, node_id, i);
    }
    node_id = next;
  }

  const Base base = ith_node(node_id).base();
  if (base.is_terminal()) {
    return insert_from_terminal(ptr, length, key, node_id, length);
  }

  const UInt32 next = base.offset() ^ TERMINAL_LABEL;
  if (ith_node(next).label() != TERMINAL_LABEL) {
    return insert_from_nonterminal(ptr, length, key, node_id, length);
  }
  GRN_DAT_DEBUG_THROW_IF(!ith_node(next).is_terminal());

  if (key != NULL) {
    const UInt32 key_id = ith_node(next).key_id();
    key->set_ptr(key_buf_ + ith_key_info(key_id).offset());
    key->set_length(ith_key_info(key_id + 1).offset()
        - ith_key_info(key_id).offset());
    key->set_id(key_id);
  }
  return false;
}

bool Trie::insert_from_terminal(const UInt8 *ptr,
                                UInt32 length,
                                Key *key,
                                UInt32 node_id,
                                UInt32 i) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(!ith_node(node_id).is_terminal());
  GRN_DAT_DEBUG_THROW_IF(i > length);

  UInt32 key_id = ith_node(node_id).key_id();
  const char *key_ptr = key_buf_ + ith_key_info(key_id).offset();
  UInt32 key_length = ith_key_info(key_id + 1).offset()
      - ith_key_info(key_id).offset();

  UInt32 j = i;
  while ((j < length) && (j < key_length)) {
    if (ptr[j] != (UInt8)key_ptr[j]) {
      break;
    }
    ++j;
  }

  if ((j == length) && (j == key_length)) {
    if (key != NULL) {
      key->set_ptr(key_ptr);
      key->set_length(key_length);
      key->set_id(key_id);
    }
    return false;
  }

  GRN_DAT_THROW_IF(SIZE_ERROR, num_keys() >= max_num_keys());
  key_id = max_key_id() + 1;

  const UInt32 key_offset = ith_key_info(key_id).offset();
  GRN_DAT_THROW_IF(SIZE_ERROR, length > (key_buf_size() - key_offset));

  for (UInt32 k = i; k < j; ++k) {
    node_id = insert_node(node_id, ptr[k]);
  }
  node_id = separate(ptr, length, node_id, j);

  ith_key_info(key_id + 1).set_offset(key_offset + length);
  std::memcpy(key_buf_ + key_offset, ptr, length);
  header_->set_num_keys(num_keys() + 1);

  ith_node(node_id).set_key_id(key_id);

  if (key != NULL) {
    key->set_ptr(key_buf_ + key_offset);
    key->set_length(length);
    key->set_id(key_id);
  }
  return true;
}

bool Trie::insert_from_nonterminal(const UInt8 *ptr,
                                   UInt32 length,
                                   Key *key,
                                   UInt32 node_id,
                                   UInt32 i) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(ith_node(node_id).is_terminal());
  GRN_DAT_DEBUG_THROW_IF(i > length);

  GRN_DAT_THROW_IF(SIZE_ERROR, num_keys() >= max_num_keys());
  const UInt32 key_id = max_key_id() + 1;

  const UInt32 key_offset = ith_key_info(key_id).offset();
  GRN_DAT_THROW_IF(SIZE_ERROR, length > (key_buf_size() - key_offset));

  const UInt16 label = (i < length) ? (UInt16)ptr[i] : (UInt16)TERMINAL_LABEL;
  const Base base = ith_node(node_id).base();
  if ((base.offset() == INVALID_OFFSET) ||
      !ith_node(base.offset() ^ label).is_phantom()) {
    resolve(node_id, label);
  }
  node_id = insert_node(node_id, label);

  ith_key_info(key_id + 1).set_offset(key_offset + length);
  std::memcpy(key_buf_ + key_offset, ptr, length);
  header_->set_num_keys(num_keys() + 1);

  ith_node(node_id).set_key_id(key_id);

  if (key != NULL) {
    key->set_ptr(key_buf_ + key_offset);
    key->set_length(length);
    key->set_id(key_id);
  }
  return true;
}

UInt32 Trie::insert_node(UInt32 node_id,
                         UInt16 label) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(label > MAX_LABEL);

  const Base base = ith_node(node_id).base();
  UInt32 offset;
  if (base.is_terminal() || (base.offset() == INVALID_OFFSET)) {
    offset = find_offset(&label, 1);
  } else {
    offset = base.offset();
  }

  const UInt32 next = offset ^ label;
  reserve_node(next);

  ith_node(next).set_label(label);
  if (base.is_terminal()) {
    GRN_DAT_DEBUG_THROW_IF(ith_node(offset).is_offset());
    ith_node(offset).set_is_offset(true);
    ith_node(next).set_key_id(base.key_id());
  } else if (base.offset() == INVALID_OFFSET) {
    GRN_DAT_DEBUG_THROW_IF(ith_node(offset).is_offset());
    ith_node(offset).set_is_offset(true);
  } else {
    GRN_DAT_DEBUG_THROW_IF(!ith_node(offset).is_offset());
  }
  ith_node(node_id).set_offset(offset);

  const UInt32 child_label = ith_node(node_id).child();
  GRN_DAT_DEBUG_THROW_IF(child_label == label);
  if (child_label == INVALID_LABEL) {
    ith_node(node_id).set_child(label);
  } else if ((label == TERMINAL_LABEL) ||
             ((child_label != TERMINAL_LABEL) && (label < child_label))) {
    GRN_DAT_DEBUG_THROW_IF(ith_node(offset ^ child_label).is_phantom());
    GRN_DAT_DEBUG_THROW_IF(ith_node(offset ^ child_label).label() != child_label);
    ith_node(next).set_sibling(child_label);
    ith_node(node_id).set_child(label);
  } else {
    UInt32 prev = offset ^ child_label;
    GRN_DAT_DEBUG_THROW_IF(ith_node(prev).label() != child_label);
    UInt32 sibling_label = ith_node(prev).sibling();
    while (label > sibling_label) {
      prev = offset ^ sibling_label;
      GRN_DAT_DEBUG_THROW_IF(ith_node(prev).label() != sibling_label);
      sibling_label = ith_node(prev).sibling();
    }
    GRN_DAT_DEBUG_THROW_IF(label == sibling_label);
    ith_node(next).set_sibling(ith_node(prev).sibling());
    ith_node(prev).set_sibling(label);
  }
  return next;
}

UInt32 Trie::separate(const UInt8 *ptr,
                      UInt32 length,
                      UInt32 node_id,
                      UInt32 i) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(!ith_node(node_id).is_terminal());
  GRN_DAT_DEBUG_THROW_IF(i > length);

  const UInt32 key_id = ith_node(node_id).key_id();
  const UInt32 key_offset = ith_key_info(key_id).offset();
  const char *key_ptr = key_buf_ + key_offset;
  const UInt32 key_length = ith_key_info(key_id + 1).offset() - key_offset;

  UInt16 labels[2];
  labels[0] = (i < key_length) ?
      (UInt16)(UInt8)key_ptr[i] : (UInt16)TERMINAL_LABEL;
  labels[1] = (i < length) ? (UInt16)ptr[i] : (UInt16)TERMINAL_LABEL;
  GRN_DAT_DEBUG_THROW_IF(labels[0] == labels[1]);

  const UInt32 offset = find_offset(labels, 2);
  GRN_DAT_DEBUG_THROW_IF(ith_node(offset).is_offset());

  UInt32 next = offset ^ labels[0];
  reserve_node(next);

  ith_node(next).set_label(labels[0]);
  ith_node(next).set_key_id(key_id);

  next = offset ^ labels[1];
  reserve_node(next);

  ith_node(next).set_label(labels[1]);

  ith_node(offset).set_is_offset(true);
  ith_node(node_id).set_offset(offset);

  if ((labels[0] == TERMINAL_LABEL) ||
      ((labels[1] != TERMINAL_LABEL) && (labels[0] < labels[1]))) {
    ith_node(node_id).set_child(labels[0]);
    ith_node(offset ^ labels[0]).set_sibling(labels[1]);
  } else {
    ith_node(node_id).set_child(labels[1]);
    ith_node(offset ^ labels[1]).set_sibling(labels[0]);
  }
  return next;
}

void Trie::resolve(UInt32 node_id,
                   UInt16 label) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(ith_node(node_id).is_terminal());
  GRN_DAT_DEBUG_THROW_IF(label > MAX_LABEL);

  UInt32 offset = ith_node(node_id).offset();
  if (offset != INVALID_OFFSET) {
    UInt16 labels[MAX_LABEL + 1];
    UInt32 num_labels = 0;

    UInt32 next_label = ith_node(node_id).child();
    GRN_DAT_DEBUG_THROW_IF(next_label == INVALID_LABEL);
    while (next_label != INVALID_LABEL) {
      GRN_DAT_DEBUG_THROW_IF(next_label > MAX_LABEL);
      labels[num_labels++] = next_label;
      next_label = ith_node(offset ^ next_label).sibling();
    }
    GRN_DAT_DEBUG_THROW_IF(num_labels == 0);

    labels[num_labels] = label;
    offset = find_offset(labels, num_labels + 1);
    move_nodes(node_id, offset, labels, num_labels);
  } else {
    offset = find_offset(&label, 1);
    if (offset >= num_nodes()) {
      GRN_DAT_DEBUG_THROW_IF((offset / BLOCK_SIZE) != num_blocks());
      reserve_block(num_blocks());
    }
    ith_node(offset).set_is_offset(true);
    ith_node(node_id).set_offset(offset);
  }
}

void Trie::move_nodes(UInt32 node_id,
                      UInt32 dest_offset,
                      const UInt16 *labels,
                      UInt32 num_labels) {
  GRN_DAT_DEBUG_THROW_IF(node_id >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(ith_node(node_id).is_terminal());
  GRN_DAT_DEBUG_THROW_IF(labels == NULL);
  GRN_DAT_DEBUG_THROW_IF(num_labels == 0);
  GRN_DAT_DEBUG_THROW_IF(num_labels > (MAX_LABEL + 1));

  const UInt32 src_offset = ith_node(node_id).offset();
  GRN_DAT_DEBUG_THROW_IF(src_offset == INVALID_OFFSET);
  GRN_DAT_DEBUG_THROW_IF(!ith_node(src_offset).is_offset());

  for (UInt32 i = 0; i < num_labels; ++i) {
    const UInt32 src_node_id = src_offset ^ labels[i];
    const UInt32 dest_node_id = dest_offset ^ labels[i];
    GRN_DAT_DEBUG_THROW_IF(ith_node(src_node_id).is_phantom());
    GRN_DAT_DEBUG_THROW_IF(ith_node(src_node_id).label() != labels[i]);

    reserve_node(dest_node_id);
    ith_node(dest_node_id).set_except_is_offset(
        ith_node(src_node_id).except_is_offset());
    ith_node(dest_node_id).set_base(ith_node(src_node_id).base());
  }
  header_->set_num_zombies(num_zombies() + num_labels);

  GRN_DAT_DEBUG_THROW_IF(ith_node(dest_offset).is_offset());
  ith_node(dest_offset).set_is_offset(true);
  ith_node(node_id).set_offset(dest_offset);
}

UInt32 Trie::find_offset(const UInt16 *labels,
                         UInt32 num_labels) {
  GRN_DAT_DEBUG_THROW_IF(labels == NULL);
  GRN_DAT_DEBUG_THROW_IF(num_labels == 0);
  GRN_DAT_DEBUG_THROW_IF(num_labels > (MAX_LABEL + 1));

  // Blocks are tested in descending order of level, a lower level block
  // contains more phantom nodes.
  UInt32 level = 1;
  while (num_labels >= (1U << level)) {
    ++level;
  }
  level = (level < MAX_BLOCK_LEVEL) ? (MAX_BLOCK_LEVEL - level) : 0;

  UInt32 block_count = 0;
  do {
    UInt32 entry = header_->ith_entry(level);
    if (entry == INVALID_ENTRY) {
      // This level has no blocks and it is thus skipped.
      continue;
    }

    UInt32 block_id = entry;
    do {
      const Block &block = ith_block(block_id);
      GRN_DAT_DEBUG_THROW_IF(block.level() != level);

      const UInt32 first = (block_id * BLOCK_SIZE) | block.first_phantom();
      UInt32 node_id = first;
      do {
        GRN_DAT_DEBUG_THROW_IF(!ith_node(node_id).is_phantom());
        const UInt32 offset = node_id ^ labels[0];
        if (!ith_node(offset).is_offset()) {
          UInt32 i = 1;
          for ( ; i < num_labels; ++i) {
            if (!ith_node(offset ^ labels[i]).is_phantom()) {
              break;
            }
          }
          if (i >= num_labels) {
            return offset;
          }
        }
        node_id = (block_id * BLOCK_SIZE) | ith_node(node_id).next();
      } while (node_id != first);

      const UInt32 prev = block_id;
      const UInt32 next = block.next();
      block_id = next;
      ith_block(prev).set_fail_count(ith_block(prev).fail_count() + 1);

      // The level of a block is updated when this function fails many times,
      // actually MAX_FAIL_COUNT times, in that block.
      if (ith_block(prev).fail_count() == MAX_FAIL_COUNT) {
        update_block_level(prev, level + 1);
        if (next == entry) {
          break;
        } else {
          // Note that the entry might be updated in the level update.
          entry = header_->ith_entry(level);
          continue;
        }
      }
    } while ((++block_count < MAX_BLOCK_COUNT) && (block_id != entry));
  } while ((block_count < MAX_BLOCK_COUNT) && (level-- != 0));

  return num_nodes() ^ labels[0];
}

void Trie::reserve_node(UInt32 node_id) {
  GRN_DAT_DEBUG_THROW_IF(node_id > num_nodes());
  if (node_id >= num_nodes()) {
    reserve_block(node_id / BLOCK_SIZE);
  }

  Node &node = ith_node(node_id);
  GRN_DAT_DEBUG_THROW_IF(!node.is_phantom());

  const UInt32 block_id = node_id / BLOCK_SIZE;
  Block &block = ith_block(block_id);
  GRN_DAT_DEBUG_THROW_IF(block.num_phantoms() == 0);

  const UInt32 next = (block_id * BLOCK_SIZE) | node.next();
  const UInt32 prev = (block_id * BLOCK_SIZE) | node.prev();
  GRN_DAT_DEBUG_THROW_IF(next >= num_nodes());
  GRN_DAT_DEBUG_THROW_IF(prev >= num_nodes());

  if ((node_id & BLOCK_MASK) == block.first_phantom()) {
    // The first phantom node is removed from the block and the second phantom
    // node comes first.
    block.set_first_phantom(next & BLOCK_MASK);
  }

  ith_node(next).set_prev(prev & BLOCK_MASK);
  ith_node(prev).set_next(next & BLOCK_MASK);

  if (block.level() != MAX_BLOCK_LEVEL) {
    const UInt32 threshold = 1U << ((MAX_BLOCK_LEVEL - block.level() - 1) * 2);
    if (block.num_phantoms() == threshold) {
      update_block_level(block_id, block.level() + 1);
    }
  }
  block.set_num_phantoms(block.num_phantoms() - 1);

  node.set_is_phantom(false);

  GRN_DAT_DEBUG_THROW_IF(node.offset() != INVALID_OFFSET);
  GRN_DAT_DEBUG_THROW_IF(node.label() != INVALID_LABEL);

  header_->set_num_phantoms(num_phantoms() - 1);
}

void Trie::reserve_block(UInt32 block_id) {
  GRN_DAT_DEBUG_THROW_IF(block_id != num_blocks());
  GRN_DAT_THROW_IF(SIZE_ERROR, block_id >= max_num_blocks());

  header_->set_num_blocks(block_id + 1);
  ith_block(block_id).set_fail_count(0);
  ith_block(block_id).set_first_phantom(0);
  ith_block(block_id).set_num_phantoms(BLOCK_SIZE);

  const UInt32 begin = block_id * BLOCK_SIZE;
  const UInt32 end = begin + BLOCK_SIZE;
  GRN_DAT_DEBUG_THROW_IF(end != num_nodes());

  Base base;
  base.set_offset(INVALID_OFFSET);

  Check check;
  check.set_is_phantom(true);

  for (UInt32 i = begin; i < end; ++i) {
    check.set_prev((i - 1) & BLOCK_MASK);
    check.set_next((i + 1) & BLOCK_MASK);
    ith_node(i).set_base(base);
    ith_node(i).set_check(check);
  }

  // The leve of the new block is 0.
  set_block_level(block_id, 0);
  header_->set_num_phantoms(num_phantoms() + BLOCK_SIZE);
}

void Trie::update_block_level(UInt32 block_id,
                              UInt32 level) {
  GRN_DAT_DEBUG_THROW_IF(block_id >= num_blocks());
  GRN_DAT_DEBUG_THROW_IF(level > MAX_BLOCK_LEVEL);

  clear_block_level(block_id);
  set_block_level(block_id, level);
}

void Trie::set_block_level(UInt32 block_id,
                           UInt32 level) {
  GRN_DAT_DEBUG_THROW_IF(block_id >= num_blocks());
  GRN_DAT_DEBUG_THROW_IF(level > MAX_BLOCK_LEVEL);

  const UInt32 entry = header_->ith_entry(level);
  if (entry == INVALID_ENTRY) {
    // The new block becomes the only one block of the linked list.
    ith_block(block_id).set_next(block_id);
    ith_block(block_id).set_prev(block_id);
    header_->set_ith_entry(level, block_id);
  } else {
    // The new block is added to the end of the list.
    const UInt32 next = entry;
    const UInt32 prev = ith_block(entry).prev();
    GRN_DAT_DEBUG_THROW_IF(next >= num_blocks());
    GRN_DAT_DEBUG_THROW_IF(prev >= num_blocks());
    ith_block(block_id).set_next(next);
    ith_block(block_id).set_prev(prev);
    ith_block(next).set_prev(block_id);
    ith_block(prev).set_next(block_id);
  }
  ith_block(block_id).set_level(level);
  ith_block(block_id).set_fail_count(0);
}

void Trie::clear_block_level(UInt32 block_id) {
  GRN_DAT_DEBUG_THROW_IF(block_id >= num_blocks());

  const UInt32 level = ith_block(block_id).level();
  GRN_DAT_DEBUG_THROW_IF(level > MAX_BLOCK_LEVEL);

  const UInt32 entry = header_->ith_entry(level);
  GRN_DAT_DEBUG_THROW_IF(entry == INVALID_ENTRY);

  const UInt32 next = ith_block(block_id).next();
  const UInt32 prev = ith_block(block_id).prev();
  GRN_DAT_DEBUG_THROW_IF(next >= num_blocks());
  GRN_DAT_DEBUG_THROW_IF(prev >= num_blocks());

  if (next == block_id) {
    // The linked list becomes empty.
    header_->set_ith_entry(level, INVALID_ENTRY);
  } else {
    ith_block(next).set_prev(prev);
    ith_block(prev).set_next(next);
    if (block_id == entry) {
      // The second block becomes the entry to the linked list.
      header_->set_ith_entry(level, next);
    }
  }
}

}  // namespace dat
}  // namespace grn
