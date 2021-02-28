#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../document/document.h"
#include "../string_processing/string_processing.h"

class SearchServer {
public:
    SearchServer();

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

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
    ) const;

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
    static StringContainer ThrowInvalidWords(const StringContainer& words);

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate);

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const Query& query,
        DocumentPredicate predicate
    ) const;
};
