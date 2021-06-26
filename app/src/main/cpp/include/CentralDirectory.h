//
// Created by season on 2021/6/27.
//

#pragma once
class CentralDirectory {
public:
    CentralDirectory(void) : base_ptr_(nullptr), length_(0) {}

    const uint8_t* GetBasePtr() const { return base_ptr_; }

    size_t GetMapLength() const { return length_; }

    void Initialize(void* map_base_ptr, off64_t cd_start_offset, size_t cd_size) {
        base_ptr_ = static_cast<uint8_t *>(map_base_ptr) + cd_start_offset;
        length_ = cd_size;
    }

private:
    const uint8_t* base_ptr_;
    size_t length_;
};