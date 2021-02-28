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
#include "search_server/search_server.h"

using namespace std;

/* ----------------------------- Search Server ----------------------------- */

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