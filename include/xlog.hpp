#ifndef XLOG_HPP
#define XLOG_HPP

#include "xlog.h"
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace xlog {

class error : public std::runtime_error {
public:
    error(int code, const char *msg) : std::runtime_error(msg), code_(code) {}
    int code() const { return code_; }
private:
    int code_;
};

[[noreturn]] inline void throw_error(int code) {
    switch(code) {
    case XLOG_ERR_IO:   throw error(code, "xlog: I/O error");
    case XLOG_ERR_CRC:  throw error(code, "xlog: checksum mismatch");
    case XLOG_ERR_SIZE: throw error(code, "xlog: record size invalid");
    case XLOG_ERR_MEM:  throw error(code, "xlog: out of memory");
    default:            throw error(code, "xlog: unknown error");
    }
}

class writer {
public:
    explicit writer(const char *path) {
        w_ = xlog_writer_open(path);
        if(!w_) throw error(XLOG_ERR_IO, "xlog: failed to open writer");
    }

    writer(const char *path, uint32_t max_record_size, int flags = 0) {
        w_ = xlog_writer_open_ex(path, max_record_size, flags);
        if(!w_) throw error(XLOG_ERR_IO, "xlog: failed to open writer");
    }

    ~writer() { if(w_) xlog_writer_close(w_); }

    writer(const writer &) = delete;
    writer &operator=(const writer &) = delete;
    writer(writer &&o) noexcept : w_(o.w_) { o.w_ = nullptr; }
    writer &operator=(writer &&o) noexcept {
        if(this != &o) { if(w_) xlog_writer_close(w_); w_ = o.w_; o.w_ = nullptr; }
        return *this;
    }

    void commit(const void *buf, size_t sz) {
        int rc = xlog_writer_commit(w_, buf, sz);
        if(rc < 0) throw_error(rc);
    }

    template<typename T>
    void commit(const T &val) { commit(&val, sizeof(T)); }

private:
    xlog_writer_t *w_;
};

class reader {
public:
    explicit reader(const char *path) {
        r_ = xlog_reader_open(path);
        if(!r_) throw error(XLOG_ERR_IO, "xlog: failed to open reader");
    }

    reader(const char *path, uint32_t max_record_size) {
        r_ = xlog_reader_open_ex(path, max_record_size);
        if(!r_) throw error(XLOG_ERR_IO, "xlog: failed to open reader");
    }

    ~reader() { if(r_) xlog_reader_close(r_); }

    reader(const reader &) = delete;
    reader &operator=(const reader &) = delete;
    reader(reader &&o) noexcept : r_(o.r_) { o.r_ = nullptr; }
    reader &operator=(reader &&o) noexcept {
        if(this != &o) { if(r_) xlog_reader_close(r_); r_ = o.r_; o.r_ = nullptr; }
        return *this;
    }

    void reset() { xlog_reader_reset(r_); }

    ssize_t next(void **buf) {
        ssize_t sz = xlog_reader_next(r_, buf);
        if(sz < 0) throw_error((int)sz);
        return sz;
    }

    std::vector<uint8_t> next() {
        void *buf;
        ssize_t sz = xlog_reader_next(r_, &buf);
        if(sz < 0) throw_error((int)sz);
        if(sz == 0) return {};
        std::vector<uint8_t> result((uint8_t *)buf, (uint8_t *)buf + sz);
        free(buf);
        return result;
    }

private:
    xlog_reader_t *r_;
};

} // namespace xlog

#endif // XLOG_HPP
