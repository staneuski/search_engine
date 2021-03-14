#include "remove_duplicates.h"

template <typename Key, typename Value>
bool IsKeysEqual(
    const std::map<Key, Value>& lhs,
    const std::map<Key, Value>& rhs
)
{
    return lhs.size() == rhs.size()
        && std::equal(
            lhs.begin(), lhs.end(),
            rhs.begin(),
            [](const auto& a, const auto& b) { return a.first == b.first; }
        );
}

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> duplicate_ids;

    for (int outer_doc_id : search_server) {
        const std::map<std::string, double>& word_to_freq = search_server.GetWordFrequencies(outer_doc_id);

        for (int inner_doc_id : search_server) {
            if (outer_doc_id == inner_doc_id
                || binary_search(duplicate_ids.begin(), duplicate_ids.end(), outer_doc_id)) {
                break;
            }

            if (IsKeysEqual(word_to_freq, search_server.GetWordFrequencies(inner_doc_id))) {
                duplicate_ids.push_back(outer_doc_id);
            }
        }
    }

    for (int duplicate_id : duplicate_ids) {
        std::cout << "Found duplicate document id " << duplicate_id << std::endl;
        search_server.RemoveDocument(duplicate_id);
    }
}
