#pragma once
#include <algorithm>
#include <cmath>
#include <execution>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer() = default;

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    explicit SearchServer(const std::string_view& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(ThrowInvalidWords(MakeUniqueNonEmptyWords(stop_words)))
    {
    }

    inline auto begin() const noexcept {
        return documents_ids_.begin();
    }

    inline auto end() const noexcept {
        return documents_ids_.end();
    }

    inline int GetDocumentCount() const noexcept {
        return documents_.size();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void AddDocument(
        int document_id,
        const std::string_view& text,
        DocumentStatus status,
        const std::vector<int>& ratings
    );

    inline std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::string& raw_query,
        int document_id
    ) const
    {
        return MatchDocument(std::execution::seq, raw_query, document_id);
    }

    inline void RemoveDocument(int document_id) {
        RemoveDocument(std::execution::seq, document_id);
    }

    void RemoveDocument(std::execution::sequenced_policy, int document_id);

    void RemoveDocument(std::execution::parallel_policy, int document_id);

    inline std::vector<Document> FindTopDocuments(
        const std::string_view& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
    ) const
    {
        return FindTopDocuments(std::execution::seq, raw_query, status_to_find);
    }

    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        ExecutionPolicy execution_policy,
        const std::string_view& raw_query,
        int document_id
    ) const;

    template <typename ExecutionPolicy>
    inline std::vector<Document> FindTopDocuments(
        ExecutionPolicy execution_policy,
        const std::string_view& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
    ) const;

    template <typename DocumentPredicate>
    inline std::vector<Document> FindTopDocuments(
        const std::string_view& raw_query,
        DocumentPredicate doc_predicate
    ) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(
        ExecutionPolicy execution_policy,
        const std::string_view& raw_query,
        DocumentPredicate doc_predicate
    ) const;

private:
    struct DocumentData {
        DocumentData() = default;
        explicit DocumentData(const std::vector<std::string>& words,
                              DocumentStatus status,
                              const std::vector<int>& ratings)
            : unique_words({words.begin(), words.end()})
            , status(status)
            , rating(ComputeAverageRating(ratings))
        {
        }

        std::set<std::string> unique_words;
        DocumentStatus status;
        int rating;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string_view, std::less<>> plus_words;
        std::set<std::string_view, std::less<>> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_ = {};
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_ids_;

    static bool IsValidWord(const std::string_view& word);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    static Query ThrowInvalidQuery(const Query& query);

    inline bool IsStopWord(const std::string_view& word) const {
        return stop_words_.count(word);
    }

    inline bool IsContainWord(const std::string_view& word) const {
        return word_to_document_freqs_.count(word);
    }

    inline bool IsWordContainId(const std::string& word,
                                const int& document_id) const {
        return word_to_document_freqs_.at({word.begin(), word.end()}).count(document_id);
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view& text) const;

    QueryWord ParseQueryWord(std::string_view word) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    inline Query ParseQuery(const std::string_view& text) const {
        return ParseQuery(std::execution::seq, text);
    }

    inline void UpdateInverseDocumentFreqs() {
        UpdateInverseDocumentFreqs(std::execution::seq);
    }

    std::vector<Document> ConvertToMatchedDocuments(
        std::map<int, double> document_to_relevance
    ) const;

    template<typename ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy execution_policy,
                     const std::string_view& text) const;

    template <typename StringContainer>
    static StringContainer ThrowInvalidWords(const StringContainer& words);

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate);

    template<typename ExecutionPolicy>
    void UpdateInverseDocumentFreqs(ExecutionPolicy execution_policy);

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy,
                                           const Query& query,
                                           DocumentPredicate predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy,
                                           const Query& query,
                                           DocumentPredicate predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate, typename Container>
    void FindAllDocuments(ExecutionPolicy execution_policy,
                          const Query& query,
                          DocumentPredicate predicate,
                          Container& document_to_relevance) const;
};

template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    ExecutionPolicy execution_policy,
    const std::string_view& raw_query,
    int document_id
) const
{
    const Query query = ThrowInvalidQuery(
        ParseQuery(execution_policy, raw_query)
    );

    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.plus_words) {
        if (!IsContainWord(word))
            continue;

        if (IsWordContainId({word.begin(), word.end()}, document_id))
            matched_words.push_back(word);
    }
    for (const std::string_view& word : query.minus_words) {
        if (!IsContainWord(word))
            continue;

        if (IsWordContainId({word.begin(), word.end()}, document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy execution_policy,
    const std::string_view& raw_query,
    DocumentStatus status_to_find
) const
{
    return FindTopDocuments(
        execution_policy,
        raw_query,
        [status_to_find](__attribute__((unused)) int document_id,
                        DocumentStatus status,
                        __attribute__((unused)) int rating)
        { return status == status_to_find; }
    );
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::string_view& raw_query,
    DocumentPredicate doc_predicate
) const
{
    return FindTopDocuments(std::execution::seq, raw_query, doc_predicate);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy execution_policy,
    const std::string_view& raw_query,
    DocumentPredicate doc_predicate
) const
{
    const Query query = ThrowInvalidQuery(ParseQuery(raw_query));
    auto matched_documents = FindAllDocuments(execution_policy, query, doc_predicate);

    sort(
        matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
        return (std::abs(lhs.relevance - rhs.relevance) < 1e-6)
                ? lhs.rating > rhs.rating
                : lhs.relevance > rhs.relevance;
        }
    );
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

    return matched_documents;
}

template<typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(
    ExecutionPolicy execution_policy,
    const std::string_view& text
) const
{
    Query query;
    const std::vector<std::string_view> words = SplitIntoWordsView(text);
    std::for_each(
        execution_policy,
        words.begin(), words.end(),
        [&](const auto& word) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop && query_word.is_minus)
                query.minus_words.insert(query_word.data);
            else if (!query_word.is_stop)
                query.plus_words.insert(query_word.data);
        }
    );
    return query;
}

template <typename StringContainer>
StringContainer SearchServer::ThrowInvalidWords(const StringContainer& words) {
    return ThrowInvalidWords(
        words,
        [](const auto& word){ return !IsValidWord(word); }
    );
}

template <typename StringContainer, typename WordPredicate>
StringContainer SearchServer::ThrowInvalidWords(const StringContainer& words,
                                                WordPredicate word_predicate) {
    for (const auto& word : words) {
        const std::string word_s{word.begin(), word.end()};
        if (word_predicate(word_s))
            throw std::invalid_argument("invalid word --> [" + word_s + ']');
    }
    return words;
}

template<typename ExecutionPolicy>
void SearchServer::UpdateInverseDocumentFreqs(ExecutionPolicy execution_policy) {
    std::for_each(
        execution_policy,
        documents_ids_.begin(),
        documents_ids_.end(),
        [&](const int documents_id){
            for (const std::string& word : documents_.at(documents_id).unique_words)
                document_to_word_freqs_[documents_id][word] = ComputeWordInverseDocumentFreq(word);
        }
    );
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    std::execution::sequenced_policy,
    const Query& query,
    DocumentPredicate predicate
) const
{
    std::map<int, double> document_to_relevance;
    FindAllDocuments(std::execution::seq, query, predicate, document_to_relevance);
    return ConvertToMatchedDocuments(document_to_relevance);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    std::execution::parallel_policy,
    const Query& query,
    DocumentPredicate predicate
) const
{
    ConcurrentMap<int, double> document_to_relevance;
    FindAllDocuments(std::execution::par, query, predicate, document_to_relevance);
    return ConvertToMatchedDocuments(document_to_relevance.BuildOrdinaryMap());
}

template <typename ExecutionPolicy, typename DocumentPredicate, typename Container>
void SearchServer::FindAllDocuments(
    ExecutionPolicy execution_policy,
    const Query& query,
    DocumentPredicate predicate,
    Container& document_to_relevance
) const
{
    std::for_each(
        execution_policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        [this, &document_to_relevance, predicate](const std::string_view& word) {
            if (word_to_document_freqs_.count(word)) {
                const std::string word_s{word.begin(), word.end()};

                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_s);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word_s))
                    if (predicate(document_id,
                                  documents_.at(document_id).status,
                                  documents_.at(document_id).rating)
                    ) document_to_relevance[document_id] += term_freq*inverse_document_freq;
            }
        }
    );

    std::for_each(
        execution_policy,
        query.minus_words.begin(),
        query.minus_words.end(),
        [this, &document_to_relevance, predicate](const std::string_view& word) {
            if (word_to_document_freqs_.count(word)) {
                const std::string word_s{word.begin(), word.end()};
                for (const auto& [document_id, _] : word_to_document_freqs_.at(word_s))
                    document_to_relevance.erase(document_id);
            }
        }
    );
}