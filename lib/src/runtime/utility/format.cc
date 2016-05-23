#include <puppet/runtime/utility/format.hpp>
#include <puppet/runtime/utility/format_map.hpp>
#include <puppet/utility/regex.hpp>
#include <boost/format.hpp>
#include <algorithm>

using namespace std;

namespace puppet { namespace runtime { namespace utility {

    format::format(string const& specification)
    {
        parse_specification(specification);
    }

    template <typename T>
    static boost::optional<T> lookup(runtime::values::hash& hash, char const* attribute, char const* type_name)
    {
        auto value = hash.get(attribute);
        if (!value) {
            return boost::none;
        }
        if (!value->as<T>()) {
            throw format_exception(
                (boost::format("expected %1% for '%2%' attribute but found %3%.") %
                 type_name %
                 attribute %
                 value->infer_type()
                ).str()
            );
        }
        return value->move_as<T>();
    }

    format::format(values::hash specification)
    {
        size_t found = 0;
        auto format = lookup<string>(specification, "format", types::string::name());
        if (format) {
            ++found;
            parse_specification(*format);
        }

        auto separator = lookup<string>(specification, "separator", types::string::name());
        if (separator) {
            ++found;
            _element_separator = rvalue_cast(*separator);
        }

        separator = lookup<string>(specification, "separator2", types::string::name());
        if (separator) {
            ++found;
            _key_value_separator = rvalue_cast(*separator);
        }

        auto formats = lookup<values::hash>(specification, "string_formats", types::hash::name());
        if (formats) {
            ++found;
            _element_format_map = make_unique<format_map>(rvalue_cast(*formats), false);
        }

        if (found != specification.size()) {
            // Find the first unsupported element
            for (auto& kvp : specification) {
                if (!kvp.key().as<string>()) {
                    throw format_exception(
                        (boost::format("expected %1% for hash key but found %2%.") %
                         types::string::name() %
                         kvp.key().infer_type()
                        ).str()
                    );
                }
                auto& key = kvp.key().require<string>();
                if (key == "format" || key == "separator" || key == "separator2" || key == "string_formats") {
                    continue;
                }
                throw format_exception(
                    (boost::format("unsupported format hash key '%1%'.") %
                     key
                    ).str()
                );
            }
        }
    }

    bool format::use_space() const
    {
        return _use_space;
    }

    bool format::alternative() const
    {
        return _alternative;
    }

    bool format::show_sign() const
    {
        return _show_sign;
    }

    bool format::left_justify() const
    {
        return _left_justify;
    }

    bool format::zero_pad() const
    {
        return _zero_pad;
    }

    char format::container_start() const
    {
        return _container_start;
    }

    int64_t format::width() const
    {
        return _width;
    }

    int64_t format::precision() const
    {
        return _precision;
    }

    char format::type() const
    {
        return _type;
    }

    string const& format::element_separator() const
    {
        return _element_separator;
    }

    string const& format::key_value_separator() const
    {
        return _key_value_separator;
    }

    format const* format::find_element_format(values::value const& value) const
    {
        return _element_format_map->find_format(value);
    }

    void format::parse_specification(string const& specification)
    {
        static puppet::utility::regex format_regex(R"(^%([ +\-#0\[{<(|]*)([1-9][0-9]*)?(?:\.([0-9]+))?([a-zA-Z])$)");

        puppet::utility::regex::regions regions;
        if (!format_regex.match(specification, &regions)) {
            throw format_exception(
                (boost::format("'%1%' is not a valid format string in the form of '%%<flags><width>.<precision><format>'.") %
                 specification
                ).str()
            );
        }

        // First region is the flags
        if (!regions.empty(1)) {
            parse_flags(specification.begin() + regions.begin(1), specification.begin() + regions.end(1));
        }

        // Second region is the width
        if (!regions.empty(2)) {
            parse_width(regions.substring(specification, 2));
        }

        // Third region is the precision
        if (!regions.empty(3)) {
            parse_precision(regions.substring(specification, 3));
        }

        // Forth region is the format type
        _type = specification[regions.begin(4)];
    }

    void format::parse_flags(string::const_iterator begin, string::const_iterator end)
    {
        for (; begin != end; ++begin) {
            switch (*begin) {
                case ' ':
                    if (_use_space) {
                        throw format_exception("the '<space>' flag can only be specified once.");
                    }
                    _use_space = true;
                    if (_container_start == -1) {
                        _container_start = 0;
                    }
                    break;

                case '+':
                    if (_show_sign) {
                        throw format_exception("the '+' flag can only be specified once.");
                    }
                    _show_sign = true;
                    break;

                case '-':
                    if (_left_justify) {
                        throw format_exception("the '-' flag can only be specified once.");
                    }
                    _left_justify = true;
                    break;

                case '#':
                    if (_alternative) {
                        throw format_exception("the '#' flag can only be specified once.");
                    }
                    _alternative = true;
                    break;

                case '0':
                    if (_zero_pad) {
                        throw format_exception("the '0' flag can only be specified once.");
                    }
                    _zero_pad = true;
                    break;

                case '[':
                case '{':
                case '<':
                case '(':
                case '|':
                    if (_container_start > 0) {
                        throw format_exception("the '[', '{', '<', '(', and '|' flags can only be specified once.");
                    }
                    _container_start = *begin;
                    break;

                default:
                    throw format_exception(
                        (boost::format("'%1%' is not a valid format flag.") %
                         *begin
                        ).str()
                    );
            }
        }
    }

    void format::parse_width(string const& width)
    {
        try {
            size_t pos = 0;
            _width = static_cast<int64_t>(stoll(width, &pos));
            if (pos == width.size()) {
                return;
            }
        } catch (invalid_argument const&) {
        } catch (out_of_range const&) {
            throw format_exception("format width is out of range.");
        }
        throw format_exception("format width is not valid.");
    }

    void format::parse_precision(string const& precision)
    {
        try {
            size_t pos = 0;
            _precision = static_cast<int64_t>(stoll(precision, &pos));
            if (pos == precision.size()) {
                return;
            }
        } catch (invalid_argument const&) {
        } catch (out_of_range const&) {
            throw format_exception("format precision is out of range.");
        }
        throw format_exception("format precision is not valid.");
    }

}}}  // namespace puppet::runtime::utility
