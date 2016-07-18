#include <puppet/compiler/evaluation/repl.hpp>
#include <puppet/compiler/evaluation/evaluator.hpp>
#include <puppet/compiler/exceptions.hpp>
#include <boost/format.hpp>

using namespace std;
using namespace puppet::compiler::lexer;
namespace x3 = boost::spirit::x3;

namespace puppet { namespace compiler { namespace evaluation {

    static string REPL_PATH = "<repl>";

    struct evaluation_helper
    {
        evaluation_helper(evaluation::repl& repl, bool& multiline) :
            _repl(repl),
            _multiline(multiline)
        {
        }

        ~evaluation_helper()
        {
            _repl.complete(_multiline);
        }

     private:
        evaluation::repl& _repl;
        bool& _multiline;
    };

    repl::repl(evaluation::context& context) :
        _context(context)
    {
        _prompt = _context.node().name() + ":001:1> ";
    }

    string const& repl::prompt() const
    {
        return _prompt;
    }

    size_t repl::count() const
    {
        return _count;
    }

    size_t repl::line() const
    {
        return _line;
    }

    boost::optional<repl::result> repl::evaluate(string const& source)
    {
        return evaluate(source.c_str());
    }

    boost::optional<repl::result> repl::evaluate(char const* source)
    {
        bool multiline = false;
        evaluation_helper helper{ *this, multiline };

        if (!source || !*source) {
            multiline = !_buffer.empty();
            return boost::none;
        }

        _buffer += source;

        repl::result result;
        result.source = _buffer;

        try {
            // Import the source into the environment
            auto tree = _context.node().environment().import_source(_context.node().logger(), _buffer, REPL_PATH);

            // Evaluate all statements, returning the last value
            evaluation::evaluator evaluator{ _context };
            for (auto const& statement : tree->statements) {
                result.value = evaluator.evaluate(statement);
            }
        } catch (parse_exception const& ex) {
            if (ex.begin().offset() == _buffer.length()) {
                multiline = true;
                return boost::none;
            }
            result.exception = compilation_exception{ ex , REPL_PATH, _buffer };
        } catch (evaluation_exception const& ex) {
            result.exception = compilation_exception{ ex };
        } catch (compilation_exception& ex) {
            result.exception = rvalue_cast(ex);
        }
        return result;
    }

    void repl::complete(bool multiline)
    {
        // If the evaluation didn't complete because it spans lines, append a newline and increment the line count
        if (multiline) {
            _buffer += '\n';
            ++_line;
        } else {
            _buffer.clear();
            _line = 1;
            ++_count;
        }
        _prompt = (boost::format("%1%:%2$03d:%3%> ") % _context.node().name() % _count % _line).str();
    }

}}}  // puppet::compiler::evaluation
