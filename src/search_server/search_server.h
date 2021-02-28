#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "../document/document.h"
#include "../string_processing/string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer();

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(ThrowInvalidWords(MakeUniqueNonEmptyStrings(stop_words)))
    {
    }

    explicit SearchServer(const std::string& stop_words_text);

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    void AddDocument(int document_id, const std::string& document,
                     DocumentStatus status, const std::vector<int>& ratings);

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
        const std::string& raw_query,
        int document_id
    ) const;

    std::vector<Document> FindTopDocuments(
        const std::string& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
    ) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
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
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
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
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_ids_;

    static bool IsValidWord(const std::string& word);

    bool IsStopWord(const std::string& word) const;

    bool IsContainWord(const std::string& word) const;

    bool IsWordContainId(const std::string& word, const int& document_id) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string word) const;

    Query ParseQuery(const std::string& text) const;

    static Query ThrowInvalidQuery(const Query& query);

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename StringContainer>
    static StringContainer ThrowInvalidWords(const StringContainer& words) {
        return ThrowInvalidWords(
            words,
            [](const std::string& word){ return !IsValidWord(word); }
        );
    }

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate) {
        for (const std::string& word : words) {
            if (word_predicate(word)) {
                throw std::invalid_argument("invalid word --> [" + word + ']');
            }
        }
        return words;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const Query& query,
        DocumentPredicate predicate
    ) const
    {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
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

        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }

};
