#include <puppet/runtime/values/value.hpp>
#include <puppet/cast.hpp>
#include <boost/functional/hash.hpp>

using namespace std;

namespace puppet { namespace runtime { namespace types {

    struct type_name_visitor : boost::static_visitor<std::string>
    {
        result_type operator()(shared_ptr<compiler::evaluation::collectors::collector> const&) const
        {
            return "Collector";
        }
    };

    runtime::runtime(std::string runtime_name, std::string type_name) :
        _runtime_name(rvalue_cast(runtime_name)),
        _type_name(rvalue_cast(type_name))
    {
    }

    runtime::runtime(boost::optional<object_type> object) :
        _runtime_name("C++"),
        _object(rvalue_cast(object))
    {
        if (_object) {
            _type_name = boost::apply_visitor(type_name_visitor(), *object);
        }
    }

    std::string const& runtime::runtime_name() const
    {
        return _runtime_name;
    }

    std::string const& runtime::type_name() const
    {
        return _type_name;
    }

    boost::optional<runtime::object_type> const& runtime::object() const
    {
        return _object;
    }

    char const* runtime::name()
    {
        return "Runtime";
    }

    values::type runtime::generalize() const
    {
        return *this;
    }

    bool runtime::is_instance(values::value const& value, recursion_guard& guard) const
    {
        // Check for type
        auto type = value.as<values::type>();
        if (!type) {
            return false;
        }
        // Check for runtime type
        auto runtime = boost::get<types::runtime>(type);
        if (!runtime) {
            return false;
        }
        // If no runtime specified, then the value is a "runtime"
        if (_runtime_name.empty()) {
            return true;
        }
        // Check runtime name for equality
        if (_runtime_name != runtime->runtime_name()) {
            return false;
        }
        // Check type name for equality
        return _type_name.empty() || _type_name == runtime->type_name();
    }

    bool runtime::is_assignable(values::type const& other, recursion_guard& guard) const
    {
        auto runtime = boost::get<types::runtime>(&other);
        if (!runtime) {
            return false;
        }
        return _runtime_name.empty() || (_runtime_name == runtime->runtime_name() && (_type_name.empty() || _type_name == runtime->type_name()));
    }

    void runtime::write(ostream& stream, bool expand) const
    {
        stream << runtime::name();
        if (_runtime_name.empty()) {
            return;
        }
        stream << "['" << _runtime_name;
        if (!_type_name.empty()) {
            stream << "', '" << _type_name;
        }
        stream << "']";
    }

    ostream& operator<<(ostream& os, runtime const& type)
    {
        type.write(os);
        return os;
    }

    bool operator==(runtime const& left, runtime const& right)
    {
        return left.runtime_name() == right.runtime_name() && left.type_name() == right.type_name();
    }

    bool operator!=(runtime const& left, runtime const& right)
    {
        return !(left == right);
    }

    size_t hash_value(runtime const& type)
    {
        static const size_t name_hash = boost::hash_value(runtime::name());

        size_t seed = 0;
        boost::hash_combine(seed, name_hash);
        boost::hash_combine(seed, type.runtime_name());
        boost::hash_combine(seed, type.type_name());
        if (type.object()) {
            boost::hash_combine(seed, *type.object());
        }
        return seed;
    }

}}}  // namespace puppet::runtime::types
