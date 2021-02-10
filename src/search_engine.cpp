#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

/* ----------------------------- Search Server ----------------------------- */

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (text.size() == 0) {
            break;
        }
        if (c == ' ') {
            words.push_back(word);
            word.clear();
        } else {
            word += c;
        }
    }

    words.push_back(word);
    words.erase(
        remove_if(words.begin(), words.end(), [](const string& word) {
            return word == ""s;
        }),
        words.end()
    );
    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating)
    {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        ThrowInvalidStopWords();
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
        ThrowInvalidStopWords();
    }

    int GetDocumentCount() const {
        return documents_.size();
    }
    int GetDocumentId(int index) const {
        if (index > 0 && index <= GetDocumentCount()) {
            documents_ids_.at(index);
        }
        return documents_ids_.at(index);
    }

    void AddDocument(int document_id, const string& document,
                     DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument(
                "negative document id --> " + to_string(document_id)
            );
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0/words.size();
        for (const string& word : words) {
            {
                ThrowInvalidWord(word);
                if (count(documents_ids_.begin(), documents_ids_.end(), document_id)) {
                    throw invalid_argument(
                        "already used id --> " + to_string(document_id)
                    );
                }
            }
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(
            document_id,
            DocumentData{ComputeAverageRating(ratings), status}
        );
        documents_ids_.push_back(document_id);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
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

    vector<Document> FindTopDocuments(
        const string& raw_query,
        DocumentStatus status_to_find = DocumentStatus::ACTUAL
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
    vector<Document> FindTopDocuments(const string& raw_query,
                                      DocumentPredicate doc_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, doc_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                return (abs(lhs.relevance - rhs.relevance) < 1e-6)
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

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word);
    }
    bool IsContainWord(const string& word) const {
        return word_to_document_freqs_.count(word);
    }
    bool IsWordContainId(const string& word, const int& document_id) const {
        return word_to_document_freqs_.at(word).count(document_id);
    }

    static void ThrowInvalidWord(const string& word) {
        if (!IsValidWord(word)) {
            throw invalid_argument("invalid word --> [" + word + ']');
        }
    }
    void ThrowInvalidStopWords() const {
        for (const string& word : stop_words_) {
            ThrowInvalidWord(word);
        }
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
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

    QueryWord ParseQueryWord(string word) const {
        ThrowInvalidWord(word);
        bool is_minus = false;
        if (word[0] == '-') {
            if (word.substr(0, 2) == "--" || (word.size() == 1)) {
                throw invalid_argument("invalid minus word --> [" + word + ']');
            }
            is_minus = true;
            word = word.substr(1);
        }
        return {word, is_minus, IsStopWord(word)};
    }

    Query ParseQuery(const string& text) const {
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

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return (word_to_document_freqs_.at(word).size())
                ? log(static_cast<double>(GetDocumentCount())/word_to_document_freqs_.at(word).size())
                : 0;
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
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
};

/* ---------------------------- Input & Output ----------------------------- */

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "
         << "document_id = " << document_id << ", "
         << "status = " << static_cast<int>(status) << ", "
         << "words =";
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << '}' << endl;
}

template <typename T, typename S> 
ostream& operator<<(ostream& out, const pair<T, S>& p) {
    out << p.first << ": " << p.second;
    return out;
}

template <typename T>
ostream& Print(ostream& out, const T& container, const string& delimeter = ", ") {
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        } else {
            out << delimeter << element;
        }
    }
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    out << '[';
    Print(out, container);
    return out << ']';
}

template <typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    out << '{';
    Print(out, container);
    return out << '}';
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    return out << '}';
}

ostream& operator<<(ostream& out, const vector<Document>& documents) {
    vector<string> docs_info;
    for (const auto& doc : documents) {
        docs_info.push_back(
            "{ document_id = " + to_string(doc.id) 
          + ", relevance = " + to_string(doc.relevance)
          + ", rating = " + to_string(doc.rating)
          + " }"
        );
    }
    return Print(out, docs_info, "\n"s);
}

/* ------------------------ SearchServer unit tests ------------------------ //

#include "gtest/gtest.h" //TODO: Make unit tests using Gtest

SearchServer CreateServerWithDocuments() {
    SearchServer server;
    server.AddDocument(100, "white stily cat with ball", DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(101, "cat with thin tail  ", DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(102, "dog with sunglasses", DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(103, "snake without tooth", DocumentStatus::IRRELEVANT, {-6, -3, 0, -10});
    server.AddDocument(104, "long fat snake", DocumentStatus::BANNED, {5, -1, 3, -3});
    return server;
}

void TestAddDocument() {
    const vector<Document> found_docs = CreateServerWithDocuments().FindTopDocuments("sunglasses");

    ASSERT_HINT(!found_docs.empty(), "AddDocument() must add documents");
}

void TestSetStopWords() {
    SearchServer server("sunglasses"s);
    server.AddDocument(102, "dog with sunglasses"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    ASSERT_HINT(server.FindTopDocuments("sunglasses").empty(), "SetStopWords() must exclude stop words from a document content");
}

void TestParseQuery() {
    const SearchServer server = CreateServerWithDocuments();

    ASSERT_HINT(server.FindTopDocuments("-sunglasses"s).empty(), "ParseQuery() must exclude a document with a minus word from the result");
}

void TestMatchDocument() {
    const auto& [matched_words, _] = CreateServerWithDocuments().MatchDocument("white cat without tail", 100);
    const string hint = "MatchDocument() must return all words from the search query that are present in the document";

    ASSERT_HINT(count(matched_words.begin(), matched_words.end(), "white"), hint);
    ASSERT_HINT(count(matched_words.begin(), matched_words.end(), "cat"), hint);
    ASSERT_HINT(
        !count(matched_words.begin(), matched_words.end(), "tail"),
        "MatchDocument() mustn't return a word from the search query that are present in the document"
    );
}
void TestMatchDocumentByMinusWord() {
    const auto& [matched_words, _] = CreateServerWithDocuments().MatchDocument("cat with thin -tail"s, 101);

    ASSERT_HINT(matched_words.empty(), "MatchDocument() must return an empty vector if matching at least one minus word"s);
}
void TestMatchDocumentByNoneMatchingWords() {
    const auto& [matched_words, _] = CreateServerWithDocuments().MatchDocument("cat"s, 102);

    ASSERT_HINT(matched_words.empty(), "MatchDocument() mustn't return non empty vector if there is none matching words"s);
}

void TestSorting() {
    vector<int> found_ids;
    for (const Document& doc : CreateServerWithDocuments().FindTopDocuments("cat with tail"s)) {
        found_ids.push_back(doc.id);
    }

    ASSERT_EQUAL_HINT(vector<int>({101, 100, 102}), found_ids, "FindTopDocuments() must sort found documents by decrease of relevance");
}

void TestComputeAverageRating() {
    const vector<Document> found_docs = CreateServerWithDocuments().FindTopDocuments("dog with sunglasses"s);

    ASSERT_EQUAL_HINT(found_docs[0].rating, -1, "ComputeAverageRating() must return an arithmetic mean of the document's ratings"s);
}
void TestComputeAverageRatingWithNoneRatings() {
    SearchServer server;
    server.AddDocument(102, "dog with sunglasses"s, DocumentStatus::ACTUAL, {});
    const vector<Document> found_docs = server.FindTopDocuments("dog with sunglasses"s);

    ASSERT_EQUAL_HINT(found_docs[0].rating, 0, "ComputeAverageRating() must return zero rating if there is no ratings"s);
}

void TestFindTopDocumentsByUserPredicate() {
    auto is_id_even = [](int id, DocumentStatus status, int rating) {
        return id % 2 == 0;
    };
    set<int> found_ids;
    for (const Document& doc : CreateServerWithDocuments().FindTopDocuments("with", is_id_even)) {
        found_ids.insert(doc.id);
    }

    ASSERT_EQUAL_HINT(set<int>({100, 102}), found_ids, "FindTopDocuments() must work with user set predicate");
}

void TestFindTopDocumentsByStatus() {
    const vector<Document> found_docs = CreateServerWithDocuments().FindTopDocuments("snake"s, DocumentStatus::IRRELEVANT);

    ASSERT_EQUAL_HINT(found_docs[0].id, 103, "FindTopDocuments() must work with user set document status");
}

void TestRelevance() {
    map<int, double> id_to_relevance = {
        {100, 0.2*(log(5.0/2.0) + log(5.0/3.0) + log(5.0/1.0))},
        {101, 0.25*(log(5.0/2.0) + log(5.0/3.0))},
        {102, 1.0/3.0*log(5.0/3.0)}
    };
    map<int, double> found_id_to_relevance;
    for (const Document& doc : CreateServerWithDocuments().FindTopDocuments("cat with ball")) {
        found_id_to_relevance[doc.id] = doc.relevance;
    }

    ASSERT_EQUAL_HINT(id_to_relevance, found_id_to_relevance, "FindAllDocuments() must calculate relevance using the TF-ITF method"s);
}

void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestSetStopWords);
    RUN_TEST(TestParseQuery);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestMatchDocumentByNoneMatchingWords);
    RUN_TEST(TestMatchDocumentByMinusWord);
    RUN_TEST(TestSorting);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestComputeAverageRatingWithNoneRatings);
    RUN_TEST(TestFindTopDocumentsByUserPredicate);
    RUN_TEST(TestFindTopDocumentsByStatus);
    RUN_TEST(TestRelevance);
}

// ------------------------------------------------------------------------- */

void AddDocument(SearchServer& search_server,
                 int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Error adding document "s << document_id
             << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Search results for the query : "s << raw_query << endl;
    try {
        cout << search_server.FindTopDocuments(raw_query) << endl;
    } catch (const exception& e) {
        cout << "Search error: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Matching documents for the query : "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Error matching documents for the query : "s << query
             << ": "s << e.what() << endl;
    }
}

int main() {
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);

    return 0;
} 