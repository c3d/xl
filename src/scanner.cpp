// *****************************************************************************
// scanner.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//     This is the file scanner for the XL project
//
//     See detailed documentation in scanner.h
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2009-2013,2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "scanner.h"
#include "errors.h"
#include "syntax.h"
#include "options.h"
#include "utf8.h"
#include "utf8_fileutils.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>


XL_BEGIN


// ============================================================================
//
//   Options
//
// ============================================================================

namespace Opt
{
BooleanOption caseSensitive("case_sensitive",
                            "Make scanner case sensitive",
                            true);
} // namespace Opt



// ============================================================================
//
//    Helper classes
//
// ============================================================================

class DigitValue
// ----------------------------------------------------------------------------
//   A class used to access the digit value for a character
// ----------------------------------------------------------------------------
{
public:
    enum { SIZE = 0x100, INVALID = 999 };

public:
    DigitValue()
    {
        uint i;

        // Default for all digits
        for (i = 0; i < SIZE; i++)
            based_value[i] = base64_value[i] = INVALID;

        // Bases 2-36
        for (i = '0'; i <= '9'; i++)
            based_value[i] = i - '0';
        for (i = 'A'; i <= 'Z'; i++)
            based_value[i] = i - 'A' + 10;
        for (i = 'a'; i <= 'z'; i++)
            based_value[i] = i - 'a' + 10;

        // For base-64 (see https://en.wikipedia.org/wiki/Base64)
        for (i = 'A'; i <= 'Z'; i++)
            base64_value[i] = i - 'A';
        for (i = 'a'; i <= 'z'; i++)
            base64_value[i] = i - 'a' + 26;
        for (i = '0'; i <= '9'; i++)
            base64_value[i] = i - '0' + 52;
        base64_value[0 + '+'] = 62;
        base64_value[0 + '/'] = 63;

        // Select regular bases by default
        select_base(10);
    }

    void select_base(uint base)
    {
        value = base == 64 ? base64_value : based_value;
    }

    uint operator[] (uint8_t c)
    {
        return value[c];
    }

private:
    uint based_value[SIZE];
    uint base64_value[SIZE];
    const uint *value;
} digits;



// ============================================================================
//
//    Scanner
//
// ============================================================================

Scanner::Scanner(kstring name, Syntax &stx, Positions &pos, Errors &err)
// ----------------------------------------------------------------------------
//   Open the file and make sure it's readable
// ----------------------------------------------------------------------------
    : syntax(stx),
      input(*new utf8_ifstream(name)),
      tokenText(""),
      textValue(""), realValue(0.0), intValue(0), base(10),
      indents(), indent(0), indentChar(0),
      position(0), lineStart(0),
      positions(pos), errors(err),
      checkingIndent(false), settingIndent(false),
      hadSpaceBefore(false), hadSpaceAfter(false),
      mustDeleteInput(true)
{
    indents.push_back(0);       // We start with an indent of 0
    position = positions.OpenFile(name);
    if (input.fail())
        err.Log(Error("File $1 cannot be read: $2", position).
                Arg(name).Arg(strerror(errno), ""));

    // Skip UTF-8 BOM if present
    if (input.get() != 0xEF)
        input.unget();
    else if (input.get() != 0xBB)
        input.unget(), input.unget();
    else if(input.get() != 0xBF)
        input.unget(), input.unget(), input.unget();
}


Scanner::Scanner(std::istream &input,
                 Syntax &stx, Positions &pos, Errors &err,
                 kstring fileName)
// ----------------------------------------------------------------------------
//   Open the file and make sure it's readable
// ----------------------------------------------------------------------------
    : syntax(stx),
      input(input),
      tokenText(""),
      textValue(""), realValue(0.0), intValue(0), base(10),
      indents(), indent(0), indentChar(0),
      position(0), lineStart(0),
      positions(pos), errors(err),
      checkingIndent(false), settingIndent(false),
      hadSpaceBefore(false), hadSpaceAfter(false),
      mustDeleteInput(false)
{
    indents.push_back(0);       // We start with an indent of 0
    position = positions.OpenFile(fileName);
    if (input.fail())
        err.Log(Error("Input stream $1 cannot be read: $2", position)
                .Arg(fileName)
                .Arg(strerror(errno)));
}


Scanner::Scanner(const Scanner &parent)
// ----------------------------------------------------------------------------
//   Open the file and make sure it's readable
// ----------------------------------------------------------------------------
    : syntax(parent.syntax),
      input(parent.input),
      tokenText(""),
      textValue(""), realValue(0.0), intValue(0), base(10),
      indents(parent.indents),
      indent(parent.indent),
      indentChar(parent.indentChar),
      position(parent.position), lineStart(parent.lineStart),
      positions(parent.positions), errors(parent.errors),
      checkingIndent(false), settingIndent(false),
      hadSpaceBefore(parent.hadSpaceBefore),
      hadSpaceAfter(parent.hadSpaceAfter),
      mustDeleteInput(false)
{}


Scanner::~Scanner()
// ----------------------------------------------------------------------------
//   Scanner destructor closes the file
// ----------------------------------------------------------------------------
{
    if (mustDeleteInput)
        delete &input;
    positions.CloseFile(position);
}


static inline char xlcase(char c)
// ----------------------------------------------------------------------------
//   Change the case following XL rules
// ----------------------------------------------------------------------------
{
    if (IS_UTF8_FIRST(c) || IS_UTF8_NEXT(c))
        return c;
    if (Opt::caseSensitive)
        return c;
    return tolower(c);
}


#define NEXT_CHAR(c)                            \
    do {                                        \
        tokenText += c;                         \
        textValue += c;                         \
        c = input.get();                        \
        position++;                             \
    } while(0)


#define NEXT_LOWER_CHAR(c)                      \
    do {                                        \
        tokenText += xlcase(c);                 \
        textValue += c;                         \
        c = input.get();                        \
        position++;                             \
    } while(0)


#define IGNORE_CHAR(c)                          \
    do {                                        \
        textValue += c;                         \
        c = input.get();                        \
        position++;                             \
    } while (0)


token_t Scanner::NextToken(bool hungry)
// ----------------------------------------------------------------------------
//   Return the next token, and compute the token text and value
// ----------------------------------------------------------------------------
{
    textValue = "";
    tokenText = "";
    intValue = 0;
    realValue = 0.0;
    base = 0;

    // Check if input was opened correctly
    if (!input.good())
    {
        record(scanner, "End of file at position %lu", position);
        return tokEOF;
    }

    // Check if we unindented far enough for multiple indents
    hadSpaceBefore = true;
    while (indents.back() > indent)
    {
        indents.pop_back();
        record(scanner, "Unindent at position %lu", position);
        return tokUNINDENT;
    }

    // Read next character
    int c = input.get();
    position++;

    // Skip spaces and check indendation
    hadSpaceBefore = false;
    while (isspace(c) && c != EOF)
    {
        hadSpaceBefore = true;
        if (c == '\n')
        {
            // New line: start counting indentation
            checkingIndent = true;
            lineStart = position;
        }
        else if (checkingIndent)
        {
            // Can't mix tabs and spaces
            if (c == ' ' || c == '\t')
            {
                if (!indentChar)
                    indentChar = c;
                else if (indentChar != c)
                    errors.Log(Error("Mixed tabs and spaces in indentation",
                                     position));
            }
        }

        // Keep looking for more spaces
        if (c == '\n')
            textValue += c;
        c = input.get();
        position++;
    } // End of space testing

    // Stop counting indentation
    if (checkingIndent)
    {
        input.unget();
        position--;
        checkingIndent = false;
        ulong column = position - lineStart;

        if (settingIndent)
        {
            // We set a new indent, for instance after opening paren
            indents.push_back(indent);
            indent = column;
            settingIndent = false;
            record(scanner, "Newline at position %lu", position);
            return tokNEWLINE;
        }
        else if (column > indent)
        {
            // Strictly deeper indent : report
            indent = column;
            indents.push_back(indent);
            record(scanner, "Indent column %u at position %lu",indent,position);
            return tokINDENT;
        }
        else if (column < indents.back())
        {
            // Unindenting: remove rightmost indent level
            XL_ASSERT(indents.size());
            indents.pop_back();
            indent = column;

            // If we unindented, but did not go as far as the
            // most recent indent, report inconsistency.
            if (indents.back() < column)
            {
                errors.Log(Error("Unindenting to the right "
                                 "of previous indentation", position));
                record(scanner,
                       "Unindent %u less than column %u at position %lu",
                       indents.back(), column, position);
                return tokERROR;
            }

            // Otherwise, report that we unindented
            // We may report multiple tokUNINDENT if we unindented deep
            record(scanner,
                   "Unindent column %u at position %lu",
                   indent, position);
            return tokUNINDENT;
        }
        else
        {
            // Exactly the same indent level as before
            record(scanner,
                   "Indented newline column %u at position %lu",
                   column, position);
            return tokNEWLINE;
        }
    }

    // Report end of input if that's what we've got
    if (input.eof())
    {
        record(scanner, "End of file after skipping at position %lu", position);
	return tokEOF;
    }

    // Clear spelling from whitespaces
    textValue = "";

    // Look for numbers
    if (isdigit(c))
    {
        bool floating_point = false;
        bool basedNumber = false;

        base = 10;
        intValue = 0;
        digits.select_base(base);

        // Take integral part (or base)
        do
        {
            while (digits[c] < base)
            {
                intValue = base * intValue + digits[c];
                NEXT_CHAR(c);
                if (c == '_')       // Skip a single underscore
                {
                    IGNORE_CHAR(c);
                    if (c == '_')
                        errors.Log(Error("Two _ characters in a row look ugly",
                                         position));
                }
            }

            // Check if this is a based number
            if (c == '#' && !basedNumber)
            {
                base = intValue;
                if (base != 64 && (base < 2 || base > 36))
                {
                    base = 36;
                    errors.Log(Error("The base $1 is not valid (2..36 or 64)",
                                     position).Arg(textValue));
                }
                digits.select_base(base);
                NEXT_CHAR(c);
                intValue = 0;
                basedNumber = true;
            }
            else
            {
                basedNumber = false;
            }
        } while (basedNumber);

        // Check for fractional part
        realValue = intValue;
        if (c == '.')
        {
            int nextDigit = input.peek();
            if (digits[nextDigit] >= base)
            {
                // This is something else following an natural: 1..3, 1.(3)
                input.unget();
                position--;
                hadSpaceAfter = false;
                record(scanner, "Natural %ld ending in '.' at position %lu",
                       intValue, position);
                return tokNATURAL;
            }
            else
            {
                floating_point = true;
                double comma_position = 1.0;

                NEXT_CHAR(c);
                while (digits[c] < base)
                {
                    comma_position /= base;
                    realValue += comma_position * digits[c];
                    NEXT_CHAR(c);
                    if (c == '_')
                    {
                        IGNORE_CHAR(c);
                        if (c == '_')
                            errors.Log(Error("Two _ characters in a row "
                                             "look really ugly",
                                             position));
                    }
                }
            }
        }

        // Check if we have a second '#' at end of based number
        if (c == '#')
            NEXT_CHAR(c);

        // Check for the exponent
        if (c == 'e' || c == 'E')
        {
            NEXT_CHAR(c);

            uint exponent = 0;
            bool negative_exponent = false;

            // Exponent sign
            if (c == '+')
            {
                NEXT_CHAR(c);
            }
            else if (c == '-')
            {
                NEXT_CHAR(c);
                negative_exponent = true;
                floating_point = true;
            }

            // Exponent value
            while (digits[c] < 10)
            {
                exponent = 10 * exponent + digits[c];
                NEXT_CHAR(c);
                if (c == '_')
                    IGNORE_CHAR(c);
            }

            // Compute base^exponent
            double exponent_value = 1.0;
            double multiplier = base;
            while (exponent)
            {
                if (exponent & 1)
                    exponent_value *= multiplier;
                exponent >>= 1;
                multiplier *= multiplier;
            }

            // Compute actual value
            if (negative_exponent)
                realValue /= exponent_value;
            else
                realValue *= exponent_value;
            intValue = (ulong) realValue;
        }

        // Return the token
        input.unget();
        position--;
        hadSpaceAfter = isspace(c);
        if (floating_point)
            record(scanner, "Real %g at position %lu",
                   realValue, position);
        else
            record(scanner, "Natural %ld at position %lu",
                   intValue, position);
        return floating_point ? tokREAL : tokNATURAL;
    } // Numbers

    // Look for names
    else if (IS_UTF8_OR_ALPHA(c))
    {
        while (isalnum(c) || c == '_' || IS_UTF8_FIRST(c) || IS_UTF8_NEXT(c))
        {
            if (c == '_')
                IGNORE_CHAR(c);
            else
                NEXT_LOWER_CHAR(c);
        }
        input.unget();
        position--;
        hadSpaceAfter = isspace(c);
        if (syntax.IsBlock(textValue, endMarker))
        {
            bool closing = endMarker == "";
            record(scanner, "Name-like block %+s %s at position %lu",
                   closing ? "closing" : "opening",
                   textValue.c_str(), position);
            return closing ? tokPARCLOSE : tokPAROPEN;
        }
        record(scanner, "Name %s at position %lu", textValue.c_str(), position);
        return tokNAME;
    }

    // Look for texts
    else if (c == '"' || c == '\'')
    {
        char eos = c;
        tokenText = c;
        c = input.get();
        position++;
        for(;;)
        {
            // Check end of text
            if (c == eos)
            {
                tokenText += c;
                c = input.get();
                position++;
                if (c != eos)
                {
                    input.unget();
                    position--;
                    hadSpaceAfter = isspace(c);
                    record(scanner, "Text %s at position %lu",
                           tokenText.c_str(), position);
                    return eos == '"' ? tokTEXT : tokQUOTE;
                }

                // Double: put it in
            }
            if (c == EOF || c == '\n')
            {
                errors.Log(Error("End of input in the middle of a text",
                                 position));
                hadSpaceAfter = false;
                if (c == '\n')
                {
                    input.unget();
                    position--;
                }
                record(scanner, "Truncated text %s at position %lu",
                       tokenText.c_str(), position);
                return eos == '"' ? tokTEXT : tokQUOTE;
            }
            NEXT_CHAR(c);
        }
    }

    // Look for single-char block delimiters (parentheses, etc)
    if (syntax.IsBlock(c, endMarker))
    {
        textValue = c;
        tokenText = c;
        hadSpaceAfter = false;
        bool closing = endMarker == "";
        record(scanner, "Single-char block %+s %s at position %lu",
               closing ? "closing" : "opening",
               textValue.c_str(), position);
        return closing ? tokPARCLOSE : tokPAROPEN;
    }

    // Look for other symbols
    bool hadChar = false;
    while (ispunct(c) && c != '\'' && c != '"' && c != EOF &&
           !syntax.IsBlock(c, endMarker))
    {
        hadChar = true;
        NEXT_CHAR(c);
        if (!hungry && !syntax.KnownPrefix(tokenText))
            break;
    }
    if (hadChar)
    {
        input.unget();
        position--;
    }
    else
    {
        errors.Log(Error("Invalid character code $1", position)).Arg(c);
        NEXT_CHAR(c);
    }
    if (!hungry)
    {
        while (tokenText.length() > 1 && !syntax.KnownToken(tokenText))
        {
            tokenText.erase(tokenText.length() - 1, 1);
            textValue.erase(textValue.length() - 1, 1);
            input.unget();
            position--;
        }
    }
    hadSpaceAfter = isspace(c);
    if (syntax.IsBlock(textValue, endMarker))
    {
        bool closing = endMarker == "";
        record(scanner, "Multi-char block %+s %s at position %lu",
               closing ? "closing" : "opening",
               textValue.c_str(), position);
        return closing ? tokPARCLOSE : tokPAROPEN;
    }
    record(scanner, "Symbol %s at position %lu", textValue.c_str(), position);
    return tokSYMBOL;
}


text Scanner::Comment(text EOC, bool stripIndent)
// ----------------------------------------------------------------------------
//   Keep adding characters until end of comment is found (and consumed)
// ----------------------------------------------------------------------------
{
    kstring  eoc     = EOC.c_str();
    kstring  match   = eoc;
    text     comment = "";
    int      c       = 0;
    bool     skip    = false;
    ulong    column  = position - lineStart;

    while (*match && c != EOF)
    {
        c = input.get();
        position++;
        skip = false;

        if (c == '\n' && stripIndent)
        {
            // New line: start counting indentation
            checkingIndent = true;
            lineStart = position;
        }
        else if (checkingIndent)
        {
            if (isspace(c))
            {
                skip = ((position - lineStart) < column);
            }
            else
            {
                checkingIndent = skip = false;
                if (column > position - lineStart)
                    column = position - lineStart;
            }
        }

        if (c == *match)
        {
            match++;
        }
        else
        {
            // Backtrack in case we had something like **/
            uint i, max = match - eoc;
            kstring end = comment.c_str() + comment.length();
            for (i = 0; i < max; i++)
            {
                if (memcmp(eoc, end - max + i, match - eoc) == 0)
                    break;
                match--;
            }
        }

        if (!skip)
            comment += c;
    }

    // Returned comment includes termination
    return comment;
}


uint Scanner::OpenParen()
// ----------------------------------------------------------------------------
//   Opening some parenthese: remember the settingIndent value
// ----------------------------------------------------------------------------
{
    uint result = indent;
    if (settingIndent)
        result = ~result;
    settingIndent = true;
    return result;
}


void Scanner::CloseParen(uint oldIndent)
// ----------------------------------------------------------------------------
//   Closing some parenthese: reset the settingIndent value
// ----------------------------------------------------------------------------
{
    bool wasSet = int(oldIndent) < 0;
    indent = wasSet ? ~oldIndent : oldIndent;
    if (!settingIndent)
        if (indent == indents.back())
            indents.pop_back();
    settingIndent = wasSet;
}



// ============================================================================
//
//    Class Positions
//
// ============================================================================

ulong Positions::OpenFile(text name)
// ----------------------------------------------------------------------------
//    Open a new file
// ----------------------------------------------------------------------------
{
    positions.push_back(Range(current_position, name));
    return current_position;
}


void Positions::CloseFile (ulong pos)
// ----------------------------------------------------------------------------
//    Remember the end position for a file
// ----------------------------------------------------------------------------
{
    current_position = pos;
}


void Positions::GetFile(ulong pos, text *file, ulong *offset)
// ----------------------------------------------------------------------------
//    Return the file and the offset in the file
// ----------------------------------------------------------------------------
{
    std::vector<Range>::iterator i;
    for (i = positions.begin(); i != positions.end(); i++)
        if (pos < (*i).start)
            break;
    if (i != positions.begin())
    {
        i--;
        if (file)
            *file = (*i).file;
        if (offset)
            *offset = pos - (*i).start;
    }
    else
    {
        if (file)
            *file = "";
        if (offset)
            *offset = pos;
    }
}


void Positions::GetInfo(ulong pos, text *out_file, ulong *out_line,
                        ulong *out_column, text *out_source)
// ----------------------------------------------------------------------------
//   Scan the input files to find the location of the error
// ----------------------------------------------------------------------------
{
    ulong  offset = 0;
    ulong  line   = 1;
    ulong  column = 0;
    text   source = "";
    text   name = "";
    FILE  *file;
    char   c;

    GetFile (pos, &name, &offset);
    if (name != "")
    {
        file = fopen(name.c_str(), "r");
        if (file)
        {
            while (!feof(file))
            {
                c = fgetc(file);
                if (c == EOF)
                    break;
                if (c == '\n')
                {
                    line++;
                    column = 0;
                    source = "";
                }
                else
                {
                    column++;
                    source += c;
                }
                offset--;
                if (offset <= 1)
                    break;
            }

            // Read rest of line
            while(!feof(file))
            {
                c = fgetc(file);
                if (c == '\n' || c == EOF)
                    break;
                source += c;
            }
            fclose(file);
        }
    }

    // Output result
    if (out_file)
        *out_file = name;
    if (out_line)
        *out_line = line;
    if (out_column)
        *out_column = column;
    if (out_source)
        *out_source = source;
}

XL_END

RECORDER(scanner, 64, "Scanner");
