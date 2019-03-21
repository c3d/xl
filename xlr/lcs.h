#ifndef LCS_H
#define LCS_H
// *****************************************************************************
// lcs.h                                                              XL project
// *****************************************************************************
//
// File description:
//
//     A Longest Common Subsequence implementation.
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS

#include "base.h"
#include <vector>

XL_BEGIN

template <typename T>
struct LCS
// ----------------------------------------------------------------------------
//    A Longest Common Subsequence Implementation (LCS-Delta algorithm)
// ----------------------------------------------------------------------------
{
    LCS(): m(0), n(0) {}
    virtual ~LCS() {}

    void Compute (T &x, T &y);
    T&   Extract(T &x, T &out);
    void Extract2(T &x, T& outx, T &y, T &outy);
    int  Length();

protected:

    enum Arrows
    {
        None = 0,
        Up   = 1,
        Left = 2,
        Both = 3
    };

    std::vector<std::vector<Arrows> > b, c;
    int m, n;
};

template <typename T>
void LCS<T>::Compute (T &x, T &y)
// ----------------------------------------------------------------------------
//    Compute the LCS of x and y
// ----------------------------------------------------------------------------
{
    int i, j;

    m = x.size();
    n = y.size();

    b.resize(m+1);
    for (i = 0; i < m+1; i++)
        b[i].resize(n+1);

    c.resize(m+1);
    for (i = 0; i < m+1; i++)
        c[i].resize(n+1);

    for (i = 1; i <= m; i++)
        c[i][0] = None;

    for (i = 0; i <= n; i++)
        c[0][i] = None;

    for (i = 1; i <= m; i++)
    {
        for (j = 1; j <= n; j++)
        {
            if (x[i - 1] == y[j - 1])
            {
                c[i][j] = (Arrows)(c[i - 1][j - 1] + 1);
                b[i][j] = Both;
            }
            else if (c[i - 1][j] >= c[i][j - 1])
            {
                c[i][j] = c[i - 1][j];
                b[i][j] = Up;
            }
            else
            {
                c[i][j] = c[i][j - 1];
                b[i][j] = Left;
            }
        }
    }
};

template <typename T>
int LCS<T>::Length()
// ----------------------------------------------------------------------------
//    Return the length of the LCS
// ----------------------------------------------------------------------------
{
  return c[m][n];
}

template <typename T>
T& LCS<T>::Extract(T &x, T& out)
// ----------------------------------------------------------------------------
//    Extract the LCS by appending elements from 'x' to 'out'
// ----------------------------------------------------------------------------
{
    int i = m, j = n;

    while (i && j)
        if (b[i][j] == Both)
        {
            i--;
            j--;
            typename T::iterator it = out.begin();
            out.insert(it, x[i]);
        }
        else if (b[i][j] == Up)
            i--;
        else
            j--;
    return out;
}

template <typename T>
void LCS<T>::Extract2(T &x, T& outx, T&y, T&outy)
// ----------------------------------------------------------------------------
//    Extract the LCS by appending elements from x to outx and from y to outy
// ----------------------------------------------------------------------------
{
    int i = m, j = n;

    while (i && j)
        if (b[i][j] == Both)
        {
            i--;
            j--;
            typename T::iterator it;
            it = outx.begin();
            outx.insert(it, x[i]);
            it = outy.begin();
            outy.insert(it, y[j]);
        }
        else if (b[i][j] == Up)
            i--;
        else
            j--;
}

XL_END

#endif // LCS_H
