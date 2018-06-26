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
    /// `nDecimalLeading0s` contains the number of leading zeroes in the decimal part,
    /// for correctly calculating its power of 10.
    /// **WARNING** Assumes `decimalPart >= 0!`
    inline static F64 makeF64fromI64s(I64 wholePart, I64 decimalPart, unsigned int nDecimalLeading0s)
    {
        F64 f64Decimals = F64(decimalPart);
        while(f64Decimals >= 1.0)
        {
            // Iterative division; better than using log10/floor/pow since it potentially
            // is more precise + you don't have to check for the log10(0) case
            // (Also: multiplication by 0.1 should be faster than division by 10.0)
            f64Decimals *= 0.1;
        }
        f64Decimals *= pow(0.1, nDecimalLeading0s); // Adjust for the number of leading 0s
        return F64(wholePart) + f64Decimals;
    }


    /// === NOTE: `readX()` functions return `true` if they could read `outX`,
    /// or `false` if they could not. If the value they were trying to read was
    /// most definitely a `X`, they also set `outErr` to something. ====

    /// Tries to read a `key<whitespace>=` part, outputting the key name.
    /// Does *NOT* clear `outKey`, only appends the key to it!
    /// Keys should not contain whitespace.
    static bool readKeyPart(std::string& outKey, ErrString& outErr, std::istream& stream)
    {
        char ch;
        while(ch = stream.get(), ch != '=' && !isspace(ch) /*(includes '\n')*/)
        {
            if(ch == EOF)
            {
                outErr = "Failed reading key part; unexpected EOF before '='";
                return false;
            }
            outKey += ch;
        }

        // Skip any whitespace following the key name. The next character must be
        // a '=' for the key part to be correct
        stream >> std::ws;
        if(stream.get() == '=')
        {
            return true;
        }
        else
        {
            outErr = "Failed reading key part; extraneous characters before '=' "
                     "(note: key names can't contain spaces!)";
            return false;
        }
    }

    /// Tries to read a `'string'` or `"string"`, including the delimiters.
    /// Outputs the string's contents.
    static bool readString(std::string& outStr, ErrString& outErr, std::istream& stream)
    {
        outStr.clear();

        char delim = stream.peek();
        if(!(delim == '"' || delim == '\''))
        {
            // Not a string
            return false;
        }
        stream.get(); // (consume delimiter)

        char ch;
        while(ch = stream.get(), ch != delim)
        {
            // TODO IMPLEMENT String escapes
            if(ch == EOF)
            {
                outErr = "Unterminated string";
                return false;
            }
            outStr += ch;
        }

        return true;
    }

    /// Tries to read a 64-bit integer, including optional sign characters and ignoring
    /// _ chars in the int. Also outputs the number of leading zeroes in the
    /// number that have been ignored (useful for decimal parts).
    static bool readI64(I64& outInt, unsigned int& nLeadingZeroes, ErrString& outErr,
                        std::istream& stream)
    {
        // 64-bit signed integer: -9223372036854775808 to +9223372036854775807
        // Can't have I64s with more digits than 19!
        char digitChars[19];
        memset(digitChars, '0', sizeof(digitChars)); // (NOTE: literal 0, not null terminator!)

        char ch = stream.peek();

        unsigned int nDigits = 0;
        nLeadingZeroes = 0;
        I64 sign = 1;

        // Interpret first character
        if(ch == '0')
        {
            // Leading zero
            nLeadingZeroes ++;
        }
        else if(isdigit(ch))
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
            return false;
        }
        stream.get(); // (consume first character)

        // Read any other integer/ignored characters
        bool stillReadingLeading0s = true;
        while(ch = stream.peek(), isdigit(ch) || ch == '_')
        {
            if(ch == '_')
            {
                // '_', ignore
            }
            else if(ch == '0' && stillReadingLeading0s)
            {
                // Leading zero, ignore but keep count
                nLeadingZeroes ++;
            }
            else
            {
                // Digit (can be a non-leading zero)
                stillReadingLeading0s = false; // We're done with that here

                if(nDigits == sizeof(digitChars))
                {
                    outErr = "Number has too many digits";
                    return false;
                }
                digitChars[nDigits ++] = ch;
            }

            stream.get(); // Consume peeked character
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

        return true;
    }

    /// Tries to read either `outValue.f64` or `outValue.i64`.
    static bool readI64orF64(ConfigValue& outValue, ErrString& outErr, std::istream& stream)
    {
        // Whole and decimal parts are implicitly 0 if not set
        I64 wholePart = 0, decimalPart = 0;
        unsigned int nWholeLeading0s = 0, nDecimalLeading0s = 0;

        bool numPartFound = false; // `true` if any/both numeric parts (whole/decimal) were found
        bool dotFound = false; // `true` if a decimal separator has been found in the past,
                               // I.E. true if this value is a F64!

        char ch;
        while(ch = stream.peek(), true)
        {
            if(isspace(ch) || ch == EOF)
            {
                // End of the number
                break;
            }
            else if(ch == '_')
            {
                stream.get(); // Ignore the '_'
            }
            else if(ch == '.')
            {
                if(dotFound)
                {
                    outErr = "Multiple '.'s in F64 value";
                    return false;
                }
                dotFound = true;
                stream.get(); // Consume the '.'
            }
            else if(isdigit(ch) || (!dotFound && (ch == '+' || ch == '-')))
            {
                I64& i64Val = !dotFound ? wholePart : decimalPart;
                unsigned int& nLeading0sVal = !dotFound ? nWholeLeading0s : nDecimalLeading0s;

                if(!readI64(i64Val, nLeading0sVal, outErr, stream))
                {
                    if(outErr)
                    {
                        // Some error occurred while reading whole or decimal part
                        return false;
                    }
                    else if(dotFound)
                    {
                        outErr = "Extraneous characters in I64 or F64 value";
                        return false;
                    }
                }
                else
                {
                    numPartFound = true;
                }
            }
            else if(dotFound && (ch == '+' || ch == '-'))
            {
                outErr = "Sign not allowed in decimal part of F64";
                return false;
            }
            else
            {
                // Extraneous characters
                break;
            }

        } // (end of reading loop)

        if(!dotFound && !numPartFound)
        {
            // There isn't a number here
            return false;
        }
        else if(dotFound && !numPartFound)
        {
            outErr = "Malformed F64: '.' present but did not find any digit (use 0. or .0 for F64 zero)";
            return false;
        }

        if(dotFound)
        {
            outValue.type = ConfigValue::F64;
            outValue.value.f64 = makeF64fromI64s(wholePart, decimalPart, nDecimalLeading0s);
        }
        else
        {
            // TODO Use octal if whole part has leading zero(es)?
            outValue.type = ConfigValue::I64;
            outValue.value.i64 = wholePart;
        }
        return true;
    }

    /// Tries to read a [tT]/[fF] boolean.
    static bool readBoolean(bool& outBoolean, ErrString& outErr, std::istream& stream)
    {
        char ch = stream.peek();
        switch(ch)
        {
        case 'T':
        case 't':
            outBoolean = true;
            stream.get(); // (consume 'T')
            return true;

        case 'F':
        case 'f':
            outBoolean = false;
            stream.get(); // (consume 'F')
            return true;

        default:
            // Not a boolean
            return false;
        }
    }


    /// Tries to read any supported `ConfigValue`. Returns an error reason on error.
    static bool readAnyValue(ConfigValue& outValue, ErrString& outErr, std::istream& stream)
    {
        // TODO REFACTOR "Behold! The switch-if-cascade *OF DOOM*!"
        outErr = {};

        if(readString(outValue.value.string, outErr, stream))
        {
            outValue.type = ConfigValue::String;
            return true;
        }
        else if(outErr)
        {
            // String read error
            return false;
        }

        if(readBoolean(outValue.value.boolean, outErr, stream))
        {
            outValue.type = ConfigValue::Boolean;
            return true;
        }
        else if(outErr)
        {
            // Boolean read error
            return false;
        }

        if(readI64orF64(outValue, outErr, stream))
        {
            return true;
        }
        else if(outErr)
        {
            // I64 or F64 read error
            return false;
        }

        // No value found here
        return false;
    }

public:
    static ErrString parse(Config& outCfg, std::istream& stream, const char* ext)
    {
        // TODO IMPORTANT ENHANCEMENT Include caret positions in parsing errors!
        std::string section(""), key, line;
        ErrString outErr;
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
                if(!readKeyPart(key, outErr, stream))
                {
                    // Key parsing error
                    return outErr;
                }

                // Skip whitespace between '=' and value. If EOF or a newline is
                // found here the kv-pair is malformed
                char ch;
                while(ch = stream.peek(), ch != EOF && ch != '\n' && isspace(ch))
                {
                    stream.get(); // Consume character
                }

                // Parse value
                if(!readAnyValue(value, outErr, stream))
                {
                    if(!outErr)
                    {
                        // Whitespace or some extraneous characters were matched
                        return "Value after '=' missing or malformed (expected a string, I64, F64 or boolean)";
                    }
                    return outErr;
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
