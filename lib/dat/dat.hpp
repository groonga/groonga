#ifndef GRN_DAT_COMMON_HPP_
#define GRN_DAT_COMMON_HPP_

#ifndef _MSC_VER
 #include <stdint.h>
#endif  // _MSC_VER

#include <cstddef>
#include <exception>

#ifdef _DEBUG
 #include <iostream>
#endif  // _DEBUG

namespace grn {
namespace dat {

#ifdef _MSC_VER
typedef unsigned __int8  UInt8;
typedef unsigned __int16 UInt16;
typedef unsigned __int32 UInt32;
typedef unsigned __int64 UInt64;
#else  // _MSC_VER
typedef ::uint8_t UInt8;
typedef ::uint16_t UInt16;
typedef ::uint32_t UInt32;
typedef ::uint64_t UInt64;
#endif  // _MSC_VER

#ifndef UINT8_MAX
const UInt8 UINT8_MAX = static_cast<UInt8>(0xFFU);
#endif  // UINT8_MAX

#ifndef UINT8_MAX
const UInt16 UINT16_MAX = static_cast<UInt16>(0xFFFFU);
#endif  // UINT16_MAX

#ifndef UINT8_MAX
const UInt32 UINT32_MAX = static_cast<UInt32>(0xFFFFFFFFU);
#endif  // UINT32_MAX

#ifndef UINT8_MAX
const UInt64 UINT64_MAX = static_cast<UInt64>(0xFFFFFFFFFFFFFFFFULL);
#endif  // UINT64_MAX

// The special label attached to a terminal node.
const UInt16 TERMINAL_LABEL = 0x100;
const UInt32 LABEL_MASK = 0x1FF;
const UInt32 BLOCK_MASK = 0x1FF;

const UInt32 BLOCK_SIZE = 0x200;

const UInt32 ROOT_NODE_ID = 0;
const UInt32 KEY_ID_OFFSET = 1;

const UInt16 MAX_LABEL       = TERMINAL_LABEL;
const UInt32 MAX_NODE_ID     = 0x7FFFFFFF;
const UInt32 MAX_BLOCK_ID    = MAX_NODE_ID / BLOCK_SIZE;
const UInt32 MAX_OFFSET      = MAX_NODE_ID;
const UInt32 MAX_KEY_ID      = MAX_NODE_ID;
const UInt32 MAX_KEY_LENGTH  = 0x7FFF;

// The level of a block is incremented when find_offset() has failed to find
// a good offset in that block MAX_FAIL_COUNT times.
const UInt32 MAX_FAIL_COUNT  = 4;

// The maximum number of blocks tested in each call of find_offset().
const UInt32 MAX_BLOCK_COUNT = 16;
// A higher level block has less phantom nodes.
const UInt32 MAX_BLOCK_LEVEL = 5;

const UInt32 INVALID_LABEL   = 0x1FF;
const UInt32 INVALID_OFFSET  = 0;
const UInt32 INVALID_ENTRY   = 0x7FFFFFFF;
const UInt32 INVALID_NODE_ID = MAX_NODE_ID + 1;
const UInt32 INVALID_KEY_ID  = 0;

const UInt32 DEFAULT_FILE_SIZE          = 1U << 20;
const double DEFAULT_NUM_NODES_PER_KEY  = 4.0;
const double DEFAULT_AVERAGE_KEY_LENGTH = 16.0;

const UInt32 MAX_NUM_NODES    = MAX_NODE_ID + 1;
const UInt32 MAX_NUM_BLOCKS   = MAX_BLOCK_ID + 1;
const UInt32 MAX_NUM_KEYS     = MAX_KEY_ID + 1;
const UInt32 MAX_KEY_BUF_SIZE = 0xFFFFFFFFU;

class Exception : public std::exception {
 public:
  Exception() throw()
      : what_("") {}
  Exception(const char *what) throw()
      : what_((what != NULL) ? what : "") {}
  Exception(const Exception &ex) throw()
      : what_(ex.what_) {}
  virtual ~Exception() throw() {}

  Exception &operator=(const Exception &ex) throw() {
    what_ = ex.what_;
    return *this;
  }

  virtual const char *what() const throw() {
    return what_;
  }

 private:
  const char *what_;
};

#define GRN_DAT_DEFINE_ERROR(error_type) \
    class error_type : public Exception { \
     public: \
      error_type() throw() \
          : Exception() {} \
      error_type(const char *what) throw() \
          : Exception(what) {} \
      error_type(const error_type &ex) throw() \
          : Exception(ex) {} \
      virtual ~error_type() throw() {} \
     \
      error_type &operator=(const error_type &ex) throw() { \
        *static_cast<Exception *>(this) = ex; \
        return *this; \
      } \
    }

GRN_DAT_DEFINE_ERROR(ParamError);
GRN_DAT_DEFINE_ERROR(IOError);
GRN_DAT_DEFINE_ERROR(MemoryError);
GRN_DAT_DEFINE_ERROR(SizeError);

#undef GRN_DAT_DEFINE_ERROR

#define GRN_DAT_INT_TO_STR(value) \
    #value
#define GRN_DAT_LINE_TO_STR(line) \
    GRN_DAT_INT_TO_STR(line)
#define GRN_DAT_LINE_STR \
    GRN_DAT_LINE_TO_STR(__LINE__)

#define GRN_DAT_THROW(exception, what) \
    (throw exception(__FILE__ ":" GRN_DAT_LINE_STR ": " what))
#define GRN_DAT_THROW_IF(cond) \
    (void)((!(cond)) || (GRN_DAT_THROW(grn::dat::Exception, #cond), 0))

#define GRN_DAT_PARAM_ERROR_IF(cond) \
    (void)((!(cond)) || (GRN_DAT_THROW(grn::dat::ParamError, #cond), 0))
#define GRN_DAT_IO_ERROR_IF(cond) \
    (void)((!(cond)) || (GRN_DAT_THROW(grn::dat::IOError, #cond), 0))
#define GRN_DAT_MEMORY_ERROR_IF(cond) \
    (void)((!(cond)) || (GRN_DAT_THROW(grn::dat::MemoryError, #cond), 0))
#define GRN_DAT_SIZE_ERROR_IF(cond) \
    (void)((!(cond)) || (GRN_DAT_THROW(grn::dat::SizeError, #cond), 0))

#ifdef _DEBUG
 #define GRN_DAT_DEBUG_THROW_IF(cond) \
     GRN_DAT_THROW_IF(cond)
 #define GRN_DAT_DEBUG_LOG(var) \
     (std::clog << __FILE__ ":" GRN_DAT_LINE_STR ": " #var ": " \
                << (var) << std::endl)
#else  // _DEBUG
 #define GRN_DAT_DEBUG_THROW_IF(cond)
 #define GRN_DAT_DEBUG_LOG(var)
#endif  // _DEBUG

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_COMMON_HPP_
