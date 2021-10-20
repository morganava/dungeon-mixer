#include <smol/array.hpp>
#include <smol/json.hpp>

namespace smol
{
    // strips out comments and trailing commas and then passes to json::parse
    nl::json parse_json(const array<uint8_t>& mb)
    {
        std::vector<uint8_t> js;
        js.reserve(mb.size());

        const uint8_t* read = mb.begin();
        const uint8_t* end = mb.end();

        while(read < end)
        {
            // string
            if (read[0] == '"')
            {
                // advance until we get to end of string
                js.push_back(*read);
                read++;
                // ensure we're not dealing with an escaped quote
                while(read[0] != '"' ||
                      read[-1] == '\\')
                {
                    js.push_back(*read);
                    read++;
                }
                js.push_back(*read);
                read++;
            }
            // single line comment
            else if (read[0] == '/' &&
                     read + 1 < end &&
                     read[1] == '/')
            {
                // skip over the '//'
                read += 2;
                // keep skipping until we get to the newline
                while(read < end &&
                      read[0] != '\n') {
                    read++;
                }
            }
            // block comment
            else if (read[0] == '/' &&
                     read +1 < end &&
                     read[1] == '*')
            {
                // skip over the '/*'
                read += 2;
                // keep skipping until we get to the '*/'
                while(read < end &&
                      (read[0] != '/' || read[-1] != '*'))
                {
                    read++;
                }
                read++;
            }
            // trailing comma
            else if (read[0] == ',')
            {
                const uint8_t* lookahead = read;
                lookahead++;
                bool done = false;
                // skip over whitespace until
                // we get to a } or ]
                while(!done)
                {
                    switch(*lookahead)
                    {
                        // whitespace
                        case ' ':
                        case '\t':
                        case '\r':
                        case '\n':
                            lookahead++;
                            break;
                        // found end of array or object
                        case ']':
                        case '}':
                            // push a space instead of comma
                            js.push_back(' ');
                            read++;
                            done = true;
                            break;
                        default:
                            // found something else, so we are
                            // in the middle
                            js.push_back(*read);
                            read++;
                            done = true;
                            break;
                    }
                }
            }
            // everything else
            else
            {
                js.push_back(*read);
                read++;
            }
        }

        // second pass to overwrite trailing commas

        for(auto it = js.begin(); it < js.end(); it++)
        {
            if (*it == ',')
            {
                auto comma = it;
                bool done = false;
                while(!done)
                {
                    it++;
                    switch(*it)
                    {
                        // whitespace
                        case ' ':
                        case '\t':
                        case '\r':
                        case '\n':
                            break;
                        case ']':
                        case '}':
                            *comma = ' ';
                        default:
                            done = true;
                            break;
                    }
                }
            }
        }

        return nl::json::parse(js.begin(), js.end());
    }
}