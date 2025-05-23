#if (defined _WIN32) || (defined _WIN64)
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <windows.h>
#else  // (defined _WIN32) || (defined _WIN64)
 #include <sys/mman.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <fcntl.h>
 #include <unistd.h>
#endif  // (defined _WIN32) || (defined _WIN64)

#include "marisa/grimoire/io/mapper.h"

namespace marisa {
namespace grimoire {
namespace io {

#if (defined _WIN32) || (defined _WIN64)
Mapper::Mapper()
    : ptr_(NULL), origin_(NULL), avail_(0), size_(0),
      file_(NULL), map_(NULL) {}
#else  // (defined _WIN32) || (defined _WIN64)
Mapper::Mapper()
    : ptr_(NULL), origin_(MAP_FAILED), avail_(0), size_(0), fd_(-1) {}
#endif  // (defined _WIN32) || (defined _WIN64)

#if (defined _WIN32) || (defined _WIN64)
Mapper::~Mapper() {
  if (origin_ != NULL) {
    ::UnmapViewOfFile(origin_);
  }

  if (map_ != NULL) {
    ::CloseHandle(map_);
  }

  if (file_ != NULL) {
    ::CloseHandle(file_);
  }
}
#else  // (defined _WIN32) || (defined _WIN64)
Mapper::~Mapper() {
  if (origin_ != MAP_FAILED) {
    ::munmap(origin_, size_);
  }

  if (fd_ != -1) {
    ::close(fd_);
  }
}
#endif  // (defined _WIN32) || (defined _WIN64)

void Mapper::open(const char *filename, int flags) {
  MARISA_THROW_IF(filename == NULL, MARISA_NULL_ERROR);

  Mapper temp;
  temp.open_(filename, flags);
  swap(temp);
}

void Mapper::open(const void *ptr, std::size_t size) {
  MARISA_THROW_IF((ptr == NULL) && (size != 0), MARISA_NULL_ERROR);

  Mapper temp;
  temp.open_(ptr, size);
  swap(temp);
}

void Mapper::seek(std::size_t size) {
  MARISA_THROW_IF(!is_open(), MARISA_STATE_ERROR);
  MARISA_THROW_IF(size > avail_, MARISA_IO_ERROR);

  map_data(size);
}

bool Mapper::is_open() const {
  return ptr_ != NULL;
}

void Mapper::clear() {
  Mapper().swap(*this);
}

void Mapper::swap(Mapper &rhs) {
  marisa::swap(ptr_, rhs.ptr_);
  marisa::swap(avail_, rhs.avail_);
  marisa::swap(origin_, rhs.origin_);
  marisa::swap(size_, rhs.size_);
#if (defined _WIN32) || (defined _WIN64)
  marisa::swap(file_, rhs.file_);
  marisa::swap(map_, rhs.map_);
#else  // (defined _WIN32) || (defined _WIN64)
  marisa::swap(fd_, rhs.fd_);
#endif  // (defined _WIN32) || (defined _WIN64)
}

const void *Mapper::map_data(std::size_t size) {
  MARISA_THROW_IF(!is_open(), MARISA_STATE_ERROR);
  MARISA_THROW_IF(size > avail_, MARISA_IO_ERROR);

  const char * const data = static_cast<const char *>(ptr_);
  ptr_ = data + size;
  avail_ -= size;
  return data;
}

#if (defined _WIN32) || (defined _WIN64)
 #ifdef __MSVCRT_VERSION__
  #if __MSVCRT_VERSION__ >= 0x0601
   #define MARISA_HAS_STAT64
  #endif  // __MSVCRT_VERSION__ >= 0x0601
 #endif  // __MSVCRT_VERSION__
void Mapper::open_(const char *filename, int flags) {
  file_ = ::CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  MARISA_THROW_IF(file_ == INVALID_HANDLE_VALUE, MARISA_IO_ERROR);

  DWORD size_high, size_low;
  size_low = ::GetFileSize(file_, &size_high);
  MARISA_THROW_IF(size_low == INVALID_FILE_SIZE, MARISA_IO_ERROR);
  size_ = ((std::size_t)size_high << 32) | size_low;

  map_ = ::CreateFileMapping(file_, NULL, PAGE_READONLY, 0, 0, NULL);
  MARISA_THROW_IF(map_ == NULL, MARISA_IO_ERROR);

  origin_ = ::MapViewOfFile(map_, FILE_MAP_READ, 0, 0, 0);
  MARISA_THROW_IF(origin_ == NULL, MARISA_IO_ERROR);

  ptr_ = static_cast<const char *>(origin_);
  avail_ = size_;
}
#else  // (defined _WIN32) || (defined _WIN64)
void Mapper::open_(const char *filename, int flags) {
  fd_ = ::open(filename, O_RDONLY);
  MARISA_THROW_IF(fd_ == -1, MARISA_IO_ERROR);

  struct stat st;
  MARISA_THROW_IF(::fstat(fd_, &st) != 0, MARISA_IO_ERROR);
  MARISA_THROW_IF((UInt64)st.st_size > MARISA_SIZE_MAX, MARISA_SIZE_ERROR);
  size_ = (std::size_t)st.st_size;

  int map_flags = MAP_SHARED;
  if (flags & MARISA_MAP_POPULATE) {
    map_flags |= MAP_POPULATE;
  }

  origin_ = ::mmap(NULL, size_, PROT_READ, map_flags, fd_, 0);
  MARISA_THROW_IF(origin_ == MAP_FAILED, MARISA_IO_ERROR);

  ptr_ = static_cast<const char *>(origin_);
  avail_ = size_;
}
#endif  // (defined _WIN32) || (defined _WIN64)

void Mapper::open_(const void *ptr, std::size_t size) {
  ptr_ = ptr;
  avail_ = size;
}

}  // namespace io
}  // namespace grimoire
}  // namespace marisa
