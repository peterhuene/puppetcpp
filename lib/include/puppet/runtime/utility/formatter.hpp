/**
 * @file
 * Declares the formatter utility type.
 */
#pragma once

#include "format_map.hpp"
#include <iostream>

namespace puppet { namespace runtime { namespace utility {

    /**
     * Responsible for formatting a value.
     */
    struct formatter
    {
        /**
         * Constructs a formatter for the given stream.
         * @param stream The stream to write formatted values to.
         */
        explicit formatter(std::ostream& stream);

        /**
         * Formats a value based on the given format map.
         * @param value The value to format.
         * @param map The format map to use.
         */
        void format(values::value const& value, utility::format_map const& map) const;

     private:
        std::ostream& _stream;
    };

}}}  // namespace puppet::runtime::utility
