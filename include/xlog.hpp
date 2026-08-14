#ifndef XLOG_HPP
#define XLOG_HPP

#include "xlog.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace xlog {

class error : public std::runtime_error {
public:
    error(int code, const char *msg) : std::runtime_error(msg), code_(code) {}
    error(int code, const std::string &msg) : std::runtime_error(msg), code_(code) {}
    int code() const { return code_; }
private:
    int code_;
};

[[noreturn]] inline void throw_error(int code) {
    throw error(code, xlog_strerror(code));
}

namespace detail {

// strerror_r comes in two incompatible flavours: the XSI one returns int and
// fills buf, the GNU one returns a pointer that may not be buf. Overloading on
// the return type picks whichever is in scope.
inline const char *strerror_result(int rc, const char *buf) {
    return rc == 0 ? buf : "unknown error";
}
inline const char *strerror_result(const char *msg, const char *) { return msg; }

// The open functions report failure as NULL + errno; keep the errno text, it is
// the only thing that says why.
inline std::string open_error(const char *what, const char *path, int err) {
    char buf[256];
    buf[0] = '\0';
    std::string msg = "xlog: failed to open ";
    msg += what;
    msg += " '";
    msg += path;
    msg += "': ";
    msg += strerror_result(strerror_r(err, buf, sizeof(buf)), buf);
    return msg;
}

} // namespace detail

class writer {
public:
    explicit writer(const char *path) {
        w_ = xlog_writer_open(path);
        if(!w_) throw error(XLOG_ERR_IO, detail::open_error("writer", path, errno));
    }

    writer(const char *path, uint32_t max_record_size, int flags = 0) {
        w_ = xlog_writer_open_ex(path, max_record_size, flags);
        if(!w_) throw error(XLOG_ERR_IO, detail::open_error("writer", path, errno));
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

    // Commits the object representation of val, so only types whose bytes are
    // the whole value are accepted: a std::string or std::vector would commit
    // its internal pointers, a raw pointer the address instead of the pointee.
    template<typename T>
    void commit(const T &val) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "xlog::writer::commit(const T &) requires a trivially copyable type; "
                      "use commit(buf, size) to commit the bytes of anything else");
        static_assert(!std::is_pointer<T>::value,
                      "xlog::writer::commit(const T &) would commit the pointer itself; "
                      "use commit(ptr, size) to commit what it points to");
        commit(&val, sizeof(T));
    }

private:
    xlog_writer *w_;
};

class reader {
public:
    explicit reader(const char *path) {
        r_ = xlog_reader_open(path);
        if(!r_) throw error(XLOG_ERR_IO, detail::open_error("reader", path, errno));
    }

    reader(const char *path, uint32_t max_record_size, int flags = 0) {
        r_ = xlog_reader_open_ex(path, max_record_size, flags);
        if(!r_) throw error(XLOG_ERR_IO, detail::open_error("reader", path, errno));
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
