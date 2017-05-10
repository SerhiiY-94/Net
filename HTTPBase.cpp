#pragma warning(disable : 4018)
#include "HTTPBase.h"

#include <cstring>

using namespace net;

//const char *ContentType::str_type[] {"text/html", "image/x-icon", "image/png"};
std::vector<ContentType::StrType> ContentType::str_types = {{TEXT_HTML, "html", "text/html"},
                                                            {TEXT_CSS, "css", "text/css"},
                                                            {IMAGE_ICON, "ico", "image/x-icon"},
                                                            {IMAGE_PNG, "png", "image/png"}};

std::string ContentType::TypeString(Type type) {
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

ContentType::Type ContentType::TypeByExt(const char *ext) {
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
	return ContentType::Type::UNKNOWN;
}