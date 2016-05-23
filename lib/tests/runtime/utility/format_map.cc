#include <catch.hpp>
#include <puppet/runtime/utility/format_map.hpp>

using namespace std;
using namespace puppet::runtime;
using namespace puppet::runtime::utility;

SCENARIO("using a format map", "[formatting]")
{
    WHEN("given a format string") {
        format_map map{ "%p" };
        THEN("all values should use that format") {
            auto format = map.find_format(0ll);
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
            format = map.find_format(values::array{});
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
            format = map.find_format(true);
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
        }
    }
    WHEN("given a hash key that is not valid") {
        values::hash hash;
        hash.set("foo", "bar");
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format_map{ puppet::rvalue_cast(hash) }, "expected Type for hash key but found String[3, 3].");
        }
    }
    WHEN("given a hash value that is not valid") {
        values::hash hash;
        hash.set(types::any{}, 0ll);
        THEN("it should throw an exception") {
            REQUIRE_THROWS_WITH(format_map{ puppet::rvalue_cast(hash) }, "expected Hash or String for hash value but found Integer[0, 0].");
        }
    }
    WHEN("given a complicated type map") {
        values::hash hash;
        hash.set(types::any{}, "%p");
        hash.set(types::array{}, "%s");
        values::hash array_format;
        array_format.set("format", "%a");
        hash.set(types::array{ nullptr, 3 }, puppet::rvalue_cast(array_format));
        hash.set(types::string{0, 5}, "%C");
        hash.set(types::string{6}, "%t");

        format_map map{ puppet::rvalue_cast(hash) };
        THEN("an array of less than 4 elements should have the expected format") {
            auto format = map.find_format(values::array{});
            REQUIRE(format);
            REQUIRE(format->type() == 's');
        }
        THEN("an array of more than 3 elements should have the expected format") {
            values::array array;
            array.push_back(1ll);
            array.push_back(2ll);
            array.push_back(3ll);
            array.push_back(4ll);
            auto format = map.find_format(array);
            REQUIRE(format);
            REQUIRE(format->type() == 'a');
        }
        THEN("a string of less than 6 characters should have the expected format") {
            auto format = map.find_format("foo");
            REQUIRE(format);
            REQUIRE(format->type() == 'C');
        }
        THEN("a string of more than 5 characters should have the expected format") {
            auto format = map.find_format("foobar");
            REQUIRE(format);
            REQUIRE(format->type() == 't');
        }
        THEN("all other types should have the expected format.") {
            auto format = map.find_format(values::regex{"foobar"});
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
            format = map.find_format(values::hash{});
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
            format = map.find_format(values::undef{});
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
            format = map.find_format(values::defaulted{});
            REQUIRE(format);
            REQUIRE(format->type() == 'p');
        }
    }
}
