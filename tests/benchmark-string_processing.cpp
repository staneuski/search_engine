#include <iterator>
#include <iostream>
#include <random>
#include <numeric>
#include <string>
#include <sstream>
#include <vector>

#include "log_duration.h"
#include "string_processing.h"

int main() {
    using namespace std;
    mt19937 generator;

    const auto words = GenerateDictionary(generator, 300'000, 15);
    const string text = accumulate(
        next(words.begin()),
        words.end(),
        words[0],
        [](const string& lhs, const string& rhs) {
            return lhs + ' ' + rhs;
        }
    );

    {
        LOG_DURATION_STDERR("SplitIntoWords"sv);
        SplitIntoWords(text);
    }

    return 0;
}