/**
 * @file
 * Declares the regex runtime value.
 */
#pragma once

#include "../../utility/regex.hpp"
#include <string>
#include <ostream>

namespace puppet { namespace compiler { namespace evaluation {

    // Forward declare the evaluation context.
    struct context;

}}}  // namespace puppet::compiler::evaluation

namespace puppet { namespace runtime { namespace values {

    /**
     * Represents a runtime regex.
     */
    struct regex : puppet::utility::regex
    {
        /**
         * Constructs a regex with the given pattern.
         * @param pattern The pattern for the regex.
         */
        explicit regex(std::string pattern);

        /**
         * Gets the pattern for the regex.
         * @return Returns the pattern for the regex.
         */
        std::string const& pattern() const;

        /**
         * Matches the given string value against the regular expression.
         * If the regular expression matches, match variables are set in the evaluation context.
         * @param context The evaluation context.
         * @param value The string value to match against.
         * @return Returns true if the regular expression matched or false if not.
         */
        bool match(compiler::evaluation::context& context, std::string const& value) const;

    private:
        std::string _pattern;
    };

    /**
     * Stream insertion operator for runtime regex.
     * @param os The output stream to write the runtime regex to.
     * @param regx The runtime regex to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, regex const& regx);

    /**
     * Equality operator for regex.
     * @param left The left regex to compare.
     * @param right The right regex to compare.
     * @return Returns true if both regexes have the same pattern or false if not.
     */
    bool operator==(regex const& left, regex const& right);

    /**
     * Inequality operator for regex.
     * @param left The left regex to compare.
     * @param right The right regex to compare.
     * @return Returns true if both regexes have different patterns or false if they are the same.
     */
    bool operator!=(regex const& left, regex const& right);

    /**
     * Hashes the regex value.
     * @param regex The regex value to hash.
     * @return Returns the hash value for the value.
     */
    size_t hash_value(values::regex const& regex);

}}}  // namespace puppet::runtime::values
