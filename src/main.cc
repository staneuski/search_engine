#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "document/document.h"
#include "read_input_functions/read_input_functions.h"

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
        remove_if(words.begin(), words.end(),
                  [](const string& word) { return word.empty(); }),
        words.end()
    );
    return words;
}

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

class SearchServer {
public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(ThrowInvalidWords(MakeUniqueNonEmptyStrings(stop_words)))
    {
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        return documents_ids_.at(index);
    }

    void AddDocument(int document_id, const string& document,
                     DocumentStatus status, const vector<int>& ratings) {
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

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
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
        const Query query = ThrowInvalidQuery(ParseQuery(raw_query));
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
        bool is_minus = (word[0] == '-');
        if (is_minus) {
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

    static Query ThrowInvalidQuery(const Query& query) {
        ThrowInvalidWords(query.plus_words);
        ThrowInvalidWords(query.minus_words);
        ThrowInvalidWords(
            query.minus_words,
            [](const string& word){ return word[0] == '-' || word.empty(); }
        );
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return (word_to_document_freqs_.at(word).size())
                ? log(static_cast<double>(GetDocumentCount())/word_to_document_freqs_.at(word).size())
                : 0;
    }

    template <typename StringContainer>
    static StringContainer ThrowInvalidWords(const StringContainer& words) {
        return ThrowInvalidWords(
            words,
            [](const string& word){ return !IsValidWord(word); }
        );
    }

    template <typename StringContainer, typename WordPredicate>
    static StringContainer ThrowInvalidWords(const StringContainer& words,
                                             WordPredicate word_predicate) {
        for (const string& word : words) {
            if (word_predicate(word)) {
                throw invalid_argument("invalid word --> [" + word + ']');
            }
        }
        return words;
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

/* ----------------------------- Request Queue ----------------------------- */

template <typename InputIt>
class IteratorRange {
public:
    IteratorRange(const InputIt begin, const InputIt end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    InputIt begin() const {
        return first_;
    }

    InputIt end() const {
        return last_;
    }

private:
    InputIt first_, last_;
    size_t size_;
};

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
    {
    }

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query,
                                    DocumentPredicate document_predicate) {
        const vector<Document>& found_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
        UpdateRequestQueue(found_documents.empty());
        return found_documents;
    }

    vector<Document> AddFindRequest(const string& raw_query,
                                    DocumentStatus status) {
        const vector<Document>& found_documents = search_server_.FindTopDocuments(raw_query, status);
        UpdateRequestQueue(found_documents.empty());
        return found_documents;
    }

    vector<Document> AddFindRequest(const string& raw_query) {
        const vector<Document> found_documents = search_server_.FindTopDocuments(raw_query);
        UpdateRequestQueue(found_documents.empty());
        return found_documents;
    }

    int GetNoResultRequests() const {
        return no_result_count_;
    }

private:
    const SearchServer& search_server_;
    deque<bool> requests_;
    int no_result_count_ = 0;
    const static int sec_in_day_ = 1440;

    void UpdateRequestQueue(const bool is_empty) {
        if (is_empty) {
            ++no_result_count_;
        }

        requests_.push_back(is_empty);
        if (requests_.size() > sec_in_day_) {
            if (requests_.front()) {
                --no_result_count_;
            }
            requests_.pop_front();
        }
    }
};


/* ------------------------------- Padinate -------------------------------- */

template <typename InputIt>
class Paginator {
public:
    Paginator(const InputIt begin, const InputIt end, const size_t page_size) {
        InputIt page_end = begin;
        while (distance(page_end, end) > 0) {
            InputIt page_begin = page_end;
            advance(page_end, page_size);
            pages_.push_back({page_begin, page_end});
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    vector<IteratorRange<InputIt>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}


/* ---------------------------- Input & Output ----------------------------- */

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

ostream& operator<<(ostream& out, const Document& document) {
    out << "{ document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating
        << " }";
    return out;
}

template <typename InputIt>
ostream& operator<<(ostream& out, const IteratorRange<InputIt> iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        out << *it;
    }
    return out;
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

/* ------------------------------------------------------------------------- */

int main() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {4, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog and rat Vladimir"s, DocumentStatus::ACTUAL, {1, -3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(6, "small dog Jack-Russell terier"s, DocumentStatus::ACTUAL, {5, 3, 4});
    search_server.AddDocument(7, "dog Siberian hasky"s, DocumentStatus::ACTUAL, {2, 1, 3});

    const vector<Document> search_results = search_server.FindTopDocuments("curly dog"s);
    const auto pages = Paginate(search_results, 2);

    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }

    return 0;
}