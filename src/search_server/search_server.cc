#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../document/document.h"
#include "../string_processing/string_processing.h"
#include "search_server.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

namespace std {

SearchServer::SearchServer() = default;

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(ThrowInvalidWords(MakeUniqueNonEmptyStrings(stop_words)))
{
}

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    return documents_ids_.at(index);
}

void SearchServer::AddDocument(
    int document_id, const string& document,
    DocumentStatus status, const vector<int>& ratings
)
{
    if (count(documents_ids_.begin(), documents_ids_.end(), document_id)) {
        throw invalid_argument("already used id --> " + to_string(document_id));
    } else if (document_id < 0) {
        throw invalid_argument("negative document id --> " + to_string(document_id));
    }

    const vector<string> words = ThrowInvalidWords(SplitIntoWordsNoStop(document));
    const double inv_word_count = 1.0/words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(
        document_id,
        DocumentData{ComputeAverageRating(ratings), status}
    );
    documents_ids_.push_back(document_id);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
    const string& raw_query,
    int document_id
) const
{
    const Query query = ThrowInvalidQuery(ParseQuery(raw_query));
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (!IsContainWord(word)) {
            continue;
        }
        if (IsWordContainId(word, document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
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

vector<Document> SearchServer::FindTopDocuments(
    const string& raw_query,
    DocumentStatus status_to_find
) const
{
    return FindTopDocuments(
        raw_query,
        [status_to_find](int document_id, DocumentStatus status, int rating) {
            return status == status_to_find;
        }
    );
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(
    const string& raw_query,
    DocumentPredicate doc_predicate
) const
{
    const Query query = ThrowInvalidQuery(ParseQuery(raw_query));
    auto matched_documents = FindAllDocuments(query, doc_predicate);
    sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
            return (std::abs(lhs.relevance - rhs.relevance) < 1e-6)
                    ? lhs.rating > rhs.rating
                    : lhs.relevance > rhs.relevance;
            });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

bool SearchServer::IsValidWord(const string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word);
}

bool SearchServer::IsContainWord(const string& word) const {
    return word_to_document_freqs_.count(word);
}

bool SearchServer::IsWordContainId(
    const string& word,
    const int& document_id
) const
{
    return word_to_document_freqs_.at(word).count(document_id);
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(string word) const {
    bool is_minus = (word[0] == '-');
    if (is_minus) {
        word = word.substr(1);
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
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
        [](const string& word){ return word[0] == '-' || word.empty(); }
    );
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return (word_to_document_freqs_.at(word).size())
            ? log(static_cast<double>(GetDocumentCount())/word_to_document_freqs_.at(word).size())
            : 0;
}

template <typename StringContainer>
StringContainer SearchServer::ThrowInvalidWords(
    const StringContainer& words
)
{
    return ThrowInvalidWords(
        words,
        [](const string& word){ return !IsValidWord(word); }
    );
}

template <typename StringContainer, typename WordPredicate>
StringContainer SearchServer::ThrowInvalidWords(
    const StringContainer& words,
    WordPredicate word_predicate
)
{
    for (const string& word : words) {
        if (word_predicate(word)) {
            throw invalid_argument("invalid word --> [" + word + ']');
        }
    }
    return words;
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(
    const Query& query,
    DocumentPredicate predicate
) const
{
    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            if (predicate(document_id,
                            documents_.at(document_id).status,
                            documents_.at(document_id).rating)
            )
            {
                document_to_relevance[document_id] += term_freq*inverse_document_freq;
            }
        }
    }

    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
        });
    }
    return matched_documents;
}

} // namespace std