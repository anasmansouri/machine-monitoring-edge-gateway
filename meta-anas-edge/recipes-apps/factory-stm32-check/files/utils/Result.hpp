#pragma once
#include <cerrno>
#include <cstdint>
#include <optional>
#include <string>
namespace cc::utils {

enum class ErrorCode : std::int16_t {
    None,
    InvalidInput,
    DeviceOpenFailed,
    DeviceNotOpen,
    UartConfigFailed,
    UartReadFailed,
    UartWriteFailed,
    Timeout,
    ParseError,
    Unknown
};

struct Error {
    ErrorCode code;
    std::string message;
};

template <typename T> struct Result {
    std::optional<T> value;
    std::optional<Error> error;

    static Result ok(T v) {
        Result r;
        r.value = std::move(v);
        return r;
    }

    static Result fail(ErrorCode code, std::string message) {
        Result r;
        r.error = Error{code, std::move(message)};
        return r;
    }

    // true if value.has_value()
    explicit operator bool() const {
        if (value.has_value()) {
            return true;
        } else {
            return false;
        }
    }

    // precondition: value.has_value()
    const T& unwrap() const {
        return *value;
    }

    // precondition: error.has_value()
    const Error& unwrap_error() const {
        return *error;
    }
};

// That Result<void> part exists because templates normally expect a type parameter, but void is a
// special case in C++ — you can’t store a void in a member like std::optional<T>
template <> struct Result<void> {
    std::optional<Error> error;
    static Result ok() {
        return Result{};
    }

    static Result fail(ErrorCode code, std::string message) {
        Result r;
        r.error = Error{code, std::move(message)};
        return r;
    }
    // true if no error
    explicit operator bool() const {
        return !error.has_value();
    }
    const Error& unwrap_error() const {
        return *error;
    }
};
} // namespace cc::utils
