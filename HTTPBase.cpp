#pragma warning(disable : 4018)
#include "HTTPBase.h"

#include <cstring>

//const char *ContentType::str_type[] {"text/html", "image/x-icon", "image/png"};
std::vector<net::ContentType::StrType> net::ContentType::str_types = {{TextHTML, "html", "text/html"},
																	  {TextCSS, "css", "text/css"},
																	  {ImageIcon, "ico", "image/x-icon"},
																	  {ImagePNG, "png", "image/png"}};

std::string net::ContentType::TypeString(eType type) {
    if (type < str_types.size() && str_types[type].t == type) {
        return str_types[type].str;
    }
    for (int i = 0; i < str_types.size(); i++) {
        if (str_types[i].t == type) {
            return str_types[i].str;
        }
    }
    return "";
}

net::ContentType::eType net::ContentType::TypeByExt(const char *ext) {
    /*if (strcmp(ext, "html") == 0) {
        return TEXT_HTML;
    } else if (strcmp(ext, "ico") == 0) {
        return IMAGE_ICON;
    } else if (strcmp(ext, "png") == 0) {
        return IMAGE_PNG;
    } else {
        return TEXT_HTML;
    }*/
    for (int i = 0; i < str_types.size(); i++) {
        if (strcmp(str_types[i].ext, ext) == 0) {
            return str_types[i].t;
        }
    }
	return ContentType::eType::Unknown;
}