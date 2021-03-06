#include <puppet/compiler/evaluation/collectors/collector.hpp>
#include <puppet/compiler/evaluation/context.hpp>
#include <puppet/cast.hpp>

using namespace std;
using namespace puppet::runtime;

namespace puppet { namespace compiler { namespace evaluation { namespace collectors {

    void collector::detect_uncollected(evaluation::context& context) const
    {
    }

    vector<resource*> const& collector::resources() const
    {
        return _resources;
    }

    void collector::attributes(compiler::attributes attributes)
    {
        _attributes = rvalue_cast(attributes);
    }

    void collector::collect_resource(evaluation::context& context, compiler::resource& resource, bool check)
    {
        auto& catalog = context.catalog();

        // Realize the resource
        catalog.realize(resource);

        // Set the attributes
        for (auto& pair : _attributes) {
            switch (pair.first) {
                case ast::attribute_operator::assignment:
                    resource.set(pair.second);
                    break;

                case ast::attribute_operator::append:
                    resource.append(pair.second);
                    break;

                default:
                    throw runtime_error("unexpected attribute operator");
            }
        }

        // Check if the resource is already in the list
        if (check && find(_resources.begin(), _resources.end(), &resource) != _resources.end()) {
            return;
        }

        // Add to the list
        _resources.push_back(&resource);
    }

}}}}  // namespace puppet::compiler::evaluation::collectors
