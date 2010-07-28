// ****************************************************************************
//  scanner.cpp                     (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "scanner.h"
#include "errors.h"
#include "syntax.h"
#include "utf8.h"


XL_BEGIN

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
    enum { SIZE = 128, INVALID = 999 };

public:
    DigitValue()
    {
        uint i;
        for (i = 0; i < SIZE; i++)
            value[i] = INVALID;
        for (i = '0'; i <= '9'; i++)
            value[i] = i - '0';
        for (i = 'A'; i <= 'Z'; i++)
            value[i] = i - 'A' + 10;
        for (i = 'a'; i <= 'z'; i++)
            value[i] = i - 'a' + 10;
    }
    inline uint operator[] (uint c)
    {
        if (c < SIZE)
            return value[c];
        return INVALID;
    }
private:
    uint value[SIZE];
} digit_values;



// ============================================================================
//
//    Class XLScanner
//
// ============================================================================

Scanner::Scanner(kstring name, Syntax &stx, Positions &pos, Errors &err)
// ----------------------------------------------------------------------------
//   Open the file and make sure it's readable
// ----------------------------------------------------------------------------
    : syntax(stx),
      input(*new std::ifstream(name)),
      tokenText(""),
      textValue(""), realValue(0.0), intValue(0),
      indents(), indent(0), indentChar(0),
      checkingIndent(false), settingIndent(false),
      position(0), lineStart(0),
      positions(pos), errors(err),
      hadSpaceBefore(false), hadSpaceAfter(false),
      mustDeleteInput(true)
{
    indents.push_back(0);       // We start with an indent of 0
    position = positions.OpenFile(name);
    if (input.fail())
        err.Error("File '$1' cannot be read: $2",
                  0, name, strerror(errno));
}


Scanner::Scanner(std::istream &input, Syntax &stx, Positions &pos, Errors &err)
// ----------------------------------------------------------------------------
//   Open the file and make sure it's readable
// ----------------------------------------------------------------------------
    : syntax(stx),
      input(input),
      tokenText(""),
      textValue(""), realValue(0.0), intValue(0),
      indents(), indent(0), indentChar(0),
      checkingIndent(false), settingIndent(false),
      position(0), lineStart(0),
      positions(pos), errors(err),
      hadSpaceBefore(false), hadSpaceAfter(false),
      mustDeleteInput(false)
{
    indents.push_back(0);       // We start with an indent of 0
    position = positions.OpenFile("<stream>");
    if (input.fail())
        err.Error("Input stream cannot be read: $1",
                  0, strerror(errno));
}


Scanner::~Scanner()
// ----------------------------------------------------------------------------
//   XLScanner destructor closes the file
// ----------------------------------------------------------------------------
{
    if (mustDeleteInput)
        delete &input;
    positions.CloseFile(position);
}


#define NEXT_CHAR(c)                            \
do {                                            \
    tokenText += c;                             \
    textValue += c;                             \
    c = input.get();                            \
    position++;                                 \
} while(0)


#define NEXT_LOWER_CHAR(c)                      \
do {                                            \
    tokenText += tolower(c);                    \
    textValue += c;                             \
    c = input.get();                            \
    position++;                                 \
} while(0)


#define IGNORE_CHAR(c)                          \
do {                                            \
    textValue += c;                             \
    c = input.get();                            \
    position++;                                 \
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
        return tokEOF;

    // Check if we unindented far enough for multiple indents
    hadSpaceBefore = true;
    while (indents.back() > indent)
    {
        indents.pop_back();
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
                    errors.Error("Mixed tabs and spaces in indentation",
                                 position);
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
            return tokNEWLINE;
        }
        else if (column > indent)
        {
            // Strictly deeper indent : report
            indent = column;
            indents.push_back(indent);
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
                errors.Error("Unindenting to the right of previous indentation",
                             position);
                return tokERROR;
            }

            // Otherwise, report that we unindented
            // We may report multiple tokUNINDENT if we unindented deep
            return tokUNINDENT;
        }
        else
        {
            // Exactly the same indent level as before
            return tokNEWLINE;
        }
    }

    // Report end of input if that's what we've got
    if (input.eof())
	return tokEOF;

    // Clear spelling from whitespaces
    textValue = "";

    // Look for numbers
    if (isdigit(c))
    {
        bool floating_point = false;
        bool basedNumber = false;

        base = 10;
        intValue = 0;

        // Take integral part (or base)
        do
        {
            while (digit_values[c] < base)
            {
                intValue = base * intValue + digit_values[c];
                NEXT_CHAR(c);
                if (c == '_')       // Skip a single underscore
                {
                    IGNORE_CHAR(c);
                    if (c == '_')
                        errors.Error("Two _ characters in a row look ugly",
                                     position);
                }
            }

            // Check if this is a based number
            if (c == '#' && !basedNumber)
            {
                base = intValue;
                if (base < 2 || base > 36)
                {
                    base = 36;
                    errors.Error("The base '$1' is not valid, not in 2..36",
                                 position, textValue);
                }
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
            c = input.get();
            position++;
            if (digit_values[c] >= base)
            {
                // This is something else following an integer: 1..3, 1.(3)
                input.unget();
                input.unget();
                position -= 2;
                hadSpaceAfter = false;
                return tokINTEGER;
            }
            else
            {
                tokenText += '.';
                textValue += '.';
                floating_point = true;

                double comma_position = 1.0;
                while (digit_values[c] < base)
                {
                    comma_position /= base;
                    realValue += comma_position * digit_values[c];
                    NEXT_CHAR(c);
                    if (c == '_')
                    {
                        IGNORE_CHAR(c);
                        if (c == '_')
                            errors.Error("Two _ characters in a row look ugly",
                                         position);
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
            while (digit_values[c] < 10)
            {
                exponent = 10 * exponent + digit_values[c];
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
        return floating_point ? tokREAL : tokINTEGER;
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
            return endMarker == "" ? tokPARCLOSE : tokPAROPEN;
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
                    return eos == '"' ? tokSTRING : tokQUOTE;
                }

                // Double: put it in
            }
            if (c == EOF || c == '\n')
            {
                errors.Error("End of input in the middle of a text",
                             position);
                hadSpaceAfter = false;
                if (c == '\n')
                    input.unget();
                return eos == '"' ? tokSTRING : tokQUOTE;
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
        return endMarker == "" ? tokPARCLOSE : tokPAROPEN;
    }

    // Look for other symbols
    while (ispunct(c) && c != '\'' && c != '"' && c != EOF &&
           !syntax.IsBlock(c, endMarker))
    {
        NEXT_CHAR(c);
        if (!hungry && !syntax.KnownToken(tokenText))
            break;
    }
    input.unget();
    position--;
    hadSpaceAfter = isspace(c);
    if (syntax.IsBlock(textValue, endMarker))
        return endMarker == "" ? tokPARCLOSE : tokPAROPEN;
    return tokSYMBOL;
}


text Scanner::Comment(text EOC)
// ----------------------------------------------------------------------------
//   Keep adding characters until end of comment is found (and consumed)
// ----------------------------------------------------------------------------
{
    kstring eoc     = EOC.c_str();
    kstring match   = eoc;
    text    comment = "";
    int     c       = 0;
    bool    skip    = false;
    ulong   column  = position - lineStart;

    while (*match && c != EOF)
    {
        c = input.get();
        position++;
        skip = false;

        if (c == '\n')
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
            match++;
        else
            match = eoc;

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
