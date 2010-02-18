#ifndef LCS_H
#define LCS_H
// ****************************************************************************
//  lcs.h                                                          XLR project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
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
