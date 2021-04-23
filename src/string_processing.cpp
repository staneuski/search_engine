#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words{""};
    std::for_each(
        text.begin(), text.end(),
        [&words](const char& c) {
            if (c != ' ')
                words.back().push_back(c);
            else if (!words.back().empty())
                words.push_back({});
        }
    );

    // The last element of words vector will be empty
    // if a text at its end contains spaces
    if (words.back().empty())
        words.pop_back();

    return words;
}