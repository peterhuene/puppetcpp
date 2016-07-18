#include <puppet/compiler/environment.hpp>
#include <puppet/compiler/parser/parser.hpp>
#include <puppet/compiler/scanner.hpp>
#include <puppet/compiler/node.hpp>
#include <puppet/compiler/exceptions.hpp>
#include <puppet/logging/logger.hpp>
#include <puppet/utility/filesystem/helpers.hpp>
#include <puppet/cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace puppet::runtime;
using namespace puppet::compiler::evaluation;
using namespace puppet::utility::filesystem;
namespace fs = boost::filesystem;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace puppet { namespace compiler {

    static void load_environment_settings(logging::logger& logger, string const& directory, compiler::settings& settings)
    {
        static char const* const CONFIGURATION_FILE = "environment.conf";

        string config_file_path = (fs::path{ directory } / CONFIGURATION_FILE).string();
        sys::error_code ec;
        if (!fs::is_regular_file(config_file_path, ec)) {
            LOG(debug, "environment configuration file '%1%' was not found.", config_file_path);
            return;
        }

        LOG(debug, "loading environment settings from '%1%'.", config_file_path);

        try {
            // Read the options from the config file
            po::options_description description("");
            description.add_options()
                (settings::module_path.c_str(), po::value<string>(), "")
                (settings::manifest.c_str(),    po::value<string>(), "");
            po::variables_map vm;
            po::store(po::parse_config_file<char>(config_file_path.c_str(), description, true), vm);
            po::notify(vm);

            if (vm.count(settings::module_path)) {
                auto module_path = vm[settings::module_path].as<string>();
                LOG(debug, "using module path '%1%' from environment configuration file.", module_path);
                settings.set(settings::module_path, rvalue_cast(module_path));
            }
            if (vm.count(settings::manifest)) {
                auto manifest = vm[settings::manifest].as<string>();
                LOG(debug, "using main manifest '%1%' from environment configuration file.", manifest);
                settings.set(settings::manifest, rvalue_cast(manifest));
            }
        } catch (po::error const& ex) {
            throw compilation_exception(
                (boost::format("failed to read environment configuration file '%1%': %2%.") %
                 config_file_path %
                 ex.what()
                ).str()
            );
        }
    }

    shared_ptr<environment> environment::create(logging::logger& logger, compiler::settings settings, shared_ptr<grpc::ChannelInterface> channel)
    {
        // Get the name from the settings
        string name = boost::lexical_cast<string>(settings.get(settings::environment, false));
        if (name.empty()) {
            throw compilation_exception("cannot create an environment with an empty name.");
        }

        // Search for the environment's directory
        string base_directory;
        auto environment_path = settings.get(settings::environment_path);
        LOG(debug, "searching for environment '%1%' using environment path '%2%'.", name, environment_path);
        if (environment_path.as<string>()) {
            boost::split_iterator<string::const_iterator> end;
            for (auto it = boost::make_split_iterator(*environment_path.as<string>(), boost::first_finder(path_separator(), boost::is_equal())); it != end; ++it) {
                if (!*it) {
                    continue;
                }

                auto path = fs::path{ make_absolute({ it->begin(), it->end() }) } / name;

                sys::error_code ec;
                if (fs::is_directory(path, ec)) {
                    base_directory = path.string();
                    break;
                }
            }
        }
        if (base_directory.empty()) {
            throw compilation_exception(
                (boost::format("could not locate an environment directory for environment '%1%' using search path '%2%'.") %
                 name %
                 environment_path
                ).str());
        }

        LOG(debug, "found environment directory '%1%' for environment '%2%'.", base_directory, name);

        struct make_shared_enabler : environment
        {
            explicit make_shared_enabler(string name, string base, compiler::settings settings, shared_ptr<grpc::ChannelInterface> channel) :
                environment(rvalue_cast(name), rvalue_cast(base), rvalue_cast(settings), rvalue_cast(channel))
            {
            }
        };

        // Load the environment settings
        load_environment_settings(logger, base_directory, settings);

        auto environment = make_shared<make_shared_enabler>(
            rvalue_cast(name),
            rvalue_cast(base_directory),
            rvalue_cast(settings),
            rvalue_cast(channel)
        );
        environment->add_modules(logger);
        return environment;
    }

    string const& environment::name() const
    {
        return _name;
    }

    compiler::settings const& environment::settings() const
    {
        return _settings;
    }

    deque<module> const& environment::modules() const
    {
        return _modules;
    }

    module const* environment::find_module(string const& name) const
    {
        auto it = _module_map.find(name);
        if (it == _module_map.end()) {
            return nullptr;
        }
        return it->second;
    }

    void environment::each_module(function<bool(module const&)> const& callback) const
    {
        for (auto& module : _modules) {
            if (!callback(module)) {
                return;
            }
        }
    }

    void environment::register_builtins()
    {
        _registry.register_builtins();
    }

    vector<shared_ptr<ast::syntax_tree>> environment::import_initial_manifests(logging::logger& logger)
    {
        lock_guard<mutex> lock(_mutex);

        if (!_initial_manifests.empty()) {
            return _initial_manifests;
        }

        each_file(find_type::manifest, [&](auto const& manifest) {
            _initial_manifests.emplace_back(this->import(logger, manifest));
            return true;
        });
        return _initial_manifests;
    }

    shared_ptr<ast::syntax_tree> environment::import_manifest(logging::logger& logger, string const& path)
    {
        lock_guard<mutex> lock(_mutex);
        return import(logger, path);
    }

    shared_ptr<ast::syntax_tree> environment::import_source(logging::logger& logger, string source, string path)
    {
        auto tree = parser::parse_string(logger, rvalue_cast(source), rvalue_cast(path));

        tree->validate();

        compiler::scanner scanner{ logger, _name, _registry };
        if (scanner.scan(*tree)) {
            // The tree contained a definition, so treat it as part of the initial manifests for the environment
            _initial_manifests.push_back(tree);
        }
        return tree;
    }

    compiler::klass const* environment::find_class(logging::logger& logger, string const& name)
    {
        lock_guard<mutex> lock(_mutex);

        if (auto klass = _registry.find_class(name)) {
            return klass;
        }

        LOG(debug, "attempting import of class '%1%' into environment '%2%'.", name, _name);

        string path;
        compiler::module const* module = nullptr;
        tie(path, module) = resolve_name(logger, name, find_type::manifest);

        if (path.empty()) {
            return nullptr;
        }

        import(logger, path, module);
        return _registry.find_class(name);
    }

    compiler::defined_type const* environment::find_defined_type(logging::logger& logger, string const& name)
    {
        lock_guard<mutex> lock(_mutex);

        if (auto type = _registry.find_defined_type(name)) {
            return type;
        }

        LOG(debug, "attempting import of defined type '%1%' into environment '%2%'.", name, _name);

        string path;
        compiler::module const* module = nullptr;
        tie(path, module) = resolve_name(logger, name, find_type::manifest);

        if (path.empty()) {
            return nullptr;
        }

        import(logger, path, module);
        return _registry.find_defined_type(name);
    }

    functions::descriptor const* environment::find_function(logging::logger& logger, string const& name, ast::context const& context)
    {
        lock_guard<mutex> lock(_mutex);

        if (auto descriptor = _registry.find_function(name)) {
            return descriptor;
        }

        LOG(debug, "attempting import of function '%1%' into environment '%2%'.", name, _name);

        string path;
        compiler::module const* module = nullptr;
        tie(path, module) = resolve_name(logger, name, find_type::function);

        // Attempt an import of a Puppet function
        if (!path.empty()) {
            import(logger, path, module);
            if (auto descriptor = _registry.find_function(name)) {
                return descriptor;
            }
        }

        // Attempt an import of a function implemented in ruby
        return _registry.import_ruby_function(_name, name, context);
    }

    operators::binary::descriptor const* environment::find_binary_operator(ast::binary_operator oper) const
    {
        // Currently binary operators cannot be defined in Puppet
        // Therefore, only built-ins are supported and this function does not need to be made thread safe
        return _registry.find_binary_operator(oper);
    }

    operators::unary::descriptor const* environment::find_unary_operator(ast::unary_operator oper) const
    {
        // Currently binary operators cannot be defined in Puppet
        // Therefore, only built-ins are supported and this function does not need to be made thread safe
        return _registry.find_unary_operator(oper);
    }

    compiler::type_alias const* environment::find_type_alias(logging::logger& logger, string const& name)
    {
        lock_guard<mutex> lock(_mutex);

        if (auto alias = _registry.find_type_alias(name)) {
            return alias;
        }

        LOG(debug, "attempting import of type alias '%1%' into environment '%2%'.", name, _name);

        string path;
        compiler::module const* module = nullptr;
        tie(path, module) = resolve_name(logger, name, find_type::type);

        if (path.empty()) {
            return nullptr;
        }

        import(logger, path, module);
        return _registry.find_type_alias(name);
    }

    resource_type const* environment::find_resource_type(logging::logger& logger, string const& name, ast::context const& context)
    {
        lock_guard<mutex> lock(_mutex);

        if (auto type = _registry.find_resource_type(name)) {
            return type;
        }

        LOG(debug, "attempting import of resource type '%1%' into environment '%2%'.", name, _name);

        // TODO: load resource type from Puppet when language supports it

        // Attempt an import of a function implemented in ruby
        return _registry.import_ruby_type(_name, name, context);
    }

    pair<node_definition const*, string> environment::find_node_definition(compiler::node const& node)
    {
        lock_guard<mutex> lock(_mutex);

        // If there are no node definitions, then do nothing
        if (!_registry.has_nodes()) {
            return make_pair(nullptr, string{});
        }

        // If there's at least one definition, then we must find one for the given node
        auto definition = _registry.find_node(node);
        if (!definition.first) {
            ostringstream message;
            message << "could not find a default node definition or a node definition for the following hostnames: ";
            bool first = true;
            node.each_name([&](string const& name) {
                if (first) {
                    first = false;
                } else {
                    message << ", ";
                }
                message << name;
                return true;
            });
            message << ".";
            throw compiler::compilation_exception(message.str());
        }
        return definition;
    }

    string environment::resolve_path(logging::logger& logger, find_type type, string const& path) const
    {
        auto file = fs::path{ path }.lexically_normal();
        if (file.is_absolute()) {
            sys::error_code ec;
            if (fs::is_regular_file(file, ec)) {
                return file.string();
            }
            return {};
        }
        if (file.empty()) {
            return {};
        }
        auto it = file.begin();
        auto ns = *it;
        fs::path subname;
        for (++it; it != file.end(); ++it) {
            subname /= *it;
        }
        if (ns.string() == "environment") {
            return find_by_path(type, subname.string());
        }
        auto module = find_module(ns.string());
        if (!module) {
            LOG(debug, "could not resolve file '%1%' because module '%2%' does not exist.", path, ns);
            return {};
        }
        return module->find_by_path(type, subname.string());
    }

    environment::environment(string name, string directory, compiler::settings settings, shared_ptr<grpc::ChannelInterface> channel) :
        finder(rvalue_cast(directory), &settings),
        _name(rvalue_cast(name)),
        _settings(rvalue_cast(settings)),
        _registry(channel)
    {
    }

    void environment::add_modules(logging::logger& logger)
    {
        auto module_path = _settings.get(settings::module_path);
        if (!module_path.as<string>()) {
            throw compilation_exception((boost::format("expected a string for $%1% setting.") % settings::module_path).str());
        }

        LOG(debug, "searching for modules using module path '%1%'.", module_path);

        // Go through each module directory to load modules
        boost::split_iterator<string::const_iterator> end;
        for (auto it = boost::make_split_iterator(*module_path.as<string>(), boost::first_finder(path_separator(), boost::is_equal())); it != end; ++it) {
            if (!*it) {
                continue;
            }

            string directory{ it->begin(), it->end() };
            auto path = make_absolute(directory, this->directory());

            sys::error_code ec;
            if (!fs::is_directory(path, ec)) {
                LOG(debug, "skipping module directory '%1%' because it is not a directory.", path);
                continue;
            }

            add_modules(logger, path);
        }
    }

    void environment::add_modules(logging::logger& logger, string const& directory)
    {
        sys::error_code ec;
        if (!fs::is_directory(directory, ec)) {
            LOG(debug, "skipping module directory '%1%' because it is not a directory.", directory);
            return;
        }

        fs::directory_iterator it{directory};
        fs::directory_iterator end{};

        // Search for modules
        vector<pair<string, string>> modules;
        LOG(debug, "searching '%1%' for modules.", directory);
        for (; it != end; ++it) {
            // If not a directory, ignore
            if (!fs::is_directory(it->status())) {
                continue;
            }
            modules.emplace_back(it->path().string(), it->path().filename().string());
        }

        // Sort the directories to ensure a deterministic order
        sort(modules.begin(), modules.end(), [](auto const& left, auto const& right) { return left.second < right.second; });

        for (auto& module : modules) {
            if (module.second == "lib") {
                // Warn that the module path may not be set correctly, but add a "lib" module
                LOG(warning, "found module named 'lib' at '%1%': this may indicate the module search path is incorrect.", module.first);
            }  else if (!module::is_valid_name(module.second)) {
                // Warn about an invalid name
                LOG(warning, "found module with invalid name '%1%' at '%2%': module will be ignored.", module.second, module.first);
                continue;
            }

            auto existing = find_module(module.second);
            if (existing) {
                LOG(warning, "module '%1%' at '%2%' conflicts with existing module at '%3%' and will be ignored.", module.second, module.first, existing->directory());
                continue;
            }

            LOG(debug, "found module '%1%' at '%2%'.", module.second, module.first);
            _modules.emplace_back(*this, rvalue_cast(module.first), rvalue_cast(module.second));
            _module_map.emplace(_modules.back().name(), &_modules.back());
        }
    }

    pair<string, module const*> environment::resolve_name(logging::logger& logger, string const& name, find_type type) const
    {
        string path;
        compiler::module const* module = nullptr;
        auto pos = name.find("::");
        if (pos == string::npos) {
            // Only manifests can be implicitly loaded by module name
            if (type == find_type::manifest && name != "environment") {
                module = find_module(name);
                if (!module) {
                    LOG(debug, "could not load 'init.pp' for module '%1%' because the module does not exist.", name);
                } else {
                    path = module->find_by_name(type, "init");
                }
            }
        } else {
            // Split into namespace and subname
            auto ns = name.substr(0, pos);
            auto subname = name.substr(pos + 2);

            if (ns == "environment") {
                // Don't resolve manifests from the environment
                if (type != find_type::manifest) {
                    path = find_by_name(type, subname);
                }
            } else {
                module = find_module(ns);
                if (!module) {
                    LOG(debug, "could not load a file for '%1%' because module '%2%' does not exist.", name, ns);
                } else {
                    path = module->find_by_name(type, subname);
                }
            }
        }
        return make_pair(rvalue_cast(path), module);
    }

    shared_ptr<ast::syntax_tree> environment::import(logging::logger& logger, string const& path, compiler::module const* module)
    {
        // TODO: this needs to be made transactional

        try {
            // Check for an already parsed AST
            auto it = _parsed.find(path);
            if (it != _parsed.end()) {
                LOG(debug, "using cached AST for '%1%' in environment '%2%'.", path, _name);
                return it->second;
            }

            // Parse the file
            LOG(debug, "importing '%1%' into environment '%2%'.", path, _name);
            auto tree = parser::parse_file(logger, path, module);
            _parsed.emplace(path, tree);

            // Validate the AST
            tree->validate();

            // Scan the tree for definitions
            compiler::scanner scanner{ logger, _name, _registry };
            scanner.scan(*tree);
            return tree;
        } catch (parse_exception const& ex) {
            throw compilation_exception(ex, path);
        }
    }

}}  // namespace puppet::compiler
