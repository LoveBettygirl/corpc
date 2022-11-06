#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <map>
#include <string>
#include <vector>

namespace corpc {

class StringUtil {
public:
    // split a string to map
    // for example:  str is a=1&tt=2&cc=3  split_str = '&' joiner='='
    // get res is {"a":"1", "tt":"2", "cc", "3"}
    static void splitStrToMap(const std::string &str, const std::string &splitStr,
                                const std::string &joiner, std::map<std::string, std::string> &res);

    // split a string to vector
    // for example:  str is a=1&tt=2&cc=3  split_str = '&'
    // get res is {"a=1", "tt=2", "cc=3"}
    static void splitStrToVector(const std::string &str, const std::string &splitStr, std::vector<std::string> &res);
};

}

#endif