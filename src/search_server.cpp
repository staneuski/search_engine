#include "search_server.h"

const std::map<std::string, double>& SearchServer::GetWordFrequencies(
    int document_id
) const {
    const static std::map<std::string, double> document_to_empty_freqs;
    return document_to_word_freqs_.count(document_id)
           ? document_to_word_freqs_.at(document_id)
           : document_to_empty_freqs;
}

void SearchServer::AddDocument(
    int document_id,
    const std::string& document,
    DocumentStatus status,
    const std::vector<int>& ratings
)
{
    if (documents_.count(document_id)) {
        throw std::invalid_argument(
            "already used id --> " + std::to_string(document_id)
        );
    } else if (document_id < 0) {
        throw std::invalid_argument(
            "negative document id --> " + std::to_string(document_id)
        );
    }

    const std::vector<std::string> words = ThrowInvalidWords(
        SplitIntoWordsNoStop(document)
    );
    const double inv_word_count = 1.0/words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    documents_.emplace(
        document_id,
        DocumentData{
            {words.begin(), words.end()},
            status,
            ComputeAverageRating(ratings)
        }
    );
    documents_ids_.push_back(document_id);

    UpdateInverseDocumentFreqs();
}

void SearchServer::RemoveDocument(int document_id) {
    if (!documents_.count(document_id)) {
        throw std::invalid_argument(
            "non-existent document id --> " + std::to_string(document_id)
        );
    }

    for (const std::string& word : documents_.at(document_id).words) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    documents_.erase(document_id);
    documents_ids_.erase(
        remove(documents_ids_.begin(), documents_ids_.end(), document_id),
        documents_ids_.end()
    );

    UpdateInverseDocumentFreqs();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
    const std::string& raw_query,
    int document_id
) const
{
    const Query query = ThrowInvalidQuery(ParseQuery(raw_query));
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (!IsContainWord(word)) {
            continue;
        }
        if (IsWordContainId(word, document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (!IsContainWord(word)) {
            continue;
        }
        if (IsWordContainId(word, document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::vector<Document> SearchServer::FindTopDocuments(
    const std::string& raw_query,
    DocumentStatus status_to_find
) const
{
    return FindTopDocuments(
        raw_query,
        [status_to_find](__attribute__((unused)) int document_id,
                        DocumentStatus status,
                        __attribute__((unused)) int rating) {
            return status == status_to_find;
        }
    );
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(
    const std::string& text
) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.size()) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum/static_cast<int>(ratings.size());
    } else {
        return 0;
    }
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string word) const {
    bool is_minus = (word[0] == '-');
    if (is_minus) {
        word = word.substr(1);
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop && query_word.is_minus) {
            query.minus_words.insert(query_word.data);
        } else if (!query_word.is_stop) {
            query.plus_words.insert(query_word.data);
        }
    }
    return query;
}

SearchServer::Query SearchServer::ThrowInvalidQuery(const Query& query) {
    ThrowInvalidWords(query.plus_words);
    ThrowInvalidWords(query.minus_words);
    ThrowInvalidWords(
        query.minus_words,
        [](const std::string& word){ return word[0] == '-' || word.empty(); }
    );
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(
    const std::string& word
) const
{
    return (word_to_document_freqs_.at(word).size())
            ? log(static_cast<double>(GetDocumentCount())/word_to_document_freqs_.at(word).size())
            : 0;
}

void SearchServer::UpdateInverseDocumentFreqs() {
    for (int documents_id : documents_ids_) {
        for (const std::string& word : documents_.at(documents_id).words) {
            document_to_word_freqs_[documents_id][word] = ComputeWordInverseDocumentFreq(word);
        }
    }
}
