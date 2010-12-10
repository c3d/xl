// ****************************************************************************
//  winglob.cpp                                                     Tao project
// ****************************************************************************
//
//   File Description:
//
//
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "configuration.h"

#ifdef CONFIG_MINGW

#include "base.h"
#include "winglob.h"
#include <dirent.h>
#include <regex.h>
#include <string>


static void globinternal(text dir, text pattern, globpaths &paths)
// ----------------------------------------------------------------------------
//   Internal variant of glob, where dir is a directory to look into
// ----------------------------------------------------------------------------
{
    // Check if we are scanning a directory
    text subpat = "";
    bool dirmode = false;

    // Transform the regexp into a file-style regexp
    for (size_t pos = 0; !dirmode && pos < pattern.length(); pos++)
    {
        char c = pattern[pos];
        switch (c)
        {
        case '*':
            pattern.replace(pos, 1, ".*");
            pos++;
            break;
        case '.':
            pattern.replace(pos, 1, "[.]");
            pos++;
            break;
        case '\\':
        case '/':
            subpat = pattern.substr(pos + 1, pattern.npos);
            pattern = pattern.substr(0, pos-1);
            dirmode = true;
            break;
        default:
            break;
        }
    }

    // Open directory
    DIR *dirp = opendir(dir.c_str());
    if (dirp)
    {
        regex_t re;
        if (regcomp(&re, pattern.c_str(), REG_EXTENDED | REG_NOSUB) == 0)
        {
            while(struct dirent *dp = readdir(dirp))
            {
                std::string entry(dp->d_name);
                if (regexec(&re, entry.c_str(), 0, NULL, 0) == 0)
                {
                    if (dirmode)
                        globinternal(dir + "\\" + entry, subpat, paths);
                    else
                        paths.push_back(entry);
                }
            }
            regfree(&re);
        }
        closedir(dirp);
    }
}


int glob(const char *pattern,
         int flags,
         int (*errfunc)(const char *epath, int errno),
         glob_t *pglob)
// ----------------------------------------------------------------------------
//   Simulation of the glob() function on Windows
// ----------------------------------------------------------------------------
{
    globinternal("", pattern, pglob->gl_pathv);
    pglob->gl_pathc = pglob->gl_pathv.size();
    return 0;
}


void globfree(glob_t *pglob)
// ----------------------------------------------------------------------------
//   Simulation of the globfree() function on Windows
// ----------------------------------------------------------------------------
{
    pglob->gl_pathv.clear();
}

#endif // CONFIG_MINGW

