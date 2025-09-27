#include "VoicePackUtil.h"


bool VoicePackUtil::compareStrings(const std::string& str1, const std::vector<std::string>& str2Values)
{
    for (const auto& str2 : str2Values) {
        if (str1.length() == str2.length()) {
            for (int i = 0; i < str1.length(); ++i) {
                if (std::tolower(str1[i]) != std::tolower(str2[i]))
                    return false;
            }
        }
    }

    return true;
}