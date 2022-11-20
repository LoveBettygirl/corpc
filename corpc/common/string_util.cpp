#include <map>
#include <string>
#include "corpc/common/string_util.h"
#include "corpc/common/log.h"

namespace corpc {

void StringUtil::splitStrToMap(const std::string &str, const std::string &splitStr,
                                const std::string &joiner, std::map<std::string, std::string> &res)
{
    if (str.empty() || splitStr.empty() || joiner.empty()) {
        LOG_DEBUG << "str or splitStr or joiner is empty";
        return;
    }
    std::string temp = str;

    std::vector<std::string> vec;
    splitStrToVector(temp, splitStr, vec);
    for (auto i : vec) {
        if (!i.empty()) {
            size_t j = i.find_first_of(joiner);
            if (j != i.npos && j != 0) {
                std::string key = i.substr(0, j);
                std::string value = i.substr(j + joiner.size(), i.size() - j - joiner.size());
                LOG_DEBUG << "insert key = " << key << ", value=" << value;
                res[key] = value;
            }
        }
    }
}

void StringUtil::splitStrToVector(const std::string &str, const std::string &splitStr, std::vector<std::string> &res)
{
    if (str.empty() || splitStr.empty()) {
        LOG_DEBUG << "str or splitStr is empty";
        return;
    }

    std::string temp = str;
    if (temp.substr(temp.size() - splitStr.size(), splitStr.size()) != splitStr) {
        temp += splitStr;
    }

    while (true) {
        size_t i = temp.find_first_of(splitStr);
        if (i == temp.npos) {
            return;
        }
        int l = temp.size();
        std::string x = temp.substr(0, i);
        temp = temp.substr(i + splitStr.size(), l - i - splitStr.size());
        if (!x.empty()) {
            res.push_back(std::move(x));
        }
    }
}

}