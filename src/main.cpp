#include <iostream>
#include <string>
#include <vector>

//1. Вы переносите в cpp все определения, кроме шаблонных, это не всегда правильно. Норма, это когда 
//   методы с телом в одну строчку размещаются в хэдерах. Так же конструкторы по умолчания и конструкторы без тела
//   могут размещаться в хэдере. 
//2. С функциями, если функции не большие и их немного и они не требуют зависимостей, которые нужно скрыть,
//   как в вашем случае, то их можно размещать в хэдере
//3. Используйте noexcept там где это нужно
//4. Используйте inline там где это нужно
//5. Так же для каждого h и cpp создавать по папке это overflow. Представтьте что у вас в проекте 30 тысяч файлов с кодом.
//   Если вы будете делать так как вы сейчас сделаете вы ужаснетесь от вашей вложенности папок. Их обычно делять по группам.
//   Понятно, что на таком количестве и так вложенность будет большая, но ваш подход это усугубит. Я не рекомендую так делать.

//Не зачет. Нужно доработать. Удачи!

#include "iostream_helpers.h"
#include "paginator/paginator.h"
#include "request_queue/request_queue.h"
#include "search_server/search_server.h"

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