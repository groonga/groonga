/*
  Copyright(C) 2015 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef GROONGA_PORTABILITY_H
#define GROONGA_PORTABILITY_H

#ifdef WIN32
# ifdef __cplusplus
#  define grn_memcpy(dest, src, n) ::memcpy_s((dest), (n), (src), (n))
# else /* __cplusplus */
#  define grn_memcpy(dest, src, n) memcpy_s((dest), (n), (src), (n))
# endif /* __cplusplus */
#else /* WIN32 */
# ifdef __cplusplus
#  define grn_memcpy(dest, src, n) std::memcpy((dest), (src), (n))
# else /* __cplusplus */
#  define grn_memcpy(dest, src, n) memcpy((dest), (src), (n))
# endif /* __cplusplus */
#endif /* WIN32 */

#define GRN_ENV_BUFFER_SIZE 1024

#ifdef WIN32
# define grn_getenv(name, dest, dest_size) do {                         \
    char *dest_ = (dest);                                               \
    size_t dest_size_ = (dest_size);                                    \
    if (dest_size_ > 0) {                                               \
      DWORD env_size;                                                   \
      env_size = GetEnvironmentVariableA((name), dest_, dest_size_);    \
      if (env_size == 0 || env_size > dest_size_) {                     \
        dest_[0] = '\0';                                                \
      }                                                                 \
    }                                                                   \
  } while (0)
#else /* WIN32 */
# define grn_getenv(name, dest, dest_size) do {         \
    const char *env_value = getenv((name));             \
    char *dest_ = (dest);                               \
    size_t dest_size_ = (dest_size);                    \
    if (dest_size_ > 0) {                               \
      if (env_value) {                                  \
        strncpy(dest_, env_value, dest_size_ - 1);      \
      } else {                                          \
        dest_[0] = '\0';                                \
      }                                                 \
    }                                                   \
  } while (0)
#endif /* WIN32 */

#ifdef WIN32
# define grn_fopen(file, name, mode) do {       \
    if (!fopen_s(&file, name, mode)) {          \
      file = NULL;                              \
    }                                           \
  } while (0)
#else /* WIN32 */
# define grn_fopen(file, name, mode) (file) = fopen((name), (mode))
#endif /* WIN32 */

#ifdef WIN32
# define grn_strdup_raw(string) _strdup((string))
#else /* WIN32 */
# define grn_strdup_raw(string) strdup((string))
#endif /* WIN32 */

#endif /* GROONGA_PORTABILITY_H */
