#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (text.size() == 0)
            break;

        if (c == ' ') {
            words.push_back(word);
            word.clear();
        } else {
            word += c;
        }
    }

    words.push_back(word);
    words.erase(
        remove_if(words.begin(), words.end(),
                  [](const std::string& word) { return word.empty(); }),
        words.end()
    );
    return words;
}
