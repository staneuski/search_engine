#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"
#include "search_server.h"
#include "string_processing.h"

using namespace std;

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server,
          ExecutionPolicy&& policy) {
    LOG_DURATION_STDERR(mark);

    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id)
        search_server.RemoveDocument(policy, id);

    cout << search_server.GetDocumentCount() << endl;
}

#define TEST(mode) Test(#mode, search_server, execution::mode)

int main() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 1000, 100);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i)
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});

    TEST(seq);
    TEST(par);
}