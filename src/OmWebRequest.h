/*
 * OmWebRequest.h
 * 2016-11-21
 *
 * This class helps parse parts of a web request. Specifically,
 * it handles everything after the host. For example, if the
 * full URL is "http://fish.com/content/somePage.html?a=1&b=2" then
 * this class will operate only on "/content/somePage.html?a=1&b=2".
 * It lets you access the path and the query values.
 *
 * This code is platform agnostic.
 *
 * It does NOT handle %-encoding in the request for now.
 *
 * EXAMPLE
 *
 *       static OmWebRequest r; // static for reuse without hitting the stack space
 *       r.init("/dir/page.html?v1=One&v2=Two");
 *       const char *path = r.path;          // will be "/dir/page.html"
 *       const char *v1 = r.getValue("v1");  // will be "One"
 *       const char *v2 = r.getValue("v2");  // will be "Two"
 *       const char *v3 = r.getValue("v3");  // will be NULL
 */

#ifndef __OmWebRequest__
#define __OmWebRequest__

#include "OmUtil.h"

/// This class is only about the part of a Uri past the host & port. Just the path & query.
/// Doesn't handle hex coding or quotes and stuff.
class OmWebRequest
{
    static const int kRequestLengthMax = 320;
public:
    const char *path;
    std::vector<const char *> query;
    char request[kRequestLengthMax + 1];
    
    OmWebRequest()
    {
        this->path = "";
        this->query.clear();
    }

    void init(const char *request)
    {
        this->path = "";
        this->query.clear();
        int rIx = 0;
        for(int ix = 0; ix <= kRequestLengthMax; ix++)
        {
            char c = request[rIx++];
            if(c == '%' && request[rIx] && request[rIx + 1])
            {
                // handle %-hex character escapes
                this->request[ix] = omHexToInt(request + rIx, 2);
                rIx += 2;
            }
            else
            {
                this->request[ix] = c;
            }

            if(c == 0)
                break;
        }
        // pin the length safely
        this->request[kRequestLengthMax] = 0;
        
        // path is always first.
        this->path = this->request;
        
        // walk it and insert breaks.
        char *w = this->request;
        while(char c = *w)
        {
            if(c == '?'
               || c == '='
               || c == '&')
            {
                // if it's a separator, break and note it.
                *w = 0;
                this->query.push_back(w + 1);
            }
            w++;
        }
    }
    
    const char *getValue(const char *key)
    {
        for(int ix = 0; ix < (int)this->query.size() - 1; ix += 2)
        {
            if(omStringEqual(key, this->query[ix]))
                return this->query[ix + 1];
        }
        return 0;
    }
};
#endif /* defined(__OmWebRequest__) */
