#include <puppet/runtime/values/value.hpp>
#include <puppet/compiler/evaluation/collectors/collector.hpp>
#include <puppet/utility/indirect_collection.hpp>
#include <puppet/unicode/string.hpp>
#include <puppet/cast.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <rapidjson/document.h>
#include <algorithm>

using namespace std;
using namespace rapidjson;
using namespace puppet::runtime;
using namespace puppet::compiler::evaluation;
using namespace PuppetRubyHost;

namespace puppet { namespace runtime { namespace values {

    value::value(Protocols::Value const& value)
    {
        switch (value.kind_case()) {
            case Protocols::Value::kSymbol:
                if (value.symbol() == Protocols::Value_Symbol_UNDEF) {
                    *this = values::undef{};
                } else if (value.symbol() == Protocols::Value_Symbol_DEFAULT) {
                    *this = values::defaulted{};
                } else {
                    throw runtime_error("unexpected symbolic enumeration.");
                }
                break;

            case Protocols::Value::kInteger:
                *this = value.integer();
                break;

            case Protocols::Value::kFloat:
                *this = value.float_();
                break;

            case Protocols::Value::kBoolean:
                *this = value.boolean();
                break;

            case Protocols::Value::kString:
                *this = value.string();
                break;

            case Protocols::Value::kRegexp:
                *this = values::regex{ value.regexp() };
                break;

            case Protocols::Value::kType:
                // TODO: parse type!
                break;

            case Protocols::Value::kArray: {
                values::array array;
                for (auto const& element : value.array().elements()) {
                    array.emplace_back(values::value{ element });
                }
                *this = rvalue_cast(array);
                break;
            }

            case Protocols::Value::kHash: {
                values::hash hash;
                for (auto const& kvp : value.hash().elements()) {
                    hash.set(values::value{ kvp.key() }, values::value{ kvp.value() });
                }
                *this = rvalue_cast(hash);
                break;
            }

            default:
                throw runtime_error("unexpected value kind.");
        }
    }

    value::value(values::wrapper<value>&& wrapper) :
        value_base(rvalue_cast(static_cast<value_base&>(wrapper.get())))
    {
    }

    value::value(char const* string) :
        value_base(std::string(string))
    {
    }

    value& value::operator=(values::wrapper<value>&& wrapper)
    {
        value_base::operator=(rvalue_cast(static_cast<value_base&>(wrapper.get())));
        return *this;
    }

    value& value::operator=(char const* string)
    {
        value_base::operator=(std::string(string));
        return *this;
    }

    bool value::is_undef() const
    {
        return static_cast<bool>(as<undef>());
    }

    bool value::is_default() const
    {
        return static_cast<bool>(as<defaulted>());
    }

    bool value::is_true() const
    {
        auto ptr = as<bool>();
        return ptr && *ptr;
    }

    bool value::is_false() const
    {
        auto ptr = as<bool>();
        return ptr && !*ptr;
    }

    bool value::is_truthy() const
    {
        if (is_undef()) {
            return false;
        }
        if (auto ptr = as<bool>()) {
            return *ptr;
        }
        return true;
    }

    bool value::match(compiler::evaluation::context& context, value const& other) const
    {
        if (is_default()) {
            return true;
        }
        if (auto regex = as<values::regex>()) {
            if (auto string = other.as<std::string>()) {
                return regex->match(context, *string);
            }
            return false;
        }
        if (auto array = as<values::array>()) {
            if (auto other_array = other.as<values::array>()) {
                if (array->size() == other_array->size()) {
                    for (size_t i = 0; i < array->size(); ++i) {
                        if (!(*array)[i]->match(context, (*other_array)[i])) {
                            return false;
                        }
                    }
                    return true;
                }
            }
            return false;
        }
        if (auto hash = as<values::hash>()) {
            if (auto other_hash = other.as<values::hash>()) {
                for (auto const& element : *hash) {
                    auto other_value = other_hash->get(element.key());
                    if (!other_value || !element.value().match(context, *other_value)) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        }
        if (auto type = as<values::type>()) {
            runtime::types::recursion_guard guard;
            return type->is_instance(other, guard);
        }

        // Otherwise, use equals
        return *this == other;
    }

    struct type_inference_visitor : boost::static_visitor<values::type>
    {
        explicit type_inference_visitor(bool detailed) :
            _detailed(detailed)
        {
        }

        result_type operator()(undef const&)
        {
            return types::undef{};
        }

        result_type operator()(defaulted const&)
        {
            return types::defaulted{};
        }

        result_type operator()(int64_t value)
        {
            return types::integer{ value, value };
        }

        result_type operator()(double value)
        {
            return types::floating{ value, value };
        }

        result_type operator()(bool)
        {
            return types::boolean{};
        }

        result_type operator()(string const& value)
        {
            unicode::string string{ value };
            return types::string{
                static_cast<int64_t>(string.graphemes()),
                static_cast<int64_t>(string.graphemes())
            };
        }

        result_type operator()(values::regex const& value)
        {
            return types::regexp{ value.pattern() };
        }

        result_type operator()(type const& t)
        {
            return types::type{ make_unique<type>(t) };
        }

        result_type operator()(variable const& var)
        {
            return boost::apply_visitor(*this, var.value());
        }

        result_type operator()(array const& value)
        {
            if (value.empty()) {
                return types::array{ nullptr, 0, 0 };
            }

            if (_detailed) {
                return infer_detailed_array(value);
            }

            bool first = true;
            result_type element_type;
            for (auto& element : value) {
                if (first) {
                    element_type = boost::apply_visitor(*this, *element);
                    first = false;
                    continue;
                }

                element_type = infer_common_type(boost::apply_visitor(*this, *element), element_type);
            }
            return types::array{
                make_unique<type>(rvalue_cast(element_type)),
                static_cast<int64_t>(value.size()),
                static_cast<int64_t>(value.size())
            };
        }

        result_type operator()(hash const& value)
        {
            if (value.empty()) {
                return types::hash{ nullptr, nullptr, 0, 0 };
            }

            if (_detailed) {
                return infer_detailed_hash(value);
            }

            bool first = true;
            result_type key_type;
            result_type value_type;
            for (auto& kvp : value) {
                if (first) {
                    key_type = boost::apply_visitor(*this, kvp.key());
                    value_type = boost::apply_visitor(*this, kvp.value());
                    first = false;
                    continue;
                }
                key_type = infer_common_type(boost::apply_visitor(*this, kvp.key()), key_type);
                value_type = infer_common_type(boost::apply_visitor(*this, kvp.value()), value_type);
            }
            return types::hash{
                make_unique<type>(rvalue_cast(key_type)),
                make_unique<type>(rvalue_cast(value_type)),
                static_cast<int64_t>(value.size()),
                static_cast<int64_t>(value.size())
            };
        }

        result_type operator()(iterator const& value)
        {
            return types::iterator{ make_unique<type>(value.infer_produced_type()) };
        }

     private:
        result_type infer_detailed_array(values::array const& array)
        {
            vector<unique_ptr<values::type>> elements;
            for (auto& element : array) {
                elements.emplace_back(make_unique<values::type>(boost::apply_visitor(*this, *element)));
            }
            return runtime::types::tuple{
                rvalue_cast(elements),
                static_cast<int64_t>(array.size()),
                static_cast<int64_t>(array.size())
            };
        }

        result_type infer_detailed_hash(values::hash const& hash)
        {
            // If all keys are String, return a Struct
            if (all_of(hash.begin(), hash.end(), [&](auto const& kvp) {
                return kvp.key().template as<std::string>();
            })) {
                runtime::types::structure::schema_type schema;

                for (auto& kvp : hash) {
                    schema.emplace_back(
                        make_unique<values::type>(runtime::types::enumeration{ { *kvp.key().as<std::string>() } }),
                        make_unique<values::type>(boost::apply_visitor(*this, kvp.value()))
                    );
                }
                return runtime::types::structure{ rvalue_cast(schema) };
            }

            // At least one key is not a string so build up a list of key and value types
            vector<unique_ptr<values::type>> key_types;
            vector<unique_ptr<values::type>> value_types;
            for (auto& kvp : hash) {
                key_types.emplace_back(make_unique<values::type>(boost::apply_visitor(*this, kvp.key())));
                value_types.emplace_back(make_unique<values::type>(boost::apply_visitor(*this, kvp.value())));
            }

            return runtime::types::hash{
                make_unique<values::type>(runtime::types::variant{ rvalue_cast(key_types) }.unwrap()),
                make_unique<values::type>(runtime::types::variant{ rvalue_cast(value_types) }.unwrap()),
                static_cast<int64_t>(hash.size()),
                static_cast<int64_t>(hash.size())
            };
        }

        result_type infer_common_type(result_type const& left, result_type const& right)
        {
            // Check for right is assignable to left and vice versa
            if (left.is_assignable(right, _guard)) {
                return left;
            }
            if (right.is_assignable(left, _guard)) {
                return right;
            }
            // Check for both Array
            if (auto left_array = boost::get<types::array>(&left)) {
                if (auto right_array = boost::get<types::array>(&right)) {
                    return types::array{
                        make_unique<values::type>(infer_common_type(left_array->element_type(), right_array->element_type()))
                    };
                }
            }
            // Check for both Hash
            if (auto left_hash = boost::get<types::hash>(&left)) {
                if (auto right_hash = boost::get<types::hash>(&right)) {
                    return types::hash{
                        make_unique<values::type>(infer_common_type(left_hash->key_type(), right_hash->key_type())),
                        make_unique<values::type>(infer_common_type(left_hash->value_type(), right_hash->value_type()))
                    };
                }
            }
            // Check for both Class
            if (boost::get<types::klass>(&left) && boost::get<types::klass>(&right)) {
                // Unparameterized Class is the common type because neither was assignable above
                return types::klass{};
            }
            // Check for both Resource
            if (auto left_resource = boost::get<types::resource>(&left)) {
                if (auto right_resource = boost::get<types::resource>(&right)) {
                    if (left_resource->type_name() == right_resource->type_name()) {
                        return types::resource{ left_resource->type_name() };
                    }
                    // Unparameterized Resource is the common type because neither was assignable above
                    return types::resource{};
                }
            }
            // Check for both Integer
            if (auto left_integer = boost::get<types::integer>(&left)) {
                if (auto right_integer = boost::get<types::integer>(&right)) {
                    return types::integer{
                        std::min(left_integer->from(), right_integer->from()),
                        std::max(left_integer->to(), right_integer->to())
                    };
                }
            }
            // Check for both Float
            if (auto left_float = boost::get<types::floating>(&left)) {
                if (auto right_float = boost::get<types::floating>(&right)) {
                    return types::floating{
                        std::min(left_float->from(), right_float->from()),
                        std::max(left_float->to(), right_float->to())
                    };
                }
            }
            // Check for both String
            if (auto left_string = boost::get<types::string>(&left)) {
                if (auto right_string = boost::get<types::string>(&right)) {
                    return types::string{
                        std::min(left_string->from(), right_string->from()),
                        std::max(left_string->to(), right_string->to())
                    };
                }
            }
            // Check for both Pattern
            if (auto left_pattern = boost::get<types::pattern>(&left)) {
                if (auto right_pattern = boost::get<types::pattern>(&right)) {
                    return types::pattern{ join_sets<values::regex>(left_pattern->patterns(), right_pattern->patterns()) };
                }
            }
            // Check for both Enum
            if (auto left_enum = boost::get<types::enumeration>(&left)) {
                if (auto right_enum = boost::get<types::enumeration>(&right)) {
                    return types::enumeration{ join_sets<std::string>(left_enum->strings(), right_enum->strings())};
                }
            }
            // Check for both Variant
            if (auto left_variant = boost::get<types::variant>(&left)) {
                if (auto right_variant = boost::get<types::variant>(&right)) {
                    return types::variant{ join_sets<values::type>(left_variant->types(), right_variant->types()) };
                }
            }
            // Check for both Type
            if (auto left_type = boost::get<types::type>(&left)) {
                if (auto right_type = boost::get<types::type>(&right)) {
                    if (!left_type->parameter() || !right_type->parameter()) {
                        return types::type{};
                    }
                    return types::type{
                        make_unique<values::type>(infer_common_type(*left_type->parameter(), *right_type->parameter()))
                    };
                }
            }
            // Check for both Regexp
            if (boost::get<types::regexp>(&left) && boost::get<types::regexp>(&right)) {
                // Unparameterized Regexp is the common type because neither was assignable above
                return types::regexp{};
            }
            // Check for both Callable
            if (boost::get<types::callable>(&left) && boost::get<types::callable>(&right)) {
                // Unparameterized Callable is the common type because neither was assignable above
                return types::callable{};
            }
            // Check for both Runtime
            if (boost::get<types::runtime>(&left) && boost::get<types::runtime>(&right)) {
                // Unparameterized Runtime is the common type because neither was assignable above
                return types::runtime{};
            }
            // Check fror both Numeric
            if (types::numeric::instance.is_assignable(left, _guard) && types::numeric::instance.is_assignable(right, _guard)) {
                return types::numeric{};
            }
            // Check fror both Scalar
            if (types::scalar::instance.is_assignable(left, _guard) && types::scalar::instance.is_assignable(right, _guard)) {
                return types::scalar{};
            }
            // Check fror both Data
            if (types::data::instance.is_assignable(left, _guard) && types::data::instance.is_assignable(right, _guard)) {
                return types::data{};
            }

            // None of the above, return Any
            return types::any{};
        }

        template <typename T>
        static set<T> join_sets(set<T> const& left, set<T> const& right)
        {
            set<T> result;
            for (auto& element : left) {
                result.insert(element);
            }
            for (auto& element : right) {
                result.insert(element);
            }
            return result;
        }

        template <typename T>
        static vector<T> join_sets(vector<T> const& left, vector<T> const& right)
        {
            vector<T> result;
            utility::indirect_set<T> set;
            for (auto& element : left) {
                if (set.insert(&element).second) {
                    result.push_back(element);
                }
            }
            for (auto& element : right) {
                if (set.insert(&element).second) {
                    result.push_back(element);
                }
            }
            return result;
        }

        template <typename T>
        static vector<unique_ptr<T>> join_sets(vector<unique_ptr<T>> const& left, vector<unique_ptr<T>> const& right)
        {
            vector<unique_ptr<T>> result;
            utility::indirect_set<T> set;
            for (auto& element : left) {
                if (set.insert(element.get()).second) {
                    result.push_back(make_unique<T>(*element));
                }
            }
            for (auto& element : right) {
                if (set.insert(element.get()).second) {
                    result.push_back(make_unique<T>(*element));
                }
            }
            return result;
        }

        runtime::types::recursion_guard _guard;
        bool _detailed;
    };

    values::type value::infer_type(bool detailed) const
    {
        type_inference_visitor visitor{ detailed };
        return boost::apply_visitor(visitor, *this);
    }

    array value::to_array(bool convert_hash)
    {
        // If already an array, return it
        if (as<values::array>()) {
            return move_as<values::array>();
        }

        array result;

        // Check for hash
        auto hash_ptr = as<values::hash>();
        if (convert_hash && hash_ptr) {
            // Turn the hash into an array of [K,V]
            for (auto& kvp : *hash_ptr) {
                array element;
                element.emplace_back(kvp.key());
                element.emplace_back(kvp.value());
                result.emplace_back(rvalue_cast(element));
            }
        } else if (auto iterator = as<values::iterator>()) {
            // Copy the iteration into the result
            iterator->each([&](auto const* key, auto const& value) {
                if (key) {
                    array kvp(2);
                    kvp[0] = *key;
                    kvp[1] = value;
                    result.emplace_back(rvalue_cast(kvp));
                } else {
                    result.emplace_back(value);
                }
                return true;
            });
        }
        else if (!is_undef()) {
            // Otherwise, add the value as the only element
            result.emplace_back(rvalue_cast(*this));
        }
        return result;
    }

    struct protocol_visitor : boost::static_visitor<void>
    {
        explicit protocol_visitor(Protocols::Value& value) :
            _value(value)
        {
        }

        void operator()(undef const&) const
        {
            _value.set_symbol(Protocols::Value_Symbol_UNDEF);
        }

        void operator()(defaulted const&) const
        {
            _value.set_symbol(Protocols::Value_Symbol_DEFAULT);
        }

        void operator()(int64_t value) const
        {
            _value.set_integer(value);
        }

        void operator()(double value) const
        {
            _value.set_float_(value);
        }

        void operator()(bool value) const
        {
            _value.set_boolean(value);
        }

        void operator()(string const& value) const
        {
            _value.set_string(value);
        }

        void operator()(values::regex const& value) const
        {
            _value.set_regexp(value.pattern());
        }

        void operator()(values::type const& type) const
        {
            _value.set_type(boost::lexical_cast<string>(type).c_str());
        }

        void operator()(values::variable const& value) const
        {
            boost::apply_visitor(*this, value.value());
        }

        void operator()(values::array const& value) const
        {
            auto array = _value.mutable_array();

            for (auto const& element : value) {
                auto e = array->add_elements();
                boost::apply_visitor(protocol_visitor{ *e }, *element);
            }
        }

        void operator()(values::hash const& value) const
        {
            auto hash = _value.mutable_hash();

            for (auto const& element : value) {
                auto e = hash->add_elements();

                auto k = e->mutable_key();
                boost::apply_visitor(protocol_visitor{ *k }, element.key());

                auto v = e->mutable_value();
                boost::apply_visitor(protocol_visitor{ *v }, element.value());
            }
        }

        void operator()(values::iterator const& value) const
        {
            auto array = _value.mutable_array();

            value.each([&](auto const* key, auto const& value) {
                auto e = array->add_elements();
                if (key) {
                    values::array subarray;
                    subarray.push_back(*key);
                    subarray.push_back(value);
                    protocol_visitor{ *e }(subarray);
                } else {
                    boost::apply_visitor(protocol_visitor{ *e }, value);
                }
                return true;
            });
        }

     private:
        Protocols::Value& _value;
    };

    void value::to_protocol_value(Protocols::Value& value) const
    {
        boost::apply_visitor(protocol_visitor{ value }, *this);
    }

    struct value_printer : boost::static_visitor<ostream&>
    {
        explicit value_printer(ostream& os) :
            _os(os)
        {
        }

        result_type operator()(bool value) const
        {
            _os << (value ? "true" : "false");
            return _os;
        }

        result_type operator()(values::type const& type) const
        {
            // Don't expand when printing out a value that is a type (display aliases as just the name)
            type.write(_os, false);
            return _os;
        }

        template <typename T>
        result_type operator()(T const& value) const
        {
            _os << value;
            return _os;
        }

    private:
        ostream& _os;
    };

    ostream& operator<<(ostream& os, value const& val)
    {
        return boost::apply_visitor(value_printer(os), val);
    }

    bool equality_visitor::operator()(std::string const& left, std::string const& right) const
    {
        // Build the unicode string depending on which is shorter (faster "invariant" check)
        // Note: this is not a case insensitive check
        // String equality is case sensitive; Puppet's binary '==' operator is case insensitive
        if (left.size() < right.size()) {
            return unicode::string{ left } == right;
        }
        return unicode::string{ right } == left;
    }

    bool operator==(value const& left, value const& right)
    {
        return boost::apply_visitor(equality_visitor(), left, right);
    }

    bool operator!=(value const& left, value const& right)
    {
        return !boost::apply_visitor(equality_visitor(), left, right);
    }

    size_t hash_value(values::value const& value)
    {
        // If a string, hash using unicode::string to handle Unicode normalization
        if (auto ptr = value.as<std::string>()) {
            unicode::string string{ *ptr };
            return hash_value(string);
        }
        return hash_value(static_cast<value_base const&>(value));
    }

    void value::each_resource(function<void(runtime::types::resource const&)> const& callback, function<void(string const&)> const& error) const
    {
        namespace pt = puppet::runtime::types;

        // Check for string, type, or array
        if (auto str = as<string>()) {
            auto resource = pt::resource::parse(*str);
            if (!resource) {
                if (error) {
                    error((boost::format("expected a resource string but found \"%1%\".") % *str).str());
                }
                return;
            }
            callback(*resource);
            return;
        } else if (auto type = as<values::type>()) {
            // Check for a resource or klass type
            if (auto resource = boost::get<pt::resource>(type)) {
                if (resource->fully_qualified()) {
                    callback(*resource);
                    return;
                }
            } else if (auto klass = boost::get<pt::klass>(type)) {
                if (!klass->class_name().empty()) {
                    callback(runtime::types::resource("class", klass->class_name()));
                    return;
                }
            } else if (auto runtime = boost::get<pt::runtime>(type)) {
                // Check for a collector
                if (runtime->object()) {
                    if (auto collector = boost::get<shared_ptr<collectors::collector>>(*runtime->object())) {
                        for (auto resource : collector->resources()) {
                            callback(resource->type());
                        }
                        return;
                    }
                }
            }
        } else if (auto array = as<values::array>()) {
            // For arrays, recurse on each element
            for (auto& element : *array) {
                element->each_resource(callback, error);
            }
            return;
        }

        if (error) {
            error((boost::format("expected %1% or fully qualified %2% for relationship but found %3%.") %
                pt::string::name() %
                pt::resource::name() %
                infer_type()
            ).str());
        }
    }

    struct json_visitor : boost::static_visitor<json_value>
    {
        explicit json_visitor(json_allocator& allocator) :
            _allocator(allocator)
        {
        }

        result_type operator()(undef const&) const
        {
            json_value value;
            value.SetNull();
            return value;
        }

        result_type operator()(defaulted const&) const
        {
            json_value value;
            value.SetString("default");
            return value;
        }

        result_type operator()(int64_t i) const
        {
            json_value value;
            value.SetInt64(i);
            return value;
        }

        result_type operator()(double d) const
        {
            json_value value;
            value.SetDouble(static_cast<double>(d));
            return value;
        }

        result_type operator()(bool b) const
        {
            json_value value;
            value.SetBool(b);
            return value;
        }

        result_type operator()(string const& s) const
        {
            json_value value;
            value.SetString(StringRef(s.c_str(), s.size()));
            return value;
        }

        result_type operator()(values::regex const& regex) const
        {
            auto const& pattern = regex.pattern();
            json_value value;
            value.SetString(StringRef(pattern.c_str(), pattern.size()));
            return value;
        }

        result_type operator()(values::type const& type) const
        {
            json_value value;
            value.SetString(boost::lexical_cast<string>(type).c_str(), _allocator);
            return value;
        }

        result_type operator()(values::variable const& variable) const
        {
            return boost::apply_visitor(*this, variable.value());
        }

        result_type operator()(values::array const& array) const
        {
            json_value value;
            value.SetArray();
            value.Reserve(array.size(), _allocator);

            for (auto const& element : array) {
                value.PushBack(boost::apply_visitor(*this, *element), _allocator);
            }
            return value;
        }

        result_type operator()(values::hash const& hash) const
        {
            json_value value;
            value.SetObject();

            for (auto const& kvp : hash) {
                value.AddMember(
                    json_value(boost::lexical_cast<string>(kvp.key()).c_str(), _allocator),
                    boost::apply_visitor(*this, kvp.value()),
                    _allocator
                );
            }
            return value;
        }

        result_type operator()(values::iterator const& iterator) const
        {
            json_value result;

            bool is_hash = iterator.value().as<values::hash>();

            if (is_hash) {
                result.SetObject();
            } else {
                result.SetArray();
            }

            // Copy the iteration into the result
            iterator.each([&](auto const* key, auto const& value) {
                if (key) {
                    result.AddMember(
                        json_value(boost::lexical_cast<string>(*key).c_str(), _allocator),
                        boost::apply_visitor(*this, value),
                        _allocator
                    );
                } else {
                    result.PushBack(boost::apply_visitor(*this, value), _allocator);
                }
                return true;
            });
            return result;
        }

     private:
        json_allocator& _allocator;
    };

    json_value value::to_json(json_allocator& allocator) const
    {
        return boost::apply_visitor(json_visitor(allocator), *this);
    }

}}}  // namespace puppet::runtime::values
