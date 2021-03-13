#include <iostream>
#include <string>
#include <vector>

#include "iostream_helpers.h"
#include "log_duration.h"
#include "paginator.h"
#include "request_queue.h"
#include "search_server.h"

void AddDocuments(SearchServer& search_server) {
    search_server.AddDocument(1, "funny pet and nasty rat", DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair", DocumentStatus::ACTUAL, {4, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair", DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog and rat Vladimir", DocumentStatus::ACTUAL, {1, -3, 2});
    search_server.AddDocument(5, "big dog hamster Borya", DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(6, "small dog Jack-Russell terier", DocumentStatus::ACTUAL, {5, 3, 4});
    search_server.AddDocument(7, "dog Siberian hasky", DocumentStatus::ACTUAL, {2, 1, 3});
}

int main() {
    using namespace std;

    /* ----------------------------- Paginate ------------------------------ */
    {
        LOG_DURATION("curly dog", cerr);
        SearchServer search_server("and at on in"s);
        AddDocuments(search_server);
        const vector<Document> search_results = search_server.FindTopDocuments(
            "curly dog"s
        );
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
        auto is_id_even = [](int document_id, DocumentStatus status, int rating) {
            return document_id % 2 == 0;
        };
        request_queue.AddFindRequest("curly dog"s, is_id_even);
        request_queue.AddFindRequest("big collar"s);
        request_queue.AddFindRequest("sparrow"s);

        cout << "Total empty requests: "s
             << request_queue.GetNoResultRequests() << endl;
    }


    return 0;
}