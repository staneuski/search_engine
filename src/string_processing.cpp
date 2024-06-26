#include "string_processing.h"

std::string GenerateWord(std::mt19937& generator, int max_length) {
    const int length = std::uniform_int_distribution(1, max_length)(generator);
    std::string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i)
        word.push_back(std::uniform_int_distribution('a', 'z')(generator));

    return word;
}

std::vector<std::string> GenerateDictionary(std::mt19937& generator,
                                       int word_count, int max_length) {
    std::vector<std::string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i)
        words.push_back(GenerateWord(generator, max_length));

    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

std::string GenerateQuery(std::mt19937& generator,
                          const std::vector<std::string>& dictionary,
                          int max_word_count) {
    const int word_count = std::uniform_int_distribution(1, max_word_count)(generator);
    std::string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty())
            query.push_back(' ');

        query += dictionary[std::uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

std::vector<std::string> GenerateQueries(
    std::mt19937& generator,
    const std::vector<std::string>& dictionary,
    int query_count,
    int max_word_count
)
{
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i)
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));

    return queries;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view text) {
    std::vector<std::string_view> words;

    const size_t pos_end = text.npos;
    while (true) {
        size_t space = text.find(' ');
        std::string_view word = text.substr(0, space);

        words.push_back(word);

        if (space == pos_end)
            break;
        else
            text.remove_prefix(space + 1);
    }

    // The last element of words vector will be empty
    // if a text at its end contains spaces
    if (words.back().empty())
        words.pop_back();

    return words;
}