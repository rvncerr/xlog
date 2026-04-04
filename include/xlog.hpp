#ifndef XLOG_HPP
#define XLOG_HPP

#include "xlog.h"
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
    throw error(code, xlog_strerror(code));
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

    ~writer() { if(w_) (void)xlog_writer_close(w_); }

    writer(const writer &) = delete;
    writer &operator=(const writer &) = delete;
    writer(writer &&o) noexcept : w_(o.w_) { o.w_ = nullptr; }
    writer &operator=(writer &&o) noexcept {
        if(this != &o) { if(w_) (void)xlog_writer_close(w_); w_ = o.w_; o.w_ = nullptr; }
        return *this;
    }

    // Explicit close — throws on error (e.g. XLOG_ERR_IO).
    // Call before destruction to detect sync/close failures.
    void close() {
        if(!w_) return;
        int rc = xlog_writer_close(w_);
        w_ = nullptr;
        if(rc < 0) throw_error(rc);
    }

    void commit(const void *buf, size_t sz) {
        int rc = xlog_writer_commit(w_, buf, sz);
        if(rc < 0) throw_error(rc);
    }

    template<typename T>
    void commit(const T &val) { commit(&val, sizeof(T)); }

private:
    xlog_writer *w_;
};

class reader {
public:
    explicit reader(const char *path) {
        r_ = xlog_reader_open(path);
        if(!r_) throw error(XLOG_ERR_IO, "xlog: failed to open reader");
    }

    reader(const char *path, uint32_t max_record_size, int flags = 0) {
        r_ = xlog_reader_open_ex(path, max_record_size, flags);
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

    void reset() {
        int rc = xlog_reader_reset(r_);
        if(rc < 0) throw_error(rc);
    }

    ssize_t next(void *buf, size_t cap) {
        ssize_t sz = xlog_reader_next(r_, buf, cap);
        if(sz < 0) throw_error((int)sz);
        return sz;
    }

    std::vector<uint8_t> next(size_t cap) {
        std::vector<uint8_t> buf(cap);
        ssize_t sz = xlog_reader_next(r_, buf.data(), buf.size());
        if(sz < 0) throw_error((int)sz);
        buf.resize(sz);
        return buf;
    }

private:
    xlog_reader *r_;
};

} // namespace xlog

#endif // XLOG_HPP
