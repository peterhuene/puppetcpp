#include <puppet/runtime/values/value.hpp>
#include <puppet/compiler/registry.hpp>
#include <puppet/cast.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace puppet { namespace runtime { namespace types {

    klass const klass::instance;

    klass::klass(std::string name) :
        _name(rvalue_cast(name))
    {
        compiler::registry::normalize(_name);
    }

    std::string const& klass::class_name() const
    {
        return _name;
    }

    bool klass::fully_qualified() const
    {
        return !_name.empty();
    }

    char const* klass::name()
    {
        return "Class";
    }

    values::type klass::generalize() const
    {
        return *this;
    }

    bool klass::is_instance(values::value const& value, recursion_guard& guard) const
    {
        auto ptr = value.as<values::type>();
        if (!ptr) {
            return false;
        }
        auto class_ptr = boost::get<klass>(ptr);
        if (!class_ptr) {
            return false;
        }
        return _name.empty() || _name == class_ptr->class_name();
    }

    bool klass::is_assignable(values::type const& other, recursion_guard& guard) const
    {
        auto ptr = boost::get<klass>(&other);
        if (!ptr) {
            return false;
        }
        return _name.empty() || _name == ptr->class_name();
    }

    void klass::write(ostream& stream, bool expand) const
    {
        stream << klass::name();
        if (_name.empty()) {
            return;
        }
        stream << "[" << _name << "]";
    }

    ostream& operator<<(ostream& os, klass const& type)
    {
        type.write(os);
        return os;
    }

    bool operator==(klass const& left, klass const& right)
    {
        return left.class_name() == right.class_name();
    }

    bool operator!=(klass const& left, klass const& right)
    {
        return !(left == right);
    }

    size_t hash_value(klass const& type)
    {
        static const size_t name_hash = boost::hash_value(klass::name());

        size_t seed = 0;
        boost::hash_combine(seed, name_hash);
        boost::hash_combine(seed, type.class_name());
        return seed;
    }

}}}  // namespace puppet::runtime::types
