#pragma once

#include <string>

namespace Ares
{

/// A path to a resource file, relative to the `FileStore`'s root.
class Path
{
    friend std::ostream& operator<<(std::ostream& stream, const Path& path);

    std::string path_;

public:
    Path()
        : path_()
    {
    }

    Path(const std::string& pathStr)
        : path_(pathStr)
    {
    }

    Path(std::string&& pathStr)
        : path_(std::move(pathStr))
    {
    }

    Path(const char* pathStr)
        : path_(pathStr)
    {
    }


    /// Returns the file extension of the given path (including the initial dot)
    /// or an empty string on error (path is empty, file has no extension, path
    /// is to a folder).
    /// **WARNING**: The returned string will only be valid until the path is
    ///              modified or destroyed!!
    const char* extension() const
    {
        const char* ext = "";
        for(const char* ch = &path_.back();
            ch != &path_.front() && *ch != '/'; // (not the first char, not the last slash)
            ch --)
        {
            if(*ch == '.')
            {
                // Found a dot after the last slash, update extension
                ext = ch;
            }
        }
        return ext;
    }

    /// Returns the dirname (i.e. the parent directory) of the given path, or an
    /// empty string on error (path contains no parent dir).
    ///
    /// Examples:
    /// `dirname("/") = "/"`
    /// `dirname("/x") = "/"`
    /// `dirname("/x/y") = "/x"`
    /// `dirname("/x/y/") = "/x"`
    /// `dirname("/x//y///") = "/x"`
    /// `dirname("x") = ""`
    /// `dirname("x/y") = "x"`
    /// `dirname("x/y/") = "x"`
    /// `dirname("x//y///") = "x"`
    const Path dirname() const
    {
        const char* firstCh = &path_[0];
        const char* lastCh = &path_[path_.length() - 1];
        bool foundNonSlash = false;
        for(; lastCh > firstCh; lastCh --)
        {
            if(*lastCh == '/')
            {
                if(foundNonSlash && *(lastCh - 1) != '/')
                {
                    // Found the first of the upper directory's separator
                    break;
                }
                // Else: skip trailing separator
            }
            else
            {
                foundNonSlash = true;
            }
        }

        size_t length = lastCh - firstCh;
        if(firstCh == lastCh && *firstCh == '/')
        {
            // Path relative to root, include the root slash
            length ++;
        }

        return Path(path_.substr(0, length));
    }


    inline operator std::string&()
    {
        return path_;
    }

    inline operator const std::string&() const
    {
        return path_;
    }


    inline bool operator==(const Path& other) const
    {
        // FIXME IMPLEMENT Perform a real path equality check, not just string equality
        return path_ == other.path_;
    }


    inline Path operator+(const Path& other) const
    {
        // FIXME IMPLEMENT Perform a real path concatenation, not just string concat
        return Path(path_ + other.path_);
    }

    inline Path& operator+=(const Path& other)
    {
        // FIXME IMPLEMENT Perform a real path concatenation, not just string concat
        path_ += other.path_;
        return *this;
    }
};

inline std::ostream& operator<<(std::ostream& stream, const Path& path)
{
    stream << path.path_;
    return stream;
}


}

namespace std
{

template <>
struct hash<Ares::Path>
{
    inline size_t operator()(const Ares::Path& value) const
    {
        // TODO Use a faster, non-crypto hash
        std::hash<std::string> strHasher;
        return strHasher(value);
    }
};

}
