#ifndef VERSION_STRING_HPP
#define VERSION_STRING_HPP

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace novatel::edie {

struct VersionId
{
    unsigned major = 0, minor = 0, patch = 0; // unsigned so from_chars rejects "1.-2"

  private:
    constexpr std::tuple<unsigned, unsigned, unsigned> as_tuple() const { return {major, minor, patch}; }

  public:
    // Always renders all three fields, so a version parsed from "1.2" round-trips as "1.2.0".
    std::string to_string() const { return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch); }

    // std::tuple's relational operators give lexicographic member-wise comparison
    friend constexpr bool operator==(const VersionId& a, const VersionId& b) { return a.as_tuple() == b.as_tuple(); }
    friend constexpr bool operator!=(const VersionId& a, const VersionId& b) { return !(a == b); }
    friend constexpr bool operator<(const VersionId& a, const VersionId& b) { return a.as_tuple() < b.as_tuple(); }
    friend constexpr bool operator>(const VersionId& a, const VersionId& b) { return b < a; }
    friend constexpr bool operator<=(const VersionId& a, const VersionId& b) { return !(b < a); }
    friend constexpr bool operator>=(const VersionId& a, const VersionId& b) { return !(a < b); }
};

// Parses a leading "MAJOR[.MINOR[.PATCH]]" and ignores everything after it.
// Omitted trailing fields are 0, so "1.2" == 1.2.0.
// "1.2.3.4" -> 1.2.3   "1.2.3-rc1" -> 1.2.3   "1.2-rc1" -> 1.2.0   "1.2.x" -> 1.2.0
// Still nullopt if the string doesn't start with a digit: "v1.2.3", "", "x".
inline std::optional<VersionId> parse_version(std::string_view s)
{
    VersionId v;
    for (unsigned* field : {&v.major, &v.minor, &v.patch})
    {
        auto res = std::from_chars(s.data(), s.data() + s.size(), *field);
        if (res.ec != std::errc{}) return std::nullopt;
        s.remove_prefix(res.ptr - s.data());
        if (!s.empty() && s.front() == '.')
            s.remove_prefix(1);
        else
            break;
    }
    return v;
}

} // namespace novatel::edie

#endif // VERSION_STRING_HPP
