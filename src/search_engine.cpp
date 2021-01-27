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
        remove_if(words.begin(), words.end(), [](const string& word) { return word == ""s; }),
        words.end()
    );

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(
            document_id,
            DocumentData{ComputeAverageRating(ratings),  status}
        );
    }

    bool IsContainWord(const string& word) const {
        return word_to_document_freqs_.count(word);
    }
    bool IsWordContainId(const string& word, const int& document_id) const {
        return word_to_document_freqs_.at(word).count(document_id);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
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

    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

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

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.size()) {
            int rating_sum = 0;
            for (const int rating : ratings) {
                rating_sum += rating;
            }
            return rating_sum / static_cast<int>(ratings.size());
        } else {
            return 0;
        }
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word);
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

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
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
                ? log(static_cast<double>(documents_.size())/word_to_document_freqs_.at(word).size())
                : 0;
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query,
                                      KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (key_mapper(document_id,
                               documents_.at(document_id).status,
                               documents_.at(document_id).rating)
                )
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};

// ------------------------- SearchServer unit tests -------------------------

SearchServer AddDocuments() {
    SearchServer server;
    server.AddDocument(154313, "white stily cat with ball"s, DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(564, "cat with thin tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(36, "dog with sunglasses"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(830, "snake without tooth"s, DocumentStatus::IRRELEVANT, {-6, -3, 0, -10});
    server.AddDocument(659, "long fat snake"s, DocumentStatus::BANNED, {5, -1, 3, -3});
    return server;
}

void TestAddDocument() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {-1, 2, 0});
    server.AddDocument(911, "chilling dog in the city"s, DocumentStatus::ACTUAL, {3, 2, 5});
    const vector<Document> found_docs = server.FindTopDocuments("cat in the city"s);
    assert(found_docs.size() == 2);
    assert(found_docs[0].id == 42);
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        assert(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        assert(doc0.id == doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("in"s).empty());
    }
}

void TestExcludeDocumentsWithMinusWords() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {-1, 2, 0});
    assert(server.FindTopDocuments("in"s).empty() == 0);
    assert(server.FindTopDocuments("-in"s).empty());
}

void TestMatchDocument() {
    SearchServer server = AddDocuments();
    {
        const auto& [matched_words, _] = server.MatchDocument("white cat without tail"s, 154313);
        assert(count(matched_words.begin(), matched_words.end(), "white"s));
        assert(count(matched_words.begin(), matched_words.end(), "cat"s));
        assert(!count(matched_words.begin(), matched_words.end(), "tail"s));
    }
    {
        const auto& [matched_words, _] = server.MatchDocument("cat with thin -tail"s, 564);
        assert(matched_words.empty());
    }
    {
        const auto& [matched_words, _] = server.MatchDocument("cat"s, 36);
        assert(matched_words.empty());
    }
}

void TestSorting() {
    SearchServer server = AddDocuments();
    vector<int> expected_ids = {564, 154313, 36};
    vector<int> found_ids;
    for (const Document& doc : server.FindTopDocuments("cat with tail"s)) {
        found_ids.push_back(doc.id);
    }
    assert(equal(expected_ids.begin(), expected_ids.end(), found_ids.begin()));
}

void TestComputeAverageRating() {
    {
        SearchServer server;
        server.AddDocument(36, "dog with sunglasses"s, DocumentStatus::ACTUAL, {});
        const vector<Document> found_docs = server.FindTopDocuments("dog with sunglasses"s);
        assert(found_docs[0].rating == 0);
    }
    {
        SearchServer server;
        server.AddDocument(36, "dog with sunglasses"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        const vector<Document> found_docs = server.FindTopDocuments("dog with sunglasses"s);
        assert(found_docs[0].rating == -1);
    }
}

void TestUserPredicates() {
    SearchServer server = AddDocuments();
    auto is_id_even = [](int id, DocumentStatus status, int rating) {
        return id % 2 == 0;
    };
    const vector<Document>& found_docs = server.FindTopDocuments("with"s, is_id_even);
    for (const Document& doc : found_docs) {
        assert(doc.id % 2 == 0);
    }
}

void TestFindByStatus() {
    SearchServer server = AddDocuments();
    const vector<Document>& found_docs = server.FindTopDocuments("snake"s, DocumentStatus::IRRELEVANT);
    assert(found_docs[0].id == 830);
}

void TestRelevance() {
    SearchServer server = AddDocuments();
    vector<int> expected_relevances = {607310, 356779, 170275};
    vector<int> found_relevances;
    for (const Document& doc : server.FindTopDocuments("cat with ball"s)) {
        found_relevances.push_back(doc.relevance*1e6);
    }
    assert(equal(expected_relevances.begin(), expected_relevances.end(), found_relevances.begin()));
}

void TestSearchServer() {
    TestAddDocument();
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeDocumentsWithMinusWords();
    TestMatchDocument();
    TestSorting();
    TestComputeAverageRating();
    TestUserPredicates();
    TestFindByStatus();
    TestRelevance();
}

// ---------------------------------------------------------------------------

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;

    return 0;
}