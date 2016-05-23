/**
 * @file
 * Declares the format map utility type.
 */
#pragma once

#include "format.hpp"

namespace puppet { namespace runtime { namespace utility {

    /**
     * Represents a map between a type and a format.
     */
    struct format_map
    {
        /**
         * Constructs a format map from a hash value.
         * @param value The hash value to construct the format map from.
         * @param allow_hash True to allow hash specifications or false to only allow string specifications.
         */
        explicit format_map(values::hash value, bool allow_hash = true);

        /**
         * Constructs a format map from a a single string format.
         * @param format The format to use for all types.
         */
        explicit format_map(std::string const& format);

        /**
         * Finds a format for the given value.
         * @param value The value to find the format for.
         * @return Returns the format for the value or nullptr if no format for the value is present in the map.
         */
        format const* find_format(values::value const& value) const;

     private:
        std::vector<std::pair<values::type, format>> _formats;
    };

}}}  // namespace puppet::runtime::utility
