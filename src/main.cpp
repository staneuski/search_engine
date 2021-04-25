#include <execution>
#include <iostream>
#include <string>
#include <vector>

#include "document.h"
#include "iostream_helpers.h"
#include "log_duration.h"
#include "process_queries.h"
#include "search_server.h"

void AddDocuments(SearchServer& search_server) {
    search_server.AddDocument(1, "funny pet and nasty rat", DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair", DocumentStatus::ACTUAL, {4, 2, 3});

    // [(== 2) --> False] дубликат документа 2, будет удалён
    search_server.AddDocument(3, "funny pet with curly hair", DocumentStatus::ACTUAL, {3, 0});

    // [(~= 2) --> False] отличие только в стоп-словах, считаем дубликатом
    search_server.AddDocument(4, "funny pet and curly hair", DocumentStatus::ACTUAL, {2, 3, -1});

    // [(~= 1) --> False] множество слов такое же
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat", DocumentStatus::ACTUAL, {0, 3});

    // [(!= 6) --> True] добавились новые слова, дубликатом не является
    search_server.AddDocument(6, "funny pet and not very nasty rat", DocumentStatus::ACTUAL, {1, 1, 2});

    // [(~= 6) --> False] множество слов такое же, несмотря на другой порядок, считаем дубликатом
    search_server.AddDocument(7, "very nasty rat and not very funny pet", DocumentStatus::ACTUAL, {3});

    // [(< 6) --> True] есть не все слова, не является дубликатом
    search_server.AddDocument(8, "pet with rat and rat and rat", DocumentStatus::ACTUAL, {1, 0, -1});

    // [(!= 1)(!= 2)(!= 6) --> True] слова из разных документов, не является дубликатом
    search_server.AddDocument(9, "nasty rat with curly hair", DocumentStatus::ACTUAL, {3, 2});
}

int main() {
    using namespace std;

    SearchServer search_server("and with"sv);
    AddDocuments(search_server);

    const string query = "curly and funny -not";

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1" << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2" << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 8);
        cout << words.size() << " words for document 8" << endl;
        // 0 words for document 8
    }

    return 0;
}