//
// Created by season on 2021/6/27.
//

#pragma once

class Writer {
public:
    virtual bool Append(uint8_t *buf, size_t buf_size) = 0;

    virtual ~Writer() {}

protected:
    Writer() = default;

private:
    DISALLOW_COPY_AND_ASSIGN(Writer);
};