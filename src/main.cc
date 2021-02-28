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
#include "request_queue/request_queue.h"

using namespace std;

/* ----------------------------- Search Server ----------------------------- */

/* ----------------------------- Request Queue ----------------------------- */

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

void AddDocuments(SearchServer& search_server) {
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {4, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog and rat Vladimir"s, DocumentStatus::ACTUAL, {1, -3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(6, "small dog Jack-Russell terier"s, DocumentStatus::ACTUAL, {5, 3, 4});
    search_server.AddDocument(7, "dog Siberian hasky"s, DocumentStatus::ACTUAL, {2, 1, 3});
}

int main() {
    /* ----------------------------- Paginate ------------------------------ */
    {
        SearchServer search_server("and at on in"s);
        AddDocuments(search_server);
        const vector<Document> search_results = search_server.FindTopDocuments("curly dog"s);
        const auto pages = Paginate(search_results, 2);

        for (auto page = pages.begin(); page != pages.end(); ++page) {
            cout << *page << endl;
            cout << "Page break"s << endl;
        }
    }


    /* --------------------------- Request Queue --------------------------- */
    {
        SearchServer search_server("and at on in"s);
        RequestQueue request_queue(search_server);
        AddDocuments(search_server);

        for (int i = 0; i < 1439; ++i) {
            request_queue.AddFindRequest("empty request"s);
        }
        request_queue.AddFindRequest("curly dog"s);
        request_queue.AddFindRequest("big collar"s);
        request_queue.AddFindRequest("sparrow"s);

        cout << "Total empty requests: "s
             << request_queue.GetNoResultRequests() << endl;
    }


    return 0;
}