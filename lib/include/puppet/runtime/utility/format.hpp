/**
 * @file
 * Declares the format utility type.
 */
#pragma once

#include "../values/value.hpp"
#include <string>
#include <exception>
#include <memory>

namespace puppet { namespace runtime { namespace utility {

    // Forward declaration of format map
    struct format_map;

    /**
     * Exception type for format errors.
     */
    struct format_exception : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    /**
     * Represents a Puppet type format.
     */
    struct format
    {
        /**
         * Constructs a format from the given format specification.
         * @param specification The format specification to parse.
         */
        explicit format(std::string const& specification);

        /**
         * Constructs a format from the given format specification.
         * @param specification The format specification to parse.
         */
        explicit format(values::hash specification);

        /**
         * Gets whether or not space should be used instead of plus sign for numerical values.
         * @return Returns true if space should be used or false if not.
         */
        bool use_space() const;

        /**
         * Gets whether or not the alternative form should be used.
         * The alternative form varies depending on the format.
         * @return Returns true if the alternative form should be used or false if not.
         */
        bool alternative() const;

        /**
         * Gets whether or not plus or minus sign should be displayed for numerical values.
         * This changes xXobB numerical formats to not use 2's compliment form.
         * @return Returns true if a sign should be shown for numerical values or false if not.
         */
        bool show_sign() const;

        /**
         * Gets whether or not the value should be left justified to the given width.
         * @return Returns true if the value should be left justified or false if not.
         */
        bool left_justify() const;

        /**
         * Gets whether or not the value should be padded with '0' instead of space.
         * @return Returns true if the value should be padded with '0' instead of space.
         */
        bool zero_pad() const;

        /**
         * Gets the starting enclosing character (e.g. <, [, (, {, or |) to use for container values.
         * A value of <0 indicates to use the default character, 0 indicates no character, and >0 indicates the character to use.
         * @return Returns the starting enclosing character.
         */
        char container_start() const;

        /**
         * Gets the minimum width for formatted values.
         * A formatted value less than this width will be padded.
         * @return Returns the minimum width for formatted values.
         */
        int64_t width() const;

        /**
         * Gets the precision to use.
         * For floating point values, this is the maximum fractional digits to display.
         * For string values, this is the maximum characters to include in the formatted value.
         * @return Returns the precision to use or a value less than 0 if the precision was not specified.
         */
        int64_t precision() const;

        /**
         * Gets the format type.
         * The format type controls how a value is formatted.
         * @return Returns the format type.
         */
        char type() const;

        /**
         * Gets the separator to use between elements in a container.
         * @return Returns the separator to use between elements in a container.
         */
        std::string const& element_separator() const;

        /**
         * Gets the separator to use between key-value pairs in a container.
         * @return Returns the separator to use between key-value pairs in a container.
         */
        std::string const& key_value_separator() const;

        /**
         * Finds a format for an element value.
         * @param value The element value to find the format for.
         * @return Returns the element format or nullptr if there is no format for the given value.
         */
        format const* find_element_format(values::value const& value) const;

     private:
        void parse_specification(std::string const& specification);
        void parse_flags(std::string::const_iterator begin, std::string::const_iterator end);
        void parse_width(std::string const& width);
        void parse_precision(std::string const& precision);

        bool _use_space = false;
        bool _alternative = false;
        bool _show_sign = false;
        bool _left_justify = false;
        bool _zero_pad = false;
        char _container_start = -1;
        int64_t _width = 0;
        int64_t _precision = -1;
        char _type = 0;
        std::string _element_separator;
        std::string _key_value_separator;
        std::unique_ptr<format_map> _element_format_map;
    };

}}}  // namespace puppet::runtime::utility
