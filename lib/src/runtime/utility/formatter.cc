#include <puppet/runtime/utility/formatter.hpp>
#include <boost/format.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/lexical_cast.hpp>
#include <utf8.h>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <bitset>

using namespace std;
using namespace puppet::runtime::values;

namespace puppet { namespace runtime { namespace utility {

    formatter::formatter(ostream& stream) :
        _stream(stream)
    {
    }

    struct format_visitor : boost::static_visitor<void>
    {
        format_visitor(ostream& stream, format_map const& map, utility::format const* format = nullptr, bool default_programatic = false) :
            _stream(stream),
            _map(map),
            _format(format),
            _default_programatic(default_programatic)
        {
        }

        void operator()(undef const&) const
        {
            auto type = this->type();
            switch (type) {
                case 'n':
                    format(alternative() ? "null" : "nil");
                    break;

                case 'u':
                    format(alternative() ? "undefined" : "undef");
                    break;

                case 'd':
                case 'x':
                case 'X':
                case 'o':
                case 'b':
                case 'B':
                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G':
                case 'a':
                case 'A':
                    format("NaN");
                    break;

                case 'v':
                    format("n/a");
                    break;

                case 'V':
                    format("N/A");
                    break;

                case 's':
                    format(alternative() ? R"("")" : "");
                    break;

                case 'p':
                    format(alternative() ? R"("undef")" : "undef");
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::undef::name()
                        ).str()
                    );
            }
        }

        void operator()(defaulted const&) const
        {
            auto type = this->type();
            switch (type) {
                case 'd':
                case 's':
                case 'p':
                    format(alternative() ? R"("default")" : "default");
                    break;

                case 'D':
                    format(alternative() ? R"("Default")" : "Default");
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::defaulted::name()
                        ).str()
                    );
            }
        }

        void operator()(int64_t value) const
        {
            auto type = this->type('d');
            switch (type) {
                case 'd':
                case 'p':
                    format(value);
                    break;

                case 'x':
                case 'X':
                    format(value, (type == 'X' ? integer_format::hex_uppercase : integer_format::hex));
                    break;

                case 'o':
                    format(value, integer_format::octal);
                    break;

                case 'b':
                case 'B':
                    format(value, (type == 'X' ? integer_format::binary_uppercase : integer_format::binary));
                    break;

                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G':
                case 'a':
                case 'A':
                    operator()(static_cast<double>(value));
                    break;

                case 'c':
                    {
                        // If the high order DWORD is non-zero, raise an error
                        uint32_t high = static_cast<uint32_t>(static_cast<uint64_t>(value) >> 32);
                        uint32_t low = static_cast<uint32_t>(static_cast<uint64_t>(value) & (1ull >> 32));
                        if (high) {
                            throw format_exception(
                                (boost::format("numeric value '%1%' exceeds the range of a Unicode code point.") %
                                 value
                                ).str()
                            );
                        }

                        try
                        {
                            string str;
                            utf8::utf32to8(&low, &low + 1, back_inserter(str));
                            format(str);
                        } catch (utf8::invalid_utf8 const&) {
                            throw format_exception(
                                (boost::format("numeric value '%1%' is not a valid Unicode code point.") %
                                 value
                                ).str()
                            );
                        }
                    }
                    break;

                case 's':
                    {
                        // Convert to a string and then format as a string (respects width and precision differently)
                        ostringstream ss;
                        utility::format f{ "%p" };
                        values::value v{ value };

                        if (alternative()) {
                            ss << '"';
                        }
                        boost::apply_visitor(format_visitor{ ss, _map, &f }, v);
                        if (alternative()) {
                            ss << '"';
                        }

                        format(ss.str());
                    }
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::integer::name()
                        ).str()
                    );
            }
        }

        void operator()(double value) const
        {
            auto type = this->type('f');
            switch (type) {
                case 'd':
                case 'x':
                case 'X':
                case 'o':
                case 'b':
                case 'B':
                    operator()(static_cast<int64_t>(value));
                    break;

                case 'e':
                case 'E':
                    format(value, (type == 'E' ? float_format::scientific_uppercase : float_format::scientific));
                    break;

                case 'f':
                    format(value);
                    break;

                case 'g':
                case 'G':
                    if (requires_exponential_form(value)) {
                        format(value, (type == 'G' ? float_format::scientific_uppercase : float_format::scientific));
                    } else {
                        format(value);
                    }
                    break;

                case 'a':
                case 'A':
                    format(value, (type == 'A' ? float_format::hex_uppercase : float_format::hex));
                    break;

                case 'p':
                    format(value, float_format::fixed, true);
                    break;

                case 's':
                    {
                        // Convert to a string and then format as a string (respects width and precision differently)
                        ostringstream ss;
                        utility::format f{ "%p" };
                        values::value v{ value };

                        if (alternative()) {
                            ss << '"';
                        }
                        boost::apply_visitor(format_visitor{ ss, _map, &f }, v);
                        if (alternative()) {
                            ss << '"';
                        }

                        format(ss.str());
                    }
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::floating::name()
                        ).str()
                    );
            }
        }

        void operator()(bool value) const
        {
            auto type = this->type();
            switch (type) {
                case 't':
                    format(alternative() ? (value ? "t" : "f") : (value ? "true" : "false"));
                    break;

                case 'T':
                    format(alternative() ? (value ? "T" : "F") : (value ? "True" : "False"));
                    break;

                case 'y':
                    format(alternative() ? (value ? "y" : "n") : (value ? "yes" : "no"));
                    break;

                case 'Y':
                    format(alternative() ? (value ? "Y" : "N") : (value ? "Yes" : "No"));
                    break;

                case 'd':
                case 'x':
                case 'X':
                case 'o':
                case 'b':
                case 'B':
                    operator()(value ? 1ll : 0ll);
                    break;

                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G':
                case 'a':
                case 'A':
                    operator()(value ? 1.0 : 0.0);
                    break;

                case 's':
                case 'p':
                    format(value ? "true" : "false");
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::boolean::name()
                        ).str()
                    );
            }
        }

        void operator()(std::string const& value) const
        {
            auto type = this->type();
            switch (type) {
                case 's':
                    format(value);
                    break;

                case 'p':
//                    switch (c) {
//                        case '\n': cc = 'n'; break;
//                        case '\r': cc = 'r'; break;
//                        case '\t': cc = 't'; break;
//                        case '\f': cc = 'f'; break;
//                        case '\013': cc = 'v'; break;
//                        case '\010': cc = 'b'; break;
//                        case '\007': cc = 'a'; break;
//                        case 033: cc = 'e'; break;
//                        default: cc = 0; break;
                    // TODO: quote and escape string
                    format(value);
                    break;

                case 'c':
                    // TODO: capitalize
                    // Make the type name lowercase
                    auto s = boost::to_lower_copy(value);
                    if

                    // Now uppercase every start of a type name
                    boost::split_iterator<std::string::iterator> end;
                    for (auto it = boost::make_split_iterator(_type_name, boost::first_finder("::", boost::is_equal())); it != end; ++it) {
                        if (!*it) {
                            continue;
                        }
                        auto range = boost::make_iterator_range(it->begin(), it->begin() + 1);
                        boost::to_upper(range);
                    }
                    // If alternative, escape/quote
                    format(value);
                    break;

                case 'C':
                    // Treat the string as a resource by capitalizing each :: segment
                    format(type:resource{ value }.type_name());
                    // TODO: segment capitalize
                    // If alternative, escape/quote
                    boost
                    format(value);
                    break;

                case 'u':
                    // TODO: upcase string
                    // If alternative, escape quote
                    format(value);
                    break;

                case 'd':
                    // TODO: lowercase string
                    // If alternative, escape quote
                    format(value);
                    break;

                case 't':
                    // TODO: trim string
                    // If alternative, escape quote
                    format(value);
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::string::name()
                        ).str()
                    );
            }
        }

        void operator()(values::regex const& value) const
        {
            auto type = this->type();
            switch (type) {
                case 'p':
                    format("/" + value.pattern() + "/");
                    break;

                case 's':
                    if (alternative()) {
                        format("\"" + value.pattern() + "\"");
                    } else {
                        format("/" + value.pattern() + "/");
                    }
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::regexp::name()
                        ).str()
                    );
            }
        }

        void operator()(values::type const& value) const
        {
            auto type = this->type();
            switch (type) {
                case 'p':
                    format(boost::lexical_cast<string>(value));
                    break;

                case 's':
                    if (alternative()) {
                        format("\"" + boost::lexical_cast<string>(value) + "\"");
                    } else {
                        format(boost::lexical_cast<string>(value));
                    }
                    break;

                default:
                    throw format_exception(
                        (boost::format("unsupported format '%1%' for %2%.") %
                         type %
                         types::type::name()
                        ).str()
                    );
            }
        }

        void operator()(variable const& value) const
        {
            boost::apply_visitor(*this, value.value());
        }

        void operator()(values::array const& value) const
        {

        }

        void operator()(values::hash const& value) const
        {

        }

        void operator()(values::iterator const& value) const
        {
        }

     private:
        char type(char default_type = 0) const
        {
            if (_format) {
                return _format->type();
            }
            if (default_type > 0) {
                return default_type;
            }
            return _default_programatic ? 'p' : 's';
        }

        bool alternative() const
        {
            return _format && _format->alternative();
        }

        bool left_justify() const
        {
            return _format && _format->left_justify();
        }

        bool zero_pad() const
        {
            return _format && _format->zero_pad();
        }

        bool show_sign() const
        {
            return _format && _format->show_sign();
        }

        bool use_space() const
        {
            return _format && _format->use_space();
        }

        int64_t width() const
        {
            return _format ? _format->width() : 0;
        }

        int64_t precision() const
        {
            return _format ? _format->precision() : -1;
        }

        bool requires_exponential_form(double value) const
        {
            // For conditional exponential form, follow Ruby MRI logic:
            // Use exponential form if the exponent is less than -4 or
            // greater to or equal the requested precision
            auto precision = this->precision();
            int exp = (value == 0) ? 0 : static_cast<int>(1 + std::log10(std::fabs(value)));
            return exp < -4 || (precision >= 0 && exp >= precision);
        }

        void format(string const& str) const
        {
            boost::io::ios_flags_saver flags_saver{ _stream };
            boost::io::ios_width_saver width_saver{ _stream };
            boost::io::ios_fill_saver fill_saver{ _stream };

            _stream << (left_justify() ? std::left : std::right);
            _stream << std::setw(width());
            _stream << std::setfill(zero_pad() ? '0' : ' ');

            // If precision is -1, then it will cast to maximum and output the entire string
            // TODO: this is incorrect for unicode strings as this is byte precision and not character; will fix in a follow-up commit
            auto precision = static_cast<size_t>(this->precision());
            if (precision < str.size()) {
                _stream << str.substr(0, precision);
            } else {
                _stream << str;
            }
        }

        enum class float_format
        {
            fixed,
            scientific,
            scientific_uppercase,
            hex,
            hex_uppercase
        };

        void format(double value, float_format format = float_format::fixed, bool ignore_precision = false) const
        {
            boost::io::ios_flags_saver flags_saver{ _stream };
            boost::io::ios_precision_saver precision_saver{ _stream };
            boost::io::ios_width_saver width_saver{ _stream };
            boost::io::ios_fill_saver fill_saver{ _stream };

            bool show_space = !show_sign() && use_space();

            _stream << (left_justify() ? std::left : std::right);
            _stream << (show_space && width() > 0 ? std::setw(width() - 1) : std::setw(width()));
            _stream << std::setfill(zero_pad() ? '0' : ' ');

            if (show_space) {
                if (value > 0) {
                    _stream << ' ';
                }
            } else if (show_sign()) {
                _stream << std::showpos;
            }

            if (!ignore_precision) {
                auto precision = this->precision();
                if (precision >= 0) {
                    _stream << std::setprecision(precision);
                }
            }

            switch (format) {
                case float_format::scientific_uppercase:
                    _stream << std::uppercase;
                case float_format::scientific:
                    _stream << std::scientific;
                    break;

                case float_format::hex_uppercase:
                    _stream << std::uppercase;
                case float_format::hex:
                    _stream << std::hexfloat;
                    break;

                case float_format::fixed:
                default:
                    _stream << std::fixed;
                    break;
            }

            _stream << std::showpoint << value;
        }

        enum class integer_format
        {
            binary,
            binary_uppercase,
            octal,
            decimal,
            hex,
            hex_uppercase
        };

        void format(int64_t value, integer_format format = integer_format::decimal) const
        {
            boost::io::ios_flags_saver flags_saver{ _stream };
            boost::io::ios_precision_saver precision_saver{ _stream };
            boost::io::ios_width_saver width_saver{ _stream };
            boost::io::ios_fill_saver fill_saver{ _stream };

            auto width = this->width();

            _stream << (left_justify() ? std::left : std::right);
            _stream << std::setfill(zero_pad() ? '0' : ' ');

            char const* base = nullptr;
            char const* bits = nullptr;

            switch (format) {
                case integer_format::binary_uppercase:
                    base = "0B";
                    bits = "..1";
                    break;

                case integer_format::binary:
                    base = "0b";
                    bits = "..1";
                    break;

                case integer_format::octal:
                    base = "0";
                    bits = "..7";
                    break;

                case integer_format::hex_uppercase:
                    base = "0X";
                    bits = "..f";
                    break;

                case integer_format::hex:
                    base = "0x";
                    bits = "..f";
                    break;

                case integer_format::decimal:
                default:
                    break;
            }

            // Output the sign
            if (!show_sign() && use_space() && value > 0) {
                _stream << ' ';
                --width;
            } else if (show_sign()) {
                _stream << (value >= 0 ? '+' : '-');
                --width;
            } else if (value < 0) {
                _stream << '-';
                --width;
            }

            if (alternative() && base) {
                _stream << format == integer_format::binary_uppercase ? "0B" : "0b";
            }

            if (width > 0) {
                _stream << std::setw(width);
            }

            _stream << std::noshowbase;
                            if (alternative()) {
                                _stream << format == integer_format::binary_uppercase ? "0B" : "0b";
                            }
                            if (!show_sign() && value < 0) {
                                _stream << "..1";
                                if (value == -1) {
                                    // ..1 means "all 1s", so we're done
                                    return;
                                }
                                // Convert to a masked 2's compliment
                                value = masking_twos_compliment(value);
                            }
                            _stream << std::bitset<64>{ static_cast<uint64_t>(value) };
                        }
                        return;

                    case integer_format::octal:
                        _stream << std::oct;
                        break;

                    case integer_format::hex_uppercase:
                        _stream << std::uppercase;
                    case integer_format::hex:
                        _stream << std::hex;
                        break;

                    case integer_format::decimal:
                    default:
                        _stream << std::dec;
                        break;
                }
            }

            switch (format) {
                case integer_format::binary_uppercase:
                case integer_format::binary:
                    {
                        if (show_sign()) {
                            _stream << (value >= 0 ? "+" : "-");
                        }
                        if (alternative()) {
                            _stream << format == integer_format::binary_uppercase ? "0B" : "0b";
                        }
                        if (!show_sign() && value < 0) {
                            _stream << "..1";
                            if (value == -1) {
                                // ..1 means "all 1s", so we're done
                                return;
                            }
                            // Convert to a masked 2's compliment
                            value = masking_twos_compliment(value);
                        }
                        _stream << std::bitset<64>{ static_cast<uint64_t>(value) };
                    }
                    return;

                case integer_format::octal:
                    _stream << std::oct;
                    break;

                case integer_format::hex_uppercase:
                    _stream << std::uppercase;
                case integer_format::hex:
                    _stream << std::hex;
                    break;

                case integer_format::decimal:
                default:
                    _stream << std::dec;
                    break;
            }

            _stream << value;
        }

        static uint8_t msb(uint64_t value)
        {
            // Find the most significant bit in value
            // This is a naive impementation
            // It could be optimized by unrolling the loop or probably by some bitwise magic
            uint8_t bit = 0;
            while (value >>= 1) {
                ++bit;
            }
            return bit;
        }

        static int64_t masking_twos_compliment(int64_t value)
        {
            // This performs a twos compliment and masks away insignificant bits
            // For example: -10 => ..f6, where ..f implies all the leading bits are 1 (for hex format)
            // Thus this function returns the part that is significant (6 in the example above)
            // Note: this does not give the correct result if given -1 as the significant part would be "empty" (i.e. all ones)
            return value & ((1 << (msb(static_cast<uint64_t>(~value + 1) + 1) )) - 1);
        }

        ostream& _stream;
        format_map const& _map;
        utility::format const* _format;
        bool _default_programatic;
    };

    void formatter::format(values::value const& value, utility::format_map const& map) const
    {
        boost::apply_visitor(format_visitor{ _stream, map, map.find_format(value) }, value);
    }

}}}  // namespace puppet::runtime::utility
