#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace std {

vector<string> SplitIntoWords(const string& text);

class SearchServer {
public:
    SearchServer();

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const string& stop_words_text);

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    void AddDocument(int document_id, const string& document,
                     DocumentStatus status, const vector<int>& ratings);

    tuple<vector<string>, DocumentStatus> MatchDocument(
        const string& raw_query,
        int document_id
    ) const;

    vector<Document> FindTopDocuments(
        const string& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
    ) const;

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      DocumentPredicate doc_predicate) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    const set<string> stop_words_ = {};
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_ids_;

    static bool IsValidWord(const string& word);

    bool IsStopWord(const string& word) const;

    bool IsContainWord(const string& word) const;

    bool IsWordContainId(const string& word, const int& document_id) const;

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    QueryWord ParseQueryWord(string word) const;

    Query ParseQuery(const string& text) const;

    static Query ThrowInvalidQuery(const Query& query);

    double ComputeWordInverseDocumentFreq(const string& word) const;

    template <typename StringContainer>
    static StringContainer ThrowInvalidWords(const StringContainer& words);

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate);

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(
        const Query& query,
        DocumentPredicate predicate
    ) const;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings);

} // namespace std
