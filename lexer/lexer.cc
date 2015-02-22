#include "lexer.hpp"

using namespace std;
using namespace boost::spirit;

namespace puppet { namespace lexer {

    lexer_istreambuf_iterator lex_begin(ifstream& file)
    {
        return lexer_istreambuf_iterator(typename lexer_istreambuf_iterator::base_type(make_default_multi_pass(istreambuf_iterator<char>(file))));
    }

    lexer_istreambuf_iterator lex_end(ifstream& file)
    {
        return lexer_istreambuf_iterator(typename lexer_istreambuf_iterator::base_type(make_default_multi_pass(istreambuf_iterator<char>())));
    }

    lexer_string_iterator lex_begin(string const& str)
    {
        return lexer_string_iterator(typename lexer_string_iterator::base_type(str.begin()));
    }

    lexer_string_iterator lex_end(string const& str)
    {
        return lexer_string_iterator(typename lexer_string_iterator::base_type(str.end()));
    }

    tuple<string, size_t> get_line_and_column(ifstream& fs, size_t position, size_t tab_width)
    {
        const size_t READ_SIZE = 4096;
        char buf[READ_SIZE];

        // Read backwards in chunks looking for the closest newline before the given position
        size_t start;
        for (start = (position > (READ_SIZE + 1) ? position - READ_SIZE - 1 : 0); fs; start -= (start < READ_SIZE ? start : READ_SIZE)) {
            if (!fs.seekg(start)) {
                return make_tuple("", 1);
            }

            // Read data into the buffer
            if (!fs.read(buf, position < READ_SIZE ? position : READ_SIZE)) {
                return make_tuple("", 1);
            }

            // Find the last newline in the buffer
            auto it = find(reverse_iterator<char*>(buf + fs.gcount()), reverse_iterator<char*>(buf), '\n');
            if (it != reverse_iterator<char*>(buf)) {
                start += distance(buf, it.base());
                break;
            }

            if (start == 0) {
                break;
            }
        }

        // Calculate the column
        size_t column = (position - start) + 1;

        // Find the end of the current line
        size_t end = position;
        fs.seekg(end);
        auto eof =  istreambuf_iterator<char>();
        for (auto it = istreambuf_iterator<char>(fs.rdbuf()); it != eof; ++it) {
            if (*it == '\n') {
                break;
            }
            ++end;
        }

        // Read the line
        size_t size = end - start;
        fs.seekg(start);
        vector<char> line_buffer(size);
        if (!fs.read(line_buffer.data(), line_buffer.size())) {
            return make_tuple("", 1);
        }

        // Convert tabs to spaces
        string line = string(line_buffer.data(), line_buffer.size());
        if (tab_width > 1) {
            column += count(line.begin(), line.begin() + column, '\t') * (tab_width - 1);
        }

        return make_tuple(move(line), column);
    }

    tuple<string, size_t> get_line_and_column(string const& input, size_t position, size_t tab_width)
    {
        auto start = input.rfind('\n', position);
        if (start == string::npos) {
            start = 0;
        } else {
            ++start;
        }

        string line = input.substr(start, input.find('\n', start));

        // Convert tabs to spaces
        size_t column = (position - start) + 1;
        if (tab_width > 1) {
            column += count(line.begin(), line.begin() + column, '\t') * (tab_width - 1);
        }

        return make_tuple(move(line), column);
    }

    token_position get_last_position(ifstream& input)
    {
        // We need to read the entire file looking for new lines
        auto pos = input.tellg();
        input.seekg(0);

        std::size_t position = 0, lines = 1;
        for (std::istreambuf_iterator<char> it(input), end; it != end; ++it) {
            if (*it == '\n') {
                ++lines;
            }
            ++position;
        }

        input.seekg(pos);
        return make_tuple(position, lines);
    }

    token_position get_last_position(string const& input)
    {
        // Count the number of lines in the input
        return make_tuple(
            input.size(),
            count(input.begin(), input.end(), '\n') + 1
        );
    }

}}  // namespace puppet::lexer
