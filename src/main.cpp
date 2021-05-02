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
    search_server.AddDocument(3, "white stily mouse round ball", DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(4, "mouse round thin tail", DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(5, "bird round sunglasses", DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(6, "snake without tooth", DocumentStatus::IRRELEVANT, {5, 3, 4});
    search_server.AddDocument(7, "long fat snake", DocumentStatus::BANNED, {5, -1, 3, -3});

    // [(== 2) --> False] дубликат документа 2, будет удалён
    search_server.AddDocument(8, "funny pet with curly hair", DocumentStatus::ACTUAL, {3, 0});

    // [(~= 2) --> False] отличие только в стоп-словах, считаем дубликатом
    search_server.AddDocument(9, "funny pet and curly hair", DocumentStatus::ACTUAL, {2, 3, -1});

    // [(~= 1) --> False] множество слов такое же
    search_server.AddDocument(10, "funny funny pet and nasty nasty rat", DocumentStatus::ACTUAL, {0, 3});

    // [(!= 11) --> True] добавились новые слова, дубликатом не является
    search_server.AddDocument(11, "funny pet and not very nasty rat", DocumentStatus::ACTUAL, {1, 1, 2});

    // [(~= 11) --> False] множество слов такое же, несмотря на другой порядок, считаем дубликатом
    search_server.AddDocument(12, "very nasty rat and not very funny pet", DocumentStatus::ACTUAL, {3});

    // [(< 11) --> True] есть не все слова, не является дубликатом
    search_server.AddDocument(13, "pet with rat and rat and rat", DocumentStatus::ACTUAL, {1, 0, -1});

    // [(!= 1)(!= 2)(!= 11) --> True] слова из разных документов, не является дубликатом
    search_server.AddDocument(14, "nasty rat with curly hair", DocumentStatus::ACTUAL, {3, 2});
}

int main() {
    using namespace std;

    SearchServer search_server("and with"s);
    AddDocuments(search_server);

    cout << "ACTUAL by default:" << endl;
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s))
       cout << document << endl;

    cout << "BANNED (seq):" << endl;
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "long snake"s, DocumentStatus::BANNED))
       cout << document << endl;

    cout << "Even ids (par):" << endl;
    auto is_id_even = [](int document_id,
                         __attribute__((unused)) DocumentStatus status,
                         __attribute__((unused)) int rating) {
        return document_id % 2 == 0;
    };
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"sv, is_id_even))
       cout << document << endl;

    return 0;
}