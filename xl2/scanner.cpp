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
// This document is confidential.
// Do not redistribute without written permission
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

XLScanner::XLScanner(kstring name)
// ----------------------------------------------------------------------------
//   XLScanner constructor opens the file
// ----------------------------------------------------------------------------
  : fileName(name), fileLine(1),
    file(NULL),
    tokenText(""),
    stringValue(""), realValue(0.0), intValue(0),
    indents(), indent(0), indentChar(0), checkingIndent(false), column(0)
{
    file = fopen(name, "r");
    indents.push_back(0);       // We start with an indent of 0
    if (!file)
        XLError(E_ScanNoFile, name, 1, name, strerror(errno));
}


XLScanner::~XLScanner()
// ----------------------------------------------------------------------------
//   XLScanner destructor closes the file
// ----------------------------------------------------------------------------
{
    if (file)
        fclose(file);
}


#define NEXT_CHAR(c)                            \
do {                                            \
    tokenText += c;                             \
    stringValue += c;                           \
    c = fgetc(file);                            \
} while(0)


#define NEXT_LOWER_CHAR(c)                      \
do {                                            \
    tokenText += c;                             \
    stringValue += tolower(c);                  \
    c = fgetc(file);                            \
} while(0)


#define IGNORE_CHAR(c)                          \
do {                                            \
    tokenText += c;                             \
    c = fgetc(file);                            \
} while (0)


token_t XLScanner::NextToken()
// ----------------------------------------------------------------------------
//   Return the next token, and compute the token text and value
// ----------------------------------------------------------------------------
{
    stringValue = "";
    tokenText = "";
    intValue = 0;
    realValue = 0.0;
    base = 0;

    // Check if file was opened correctly
    if (!file)
        return tokEOF;

    // Check if we unindented far enough for multiple indents
    while (indents.back() > indent)
    {
        indents.pop_back();
        return tokUNINDENT;
    }

    // Read next character
    int c = fgetc(file);


    // Skip spaces and check indendation
    while (isspace(c) && c != EOF)
    {
        if (c == '\n')
        {
            // New line: start counting indentation
            fileLine += 1;
            checkingIndent = true;
            column = 0;
        }
        else if (checkingIndent)
        {
            // Can't mix tabs and spaces
            if (c == ' ' || c == '\t')
            {
                if (!indentChar)
                    indentChar = c;
                else if (indentChar != c)
                    XLError(E_ScanMixedIndent, fileName, fileLine);
                column += 1;
            }
        }

        // Keep looking for more spaces
        c = fgetc(file);
    } // End of space testing

    // Stop counting indentation
    if (checkingIndent)
    {
        ungetc(c, file);
        checkingIndent = false;
        if (column > indent)
        {
            // Strictly deeper indent : report
            indent = column;
            indents.push_back(indent);
            return tokINDENT;
        }
        else if (column < indent)
        {
            // Unindenting: remove rightmost indent level
            MZ_ASSERT(indents.size());
            indents.pop_back();
            indent = column;
            
            // If we unindented, but did not go as far as the
            // most recent indent, report inconsistency.
            if (indents.back() < column)
            {
                XLError(E_ScanInconsistent, fileName, fileLine);
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

    // Report end of file if that's what we've got
    if (c == EOF)
	return tokEOF;


    // Look for numbers
    if (isdigit(c))
    {
        bool floating_point = false;
        bool exponent = false;
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
                        XLError(E_ScanDoubleUnder, fileName, fileLine);
                }
            }
            
            // Check if this is a based number
            if (c == '#' && !basedNumber)
            {
                base = intValue;
                if (base < 2 || base > 36)
                {
                    base = 36;
                    XLError(E_ScanInvalidBase, fileName, fileLine);
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
            c = fgetc(file);
            if (digit_values[c] >= base)
            {
                // This is something else following an integer: 1..3, 1.(3)
                ungetc(c, file);
                ungetc('.', file);
                return tokINTEGER;
            }
            else
            {
                tokenText += '.';
                stringValue += '.';
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
                            XLError(E_ScanDoubleUnder, fileName, fileLine);
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
        ungetc(c, file);
        return floating_point ? tokREAL : tokINTEGER;
    } // Numbers

    // Look for names
    else if (isalpha(c))
    {
        while (isalnum(c) || c == '_')
        {
            if (c == '_')
                IGNORE_CHAR(c);
            else
                NEXT_LOWER_CHAR(c);
        }
        ungetc(c, file);
        return tokNAME;
    }
    
    // Look for strings
    else if (c == '"' || c == '\'')
    {
        char eos = c;
        tokenText = c;
        c = fgetc(file);
        for(;;)
        {
            // Check end of string
            if (c == eos)
            {
                IGNORE_CHAR(c);
                if (c != eos)
                {
                    ungetc(c, file);
                    return eos == '"' ? tokSTRING : tokQUOTE;
                }

                // Double: put it in
            }
            if (c == EOF || c == '\n')
            {
                XLError(E_ScanStringEOL, fileName, fileLine);
                return tokERROR;
            }
            NEXT_CHAR(c);
        }
    }

    // Look for parentheses
    else if (c == '(' || c == ')' ||
             c == '{' || c == '}' ||
             c == '[' || c == ']')
    {
        stringValue = c;
        tokenText = c;
        return (c == '(' || c == '{' || c == '[') ? tokPAROPEN : tokPARCLOSE;
    }

    // Look for other symbols
    else
    {
        while (ispunct(c) &&
               c != '\'' && c != '"' &&
               c != '(' && c != ')' &&
               c != '{' && c != '}' &&
               c != EOF)
            NEXT_CHAR(c);
        ungetc(c, file);
        return tokSYMBOL;
    }

    MZ_ASSERT(!"What are we doing here?");
    return tokERROR;
}


text XLScanner::Comment(text EOC)
// ----------------------------------------------------------------------------
//   Keep adding characters until end of comment is found (and consumed)
// ----------------------------------------------------------------------------
{
    kstring eoc = EOC.c_str();
    kstring match = eoc;
    text comment = "";
    int c = 0;

    while (*match && c != EOF)
    {
        c = fgetc(file);

        if (c == '\n')
        {
            // New line: start counting indentation
            fileLine += 1;
            checkingIndent = true;
            column = 0;
        }
        else if (checkingIndent)
        {
            if (isspace(c))
                column += 1;
            else
                checkingIndent = false;
        }

        if (c == *match)
            match++;
        else
            match = eoc;
        comment += c;
    }

    // Returned comment includes termination
    return comment;
}
