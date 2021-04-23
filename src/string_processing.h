#pragma once
#include <algorithm>
#include <execution>
#include <set>
#include <string>
#include <vector>

template <typename ExecutionPolicy>
std::vector<std::string> SplitIntoWords(
    ExecutionPolicy&& execution_policy,
    const std::string& text
) {
    std::vector<std::string> words{""};
    std::for_each(
        execution_policy,
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

inline std::vector<std::string> SplitIntoWords(const std::string& text) {
    return SplitIntoWords(std::execution::seq, text);
}

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(
    const StringContainer& strings
)
{
    std::set<std::string> non_empty_strings;
    for (const std::string& s : strings)
        if (!s.empty())
            non_empty_strings.insert(s);

    return non_empty_strings;
}