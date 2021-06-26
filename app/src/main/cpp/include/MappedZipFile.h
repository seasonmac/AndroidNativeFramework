//
// Created by j00401612 on 2021/6/25.
//

#pragma once
namespace hms {
    class MappedZipFile {
    public:
        explicit MappedZipFile(const int fd)
                : has_fd_(true), fd_(fd), base_ptr_(nullptr), data_length_(0), read_pos_(0) {}

        explicit MappedZipFile(void *address, size_t length)
                : has_fd_(false),
                  fd_(-1),
                  base_ptr_(address),
                  data_length_(static_cast<off64_t>(length)),
                  read_pos_(0) {}

        bool HasFd() const { return has_fd_; }

        int GetFileDescriptor() const;

        void *GetBasePtr() const;

        off64_t GetFileLength() const;

        bool SeekToOffset(off64_t offset);

        bool ReadData(uint8_t *buffer, size_t read_amount);

        bool ReadAtOffset(uint8_t *buf, size_t len, off64_t off);

    private:
        // If has_fd_ is true, fd is valid and we'll read contents of a zip archive
        // from the file. Otherwise, we're opening the archive from a memory mapped
        // file. In that case, base_ptr_ points to the start of the memory region and
        // data_length_ defines the file length.
        const bool has_fd_;

        const int fd_;

        void *const base_ptr_;
        const off64_t data_length_;
        // read_pos_ is the offset to the base_ptr_ where we read data from.
        size_t read_pos_;

    private:
        bool ReadFully(int fd, void *data, size_t byte_count);
    };
}