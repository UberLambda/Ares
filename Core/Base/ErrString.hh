#pragma once

#include <string>
#include <utility>
#include <ostream>

namespace Ares
{

/// A string used to store an optional error message.
class ErrString
{
    std::string str_;

public:
    /// Initializes a new, empty error string.
    ErrString()
        : str_() // (should not allocate any memory)
    {
    }

    /// Initializes a new error string containing a copy of the given message.
    ErrString(const std::string& message)
        : str_(message)
    {
    }
    ErrString(std::string&& messageToMove)
        : str_(std::move(messageToMove))
    {
    }
    ErrString(const char* message)
        : str_(message)
    {
    }

    ErrString(const ErrString& toCopy)
    {
        (void)operator=(toCopy);
    }
    ErrString& operator=(const ErrString& toCopy)
    {
        // Copy data over
        str_ = toCopy.str_;

        return *this;
    }

    ErrString(ErrString&& toMove)
    {
        (void)operator=(std::move(toMove));
    }
    ErrString& operator=(ErrString&& toMove)
    {
        // Move data over and invalidate moved instance
        str_ = std::move(toMove.str_);

        return *this;
    }


    /// Returns a non-null string with the error message if the error string
    /// actually contains a message, or null otherwise (i.e. if no error occurred).
    inline operator const char*() const
    {
        return str_.empty() ? nullptr : str_.c_str();
    }

    /// Returns string used to store the error itself. Empty if no error occurred.
    inline const std::string& str() const
    {
        return str_;
    }


    /// Returns an error string that is a concatenation of the two given ones.
    inline ErrString operator+(const ErrString& other) const
    {
        ErrString sum(str_);
        sum.str_ += other.str_;
        return sum;
    }

    /// See `operator+()`.
    inline ErrString& operator+=(const ErrString& other)
    {
        str_ += other.str_;
        return *this;
    }
};


inline std::ostream& operator<<(std::ostream& stream, const ErrString& error)
{
    return stream << (error ? error.str() : "<no error>");
}

}
