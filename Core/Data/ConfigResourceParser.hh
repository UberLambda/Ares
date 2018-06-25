#pragma once

#include <ctype.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include "Config.hh"
#include "../Data/ResourceParser.hh"

namespace Ares
{

template <>
struct ResourceParser<Config>
{
private:
    /// Returns a float made from two integers, so that it is `wholePart.decimalPart`.
    /// **WARNING** Assumes `decimalPart >= 0!`
    inline static F64 makeF64fromI64s(I64 wholePart, I64 decimalPart)
    {
        F64 f64Decimals = F64(decimalPart);
        while(f64Decimals > 1.0)
        {
            // Iterative division; better than using log10/floor/pow since it potentially
            // is more precise + you don't have to check for the log10(0) case
            // (Also: multiplication by 0.1 should be faster than division by 10.0)
            f64Decimals *= 0.1;
        }
        return F64(wholePart) + f64Decimals;
    }


    enum ReadStatus
    {
        Read, ///< Value successfully read
        NoMatch, ///< Value does not match
        MatchError, ///< Value would have matched but an error occurred (or EOF was reached)
    };

    /// Tries to read a `key<whitespace>=` part, outputting the key name.
    /// Does *NOT* clear `outKey`, only appends the key to it!
    /// Keys should not contain whitespace.
    static ReadStatus readKeyPart(std::string& outKey, std::istream& stream)
    {
        char ch;
        while(ch = stream.get(), ch != '=' && !isspace(ch) /*(includes '\n')*/)
        {
            if(ch == EOF)
            {
                // Unterminated key
                return MatchError;
            }
            outKey += ch;
        }

        // Skip any whitespace following the key name. The next character must be
        // a '=' for the key part to be correct
        stream >> std::ws;
        return (stream.get() == '=') ? Read : MatchError;
    }

    /// Tries to read a `'string'` or `"string"`, including the delimiters.
    /// Outputs the string's contents.
    static ReadStatus readString(std::string& outStr, std::istream& stream)
    {
        outStr.clear();

        char delim = stream.peek();
        if(!(delim == '"' || delim == '\''))
        {
            // Not a string
            return NoMatch;
        }
        stream.get(); // (consume delimiter)

        char ch;
        while(ch = stream.get(), ch != delim)
        {
            // TODO IMPLEMENT String escapes
            if(ch == EOF)
            {
                // Unterminated string
                return MatchError;
            }
            outStr += ch;
        }

        return Read;
    }

    /// Tries to read a 64-bit integer, including optional sign characters and ignoring
    /// _ chars in the int.
    static ReadStatus readI64(I64& outInt, std::istream& stream)
    {
        // 64-bit signed integer: -9223372036854775808 to +9223372036854775807
        // Can't have I64s with more digits than 19!
        char digitChars[19];
        memset(digitChars, '0', sizeof(digitChars)); // (NOTE: literal 0, not null terminator!)

        char ch = stream.peek();
        unsigned int nDigits = 0;
        I64 sign = 1;

        // Interpret first character
        if(isdigit(ch))
        {
            // First digit of positive number
            digitChars[0] = ch;
            nDigits = 1;
        }
        else if(ch == '-')
        {
            // Negative sign
            sign = -1;
        }
        else if(ch == '+' || ch == '_')
        {
            // Positive sign or ignored '_' character
            // sign = 1
        }
        else
        {
            // Not a number
            return NoMatch;
        }
        stream.get(); // (consume first character)

        // Read any other integer/ignored characters
        while(ch = stream.peek(), isdigit(ch) || ch == '_')
        {
            if(ch == '_')
            {
                // '_', ignore
                stream.get();
                continue;
            }

            // Else: digit
            if(nDigits == sizeof(digitChars))
            {
                // Number has too many digits
                return MatchError;
            }

            digitChars[nDigits ++] = ch;
            stream.get();
        }

        // Convert digit strings to integer
        outInt = 0;

        I64 powOf10 = 1;
        for(int i = nDigits - 1; i >= 0; i --)
        {
            I64 digitInt = digitChars[i] - '0';
            outInt += digitInt * powOf10;
            powOf10 *= 10;
        }

        outInt *= sign;

        return Read;
    }

    /// Tries to read a [tT]/[fF] boolean.
    static ReadStatus readBoolean(bool& outBoolean, std::istream& stream)
    {
        char ch = stream.peek();
        switch(ch)
        {
        case 'T':
        case 't':
            outBoolean = true;
            stream.get(); // (consume 'T')
            return Read;

        case 'F':
        case 'f':
            outBoolean = false;
            stream.get(); // (consume 'F')
            return Read;

        default:
            return NoMatch;
        }
    }

    /// Tries to read any supported `ConfigValue`. Returns an error reason on error.
    static ErrString readAnyValue(ConfigValue& value, std::istream& stream)
    {
        // TODO REFACTOR "Behold! The switch-if-cascade *OF DOOM*!"

        switch(readString(value.value.string, stream))
        {
        case Read:
            value.type = ConfigValue::String;
            return {};

        case MatchError:
            return "Unterminated string";

        default:
            // Not a string, go on...
        break;
        }

        switch(readBoolean(value.value.boolean, stream))
        {
        case Read:
            value.type = ConfigValue::Boolean;
            return {};

        case MatchError:
            return "Boolean parsing error"; // (should never happen)

        case NoMatch:
            // Not a boolean, go on...
        break;
        }


        // Could be a I64, a F64 or a syntax error at this point
        bool isF64 = false;
        I64 f64WholePart = 0;
        I64 f64DecimalPart = 0;
        if(stream.peek() == '.')
        {
            // F64 with implicit 0 whole part
            isF64 = true;
            stream.get();
        }

        switch(readI64(value.value.i64, stream))
        {
        case MatchError:
            if(isF64)
            {
                return "Decimal part of F64 value too large";
            }
            else
            {
                return "I64 or whole part of F64 value too large";
            }

        case NoMatch:
            if(isF64)
            {
                return "Expected whole or decimal part of a F64 value around '.'";
            }
            // Not a I64, go on...
        break;

        case Read:
            if(stream.peek() == '.')
            {
                if(!isF64)
                {
                    // Actually an F64, not a I64
                    isF64 = true;
                    f64WholePart = value.value.i64;
                }
                else
                {
                    return "Multiple decimal separators ('.') in F64 value";
                }

                stream.get(); // Consume the '.'
            }

            if(!isF64)
            {
                // Read an I64 to `value.value.i64`
                value.type = ConfigValue::I64;
                return {};
            }
            else
            {
                // Have to read the decimal part of an F64, if any
                char firstDecimalPartCh = stream.peek();
                if(firstDecimalPartCh == '+' || firstDecimalPartCh == '-')
                {
                    return "Sign not allowed in the decimal part of a F64 value";
                }

                switch(readI64(f64DecimalPart, stream))
                {
                case NoMatch:
                    // Empty fractional part, or fractional part is made of junk
                    // characters. Let `readAnyValue()` take care of it in case.
                    // vvv fallthrough vvv
                case Read:
                    // Read `f64DecimalPart`, combine integer and fractional part
                    value.type = ConfigValue::F64;
                    value.value.f64 = makeF64fromI64s(f64WholePart, f64DecimalPart);
                    return {};

                case MatchError:
                    return "Decimal part of F64 value too large";
                }
            }
        }

        return "Syntax error: Value should be string, I64, F64 or boolean";
    }

public:
    static ErrString parse(Config& outCfg, std::istream& stream, const char* ext)
    {
        // TODO IMPORTANT ENHANCEMENT Include caret positions in parsing errors!
        std::string section(""), key, line;
        ConfigValue value;

        unsigned int lineNo = 1;
        while(!stream.eof())
        {
            stream >> std::ws; // Skip leading whitespace/blank lines

            char startCh = stream.peek();
            switch(startCh)
            {
            case EOF:
                // End of stream: success
                return {};
            break;

            case '#':
                // Singleline comment, ignore
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;

            case '[':
            {
                // Section delimiter
                stream.get(); // Ignore the '['
                std::getline(stream, section);

                auto endPos = section.rfind(']');
                if(endPos == std::string::npos)
                {
                    return "Failed parsing section delimiter; ']' missing";
                }
                section.resize(endPos);

                for(auto ch : section)
                {
                    if(isspace(ch))
                    {
                        return "Section name contains whitespace(s)";
                    }
                }
            }
            break;

            default:
            {
                // Key-value pair. Key name in a config is always "<section>.<key>",
                // so prepend "<section>." to it. Default section is "".
                key = section;
                key += '.';
                if(readKeyPart(key, stream) != Read)
                {
                    return "Failed parsing key/value pair; '=' misplaced or missing, or key name contains whitespace(s)";
                }

                // Skip whitespace between '=' and value. If EOF or a newline is
                // found here the kv-pair is malformed
                char ch;
                while(ch = stream.get(), ch != EOF && ch != '\n' && isspace(ch))
                {
                }

                if(ch == EOF || ch == '\n')
                {
                    return "Missing value after '='";
                }
                stream.putback(ch); // Need back the first character of the value!

                // Parse value
                auto valueErrStr = readAnyValue(value, stream);
                if(valueErrStr)
                {
                    return valueErrStr;
                }

                // Write value into output config
                outCfg.set(key, value);

                // Skip all non-newline whitespace after value
                while(ch = stream.get(), isspace(ch) && ch != '\n')
                {
                }

                if(ch != '\n')
                {
                    return "Extraneous characters after value (expected newline or EOF)";
                }
                // (note: newline already consumed)
            }
            } // (end of `switch(startCh)`)

        } // (end of read loop until EOF)

        // Should never be reached
        return "Unknown error";
    }
};

}
