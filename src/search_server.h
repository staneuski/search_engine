#pragma once
#include <algorithm>
#include <cmath>
#include <execution>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

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

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(ThrowInvalidWords(MakeUniqueNonEmptyStrings(stop_words)))
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

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    void AddDocument(
        int document_id,
        const std::string_view& text,
        DocumentStatus status,
        const std::vector<int>& ratings
    );

    inline std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
        const std::string& raw_query,
        int document_id
    ) const
    {
        return MatchDocument(std::execution::seq, raw_query, document_id);
    }

    inline void RemoveDocument(int document_id) {
        RemoveDocument(std::execution::seq, document_id);
    }

    inline std::vector<Document> FindTopDocuments(
        const std::string& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
    ) const
    {
        return FindTopDocuments(
            raw_query,
            [status_to_find](__attribute__((unused)) int document_id,
                             DocumentStatus status,
                             __attribute__((unused)) int rating)
            { return status == status_to_find; }
        );
    }

    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
        ExecutionPolicy&& execution_policy,
        const std::string& raw_query,
        int document_id
    ) const;

    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& execution_policy, int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::string& raw_query,
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
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    const std::set<std::string> stop_words_ = {};
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_ids_;

    static bool IsValidWord(const std::string& word);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    static Query ThrowInvalidQuery(const Query& query);

    inline bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word);
    }

    inline bool IsContainWord(const std::string& word) const {
        return word_to_document_freqs_.count(word);
    }

    inline bool IsWordContainId(const std::string& word,
                                const int& document_id) const {
        return word_to_document_freqs_.at(word).count(document_id);
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view& text) const;

    QueryWord ParseQueryWord(std::string word) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    inline Query ParseQuery(const std::string& text) const {
        return ParseQuery(std::execution::seq, text);
    }

    inline void UpdateInverseDocumentFreqs() {
        UpdateInverseDocumentFreqs(std::execution::seq);
    }

    template<typename ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy&& execution_policy,
                     const std::string& text) const;

    template <typename StringContainer>
    static StringContainer ThrowInvalidWords(const StringContainer& words);

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate);

    template<typename ExecutionPolicy>
    void UpdateInverseDocumentFreqs(ExecutionPolicy&& excution_policy);

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                           DocumentPredicate predicate) const;
};

template<typename ExecutionPolicy>
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
    ExecutionPolicy&& execution_policy,
    const std::string& raw_query,
    int document_id
) const
{
    const Query query = ThrowInvalidQuery(
        ParseQuery(execution_policy, raw_query)
    );

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (!IsContainWord(word))
            continue;

        if (IsWordContainId(word, document_id))
            matched_words.push_back(word);
    }
    for (const std::string& word : query.minus_words) {
        if (!IsContainWord(word))
            continue;

        if (IsWordContainId(word, document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}


template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(
    ExecutionPolicy&& execution_policy,
    int document_id
) {
    if (documents_.count(document_id)) {
        std::for_each(
            execution_policy,
            documents_.at(document_id).unique_words.begin(),
            documents_.at(document_id).unique_words.end(),
            [&, document_id](const std::string& word) {
                word_to_document_freqs_.at(word).erase(document_id);
            }
        );

        documents_.erase(document_id);
        documents_ids_.erase(
            remove(execution_policy, documents_ids_.begin(), documents_ids_.end(), document_id),
            documents_ids_.end()
        );

        UpdateInverseDocumentFreqs();
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::string& raw_query,
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
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    return matched_documents;
}

template<typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(ExecutionPolicy&& excution_policy,
                                             const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(excution_policy, text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop && query_word.is_minus)
            query.minus_words.insert(query_word.data);
        else if (!query_word.is_stop)
            query.plus_words.insert(query_word.data);
    }
    return query;
}

template <typename StringContainer>
StringContainer SearchServer::ThrowInvalidWords(const StringContainer& words) {
    return ThrowInvalidWords(
        words,
        [](const std::string& word){ return !IsValidWord(word); }
    );
}

template <typename StringContainer, typename WordPredicate>
StringContainer SearchServer::ThrowInvalidWords(const StringContainer& words,
                                                WordPredicate word_predicate) {
    for (const std::string& word : words)
        if (word_predicate(word))
            throw std::invalid_argument("invalid word --> [" + word + ']');

    return words;
}

template<typename ExecutionPolicy>
void SearchServer::UpdateInverseDocumentFreqs(ExecutionPolicy&& excution_policy) {
    std::for_each(
        excution_policy,
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
    const Query& query,
    DocumentPredicate predicate
) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0)
            continue;

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
            if (predicate(document_id,
                          documents_.at(document_id).status,
                          documents_.at(document_id).rating)
            )
                document_to_relevance[document_id] += term_freq*inverse_document_freq;
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0)
            continue;
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
            document_to_relevance.erase(document_id);
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance)
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
        });

    return matched_documents;
}