#include <puppet/runtime/utility/format_map.hpp>
#include <boost/format.hpp>

using namespace std;

namespace puppet { namespace runtime { namespace utility {

    static size_t get_type_rank(values::type const& type)
    {
        // Returns the rank from lowest generality to highest
        if (boost::get<types::structure>(&type)) {
            return 1;
        }
        if (boost::get<types::hash>(&type)) {
            return 2;
        }
        if (boost::get<types::tuple>(&type)) {
            return 3;
        }
        if (boost::get<types::array>(&type)) {
            return 4;
        }
        if (boost::get<types::pattern>(&type)) {
            return 5;
        }
        if (boost::get<types::enumeration>(&type)) {
            return 6;
        }
        if (boost::get<types::string>(&type)) {
            return 7;
        }
        return numeric_limits<size_t>::max();
    }

    format_map::format_map(values::hash value, bool allow_hash)
    {
        for (auto& kvp : value) {
            // Ensure the key is a type
            if (!kvp.key().as<values::type>()) {
                throw format_exception(
                    (boost::format("expected %1% for hash key but found %2%.") %
                     types::type::name() %
                     kvp.key().infer_type()
                    ).str()
                );
            }

            if (allow_hash) {
                if (kvp.value().as<values::hash>()) {
                    _formats.emplace_back(kvp.key().require<values::type>(), format{ kvp.value().move_as<values::hash>() });
                    continue;
                }
                // Ensure the value is a string if it is not a hash
                if (!kvp.value().as<string>()) {
                    throw format_exception(
                        (boost::format("expected %1% or %2% for hash value but found %3%.") %
                         types::hash::name() %
                         types::string::name() %
                         kvp.value().infer_type()
                        ).str()
                    );
                }
            }

            // Ensure the value is a string
            if (!kvp.value().as<string>()) {
                throw format_exception(
                    (boost::format("expected %1% for hash value but found %2%.") %
                     types::string::name() %
                     kvp.value().infer_type()
                    ).str()
                );
            }

            _formats.emplace_back(kvp.key().require<values::type>(), format{ *kvp.value().as<string>() });
        }

        types::recursion_guard guard;
        sort(_formats.begin(), _formats.end(), [&](auto const& left, auto const& right) {
            auto& left_type = left.first;
            auto& right_type = right.first;

            bool left_assignable = left_type.is_assignable(right_type, guard);
            bool right_assignable = right_type.is_assignable(left_type, guard);

            if (left_assignable && !right_assignable) {
                return false;
            }
            if (!left_assignable && right_assignable) {
                return true;
            }

            // Types are equal or disjoint, do a rank sort based on type
            return get_type_rank(left_type) < get_type_rank(right_type);
        });
    }

    format_map::format_map(string const& format)
    {
        _formats.emplace_back(types::any{}, utility::format{ format });
    }

    format const* format_map::find_format(values::value const& value) const
    {
        // The types are sorted in the order from most specific to least specific
        // Therefore, just look for the first entry that the value is an instance of
        types::recursion_guard guard;
        for (auto& entry : _formats) {
            if (entry.first.is_instance(value, guard)) {
                return &entry.second;
            }
        }
        return nullptr;
    }

}}}  // namespace puppet::runtime::utility
