#include <catch.hpp>
#include <puppet/runtime/utility/format.hpp>
#include <puppet/runtime/utility/format_map.hpp>

using namespace std;
using namespace puppet::runtime;
using namespace puppet::runtime::utility;

SCENARIO("parsing a format", "[formatting]")
{
    WHEN("given an empty specification") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{""}, "'' is not a valid format string in the form of '%<flags><width>.<precision><format>'.");
        }
    }
    WHEN("given an invalid specification") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%nope"}, "'%nope' is not a valid format string in the form of '%<flags><width>.<precision><format>'.");
        }
    }
    WHEN("space flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%  a"}, "the '<space>' flag can only be specified once.");
        }
    }
    WHEN("plus flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%++a"}, "the '+' flag can only be specified once.");
        }
    }
    WHEN("minus flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%--a"}, "the '-' flag can only be specified once.");
        }
    }
    WHEN("alt flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%##a"}, "the '#' flag can only be specified once.");
        }
    }
    WHEN("pad flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%00a"}, "the '0' flag can only be specified once.");
        }
    }
    WHEN("a enclosing container flag is specified twice") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%[[a"}, "the '[', '{', '<', '(', and '|' flags can only be specified once.");
            REQUIRE_THROWS_WITH(format{"%{{a"}, "the '[', '{', '<', '(', and '|' flags can only be specified once.");
            REQUIRE_THROWS_WITH(format{"%<<a"}, "the '[', '{', '<', '(', and '|' flags can only be specified once.");
            REQUIRE_THROWS_WITH(format{"%((a"}, "the '[', '{', '<', '(', and '|' flags can only be specified once.");
            REQUIRE_THROWS_WITH(format{"%||a"}, "the '[', '{', '<', '(', and '|' flags can only be specified once.");
        }
    }
    WHEN("a width is out of range") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%9223372036854775808a"}, "format width is out of range.");
        }
    }
    WHEN("a precision is out of range") {
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{"%.9223372036854775808a"}, "format precision is out of range.");
        }
    }
    WHEN("given a simple format") {
        format fmt{"%p"};
        THEN("the format should have the expected values") {
            REQUIRE_FALSE(fmt.use_space());
            REQUIRE_FALSE(fmt.alternative());
            REQUIRE_FALSE(fmt.show_sign());
            REQUIRE_FALSE(fmt.left_justify());
            REQUIRE_FALSE(fmt.zero_pad());
            REQUIRE(fmt.container_start() < 0);
            REQUIRE(fmt.width() == 0);
            REQUIRE(fmt.precision() < 0);
            REQUIRE(fmt.type() == 'p');
        }
    }
    WHEN("given a single flag") {
        format fmt{"%-p"};
        THEN("the format should have the expected values") {
            REQUIRE_FALSE(fmt.use_space());
            REQUIRE_FALSE(fmt.alternative());
            REQUIRE_FALSE(fmt.show_sign());
            REQUIRE(fmt.left_justify());
            REQUIRE_FALSE(fmt.zero_pad());
            REQUIRE(fmt.container_start() < 0);
            REQUIRE(fmt.width() == 0);
            REQUIRE(fmt.precision() < 0);
            REQUIRE(fmt.type() == 'p');
        }
    }
    WHEN("given a format with all flags (except container start)") {
        format fmt{"% -+#0p"};
        THEN("the format should have the expected values") {
            REQUIRE(fmt.use_space());
            REQUIRE(fmt.alternative());
            REQUIRE(fmt.show_sign());
            REQUIRE(fmt.left_justify());
            REQUIRE(fmt.zero_pad());
            REQUIRE(fmt.container_start() == 0);
            REQUIRE(fmt.width() == 0);
            REQUIRE(fmt.precision() < 0);
            REQUIRE(fmt.type() == 'p');
        }
    }
    WHEN("given a format with all flags") {
        format fmt{"% -+#0{p"};
        THEN("the format should have the expected values") {
            REQUIRE(fmt.use_space());
            REQUIRE(fmt.alternative());
            REQUIRE(fmt.show_sign());
            REQUIRE(fmt.left_justify());
            REQUIRE(fmt.zero_pad());
            REQUIRE(fmt.container_start() == '{');
            REQUIRE(fmt.width() == 0);
            REQUIRE(fmt.precision() < 0);
            REQUIRE(fmt.type() == 'p');
        }
    }
    WHEN("given a format with a width") {
        format fmt{"%5a"};
        THEN("the format should have the expected values") {
            REQUIRE_FALSE(fmt.use_space());
            REQUIRE_FALSE(fmt.alternative());
            REQUIRE_FALSE(fmt.show_sign());
            REQUIRE_FALSE(fmt.left_justify());
            REQUIRE_FALSE(fmt.zero_pad());
            REQUIRE(fmt.container_start() < 0);
            REQUIRE(fmt.width() == 5);
            REQUIRE(fmt.precision() < 0);
            REQUIRE(fmt.type() == 'a');
        }
    }
    WHEN("given a format with a precision") {
        format fmt{"%.2f"};
        THEN("the format should have the expected values") {
            REQUIRE_FALSE(fmt.use_space());
            REQUIRE_FALSE(fmt.alternative());
            REQUIRE_FALSE(fmt.show_sign());
            REQUIRE_FALSE(fmt.left_justify());
            REQUIRE_FALSE(fmt.zero_pad());
            REQUIRE(fmt.container_start() < 0);
            REQUIRE(fmt.width() == 0);
            REQUIRE(fmt.precision() == 2);
            REQUIRE(fmt.type() == 'f');
        }
    }
    WHEN("given a format with a width and precision") {
        format fmt{"%3.2f"};
        THEN("the format should have the expected values") {
            REQUIRE_FALSE(fmt.use_space());
            REQUIRE_FALSE(fmt.alternative());
            REQUIRE_FALSE(fmt.show_sign());
            REQUIRE_FALSE(fmt.left_justify());
            REQUIRE_FALSE(fmt.zero_pad());
            REQUIRE(fmt.container_start() < 0);
            REQUIRE(fmt.width() == 3);
            REQUIRE(fmt.precision() == 2);
            REQUIRE(fmt.type() == 'f');
        }
    }
    WHEN("given a format with flags, a width, and a precision") {
        format fmt{"%| +0-#6.4h"};
        THEN("the format should have the expected values") {
            REQUIRE(fmt.use_space());
            REQUIRE(fmt.alternative());
            REQUIRE(fmt.show_sign());
            REQUIRE(fmt.left_justify());
            REQUIRE(fmt.zero_pad());
            REQUIRE(fmt.container_start() == '|');
            REQUIRE(fmt.width() == 6);
            REQUIRE(fmt.precision() == 4);
            REQUIRE(fmt.type() == 'h');
        }
    }
    WHEN("given a format hash with invalid format attribute") {
        values::hash hash;
        hash.set("format", 5ll);
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected String for 'format' attribute but found Integer[5, 5].");
        }
    }
    WHEN("given a format hash with invalid separator attribute") {
        values::hash hash;
        hash.set("separator", values::regex{ "foo" });
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected String for 'separator' attribute but found Regexp[/foo/].");
        }
    }
    WHEN("given a format hash with invalid separator2 attribute") {
        values::hash hash;
        hash.set("separator2", 2.0);
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected String for 'separator2' attribute but found Float[2, 2].");
        }
    }
    WHEN("given a format hash with invalid separator2 attribute") {
        values::hash hash;
        hash.set("string_formats", "nope");
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected Hash for 'string_formats' attribute but found String[4, 4].");
        }
    }
    WHEN("given a format hash with an unsupported attribute") {
        values::hash hash;
        hash.set("wrong", "nope");
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "unsupported format hash key 'wrong'.");
        }
    }
    WHEN("given multiple element formats") {
        values::hash formats;
        formats.set(types::integer{}, "%d");
        formats.set(types::integer{ 1 }, "%x");
        formats.set(types::integer{ std::numeric_limits<int64_t>::min(), -1 }, "%B");
        values::hash hash;
        hash.set("string_formats", puppet::rvalue_cast(formats));
        format fmt{ puppet::rvalue_cast(hash) };
        THEN("the formats should be sorted such that the correct formats are returned") {
            auto element_format = fmt.find_element_format(100ll);
            REQUIRE(element_format);
            REQUIRE(element_format->type() == 'x');
            element_format = fmt.find_element_format(-100ll);
            REQUIRE(element_format);
            REQUIRE(element_format->type() == 'B');
            element_format = fmt.find_element_format(0ll);
            REQUIRE(element_format);
            REQUIRE(element_format->type() == 'd');
        }
    }
    WHEN("given a non-type key in element formats") {
        values::hash formats;
        formats.set("wrong", "%d");
        values::hash hash;
        hash.set("string_formats", puppet::rvalue_cast(formats));
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected Type for hash key but found String[5, 5].");
        }
    }
    WHEN("given a non-string value in element formats") {
        values::hash formats;
        formats.set(types::any{}, values::hash{});
        values::hash hash;
        hash.set("string_formats", puppet::rvalue_cast(formats));
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format{ puppet::rvalue_cast(hash) }, "expected String for hash value but found Hash[0, 0].");
        }
    }
    WHEN("given a valid format hash") {
        values::hash formats;
        formats.set(types::string{}, "%s");
        formats.set(types::integer{}, "%f");

        values::hash hash;
        hash.set("format", "%| +0-#6.4h");
        hash.set("separator", "+");
        hash.set("separator2", " -> ");
        hash.set("string_formats", puppet::rvalue_cast(formats));
        format fmt{ puppet::rvalue_cast(hash) };

        THEN("the format should have the expected values") {
            REQUIRE(fmt.use_space());
            REQUIRE(fmt.alternative());
            REQUIRE(fmt.show_sign());
            REQUIRE(fmt.left_justify());
            REQUIRE(fmt.zero_pad());
            REQUIRE(fmt.container_start() == '|');
            REQUIRE(fmt.width() == 6);
            REQUIRE(fmt.precision() == 4);
            REQUIRE(fmt.type() == 'h');
            REQUIRE(fmt.element_separator() == "+");
            REQUIRE(fmt.key_value_separator() == " -> ");
            auto element_format = fmt.find_element_format("foo");
            REQUIRE(element_format);
            REQUIRE(element_format->type() == 's');
            element_format = fmt.find_element_format(5ll);
            REQUIRE(element_format);
            REQUIRE(element_format->type() == 'f');
            REQUIRE_FALSE(fmt.find_element_format(types::string{}));
        }
    }
}
