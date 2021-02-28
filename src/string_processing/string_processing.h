#pragma once
#include <set>
#include <string>
#include <vector>

std::vector<std::string> SplitIntoWords(const std::string& text);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(
    const StringContainer& strings
)
{
    std::set<std::string> non_empty_strings;
    for (const std::string& s : strings) {
        if (!s.empty()) {
            non_empty_strings.insert(s);
        }
    }
    return non_empty_strings;
}