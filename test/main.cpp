#include <gtest/gtest.h>

#include "paginator.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"

using namespace std::string_literals;

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


/* ----------------------------- SearchServer ------------------------------ */

TEST(SearchServer, SearchServer) {
    SearchServer search_server("and with"s);
    AddDocuments(search_server);

    ASSERT_TRUE(search_server.FindTopDocuments("with").empty())
        << "SetStopWords() must exclude stop words from a document content";
}

TEST(SearchServer, AddDocument) {
    SearchServer search_server;
    AddDocuments(search_server);

    ASSERT_FALSE(search_server.FindTopDocuments("funny").empty())
        << "AddDocument() must add documents";
}

TEST(SearchServer, ParseQuery) {
    SearchServer search_server;
    AddDocuments(search_server);

    ASSERT_TRUE(search_server.FindTopDocuments("-terier").empty())
        << "ParseQuery() must exclude a document with a minus word from the result";
}

TEST(SearchServer, MatchDocument) {
    SearchServer search_server;
    AddDocuments(search_server);

    const auto& [matched_words, _] = search_server.MatchDocument(
        "funny pet nasty rat with tail", 1
    );
    const std::string hint = "MatchDocument() must return all words from the search query that are present in the document";

    ASSERT_TRUE(count(matched_words.begin(), matched_words.end(), "funny")) << hint;
    ASSERT_TRUE(count(matched_words.begin(), matched_words.end(), "rat")) << hint;
    ASSERT_FALSE(count(matched_words.begin(), matched_words.end(), "tail"))
        << "MatchDocument() mustn't return a word from the search query"
        << "that are present in the document";
}

TEST(SearchServer, MatchDocumentByMinusWord) {
    SearchServer search_server;
    AddDocuments(search_server);

    const auto& [matched_words, _] = search_server.MatchDocument(
        "rat -Borya", 4
    );

    ASSERT_TRUE(matched_words.empty())
        << "MatchDocument() must return an empty vector"
        << "if matching at least one minus word";
}

TEST(SearchServer, MatchDocumentByNoneMatchingWords) {
    SearchServer search_server;
    AddDocuments(search_server);

    const auto& [matched_words, _] = search_server.MatchDocument("cat", 7);

    ASSERT_TRUE(matched_words.empty())
        << "MatchDocument() mustn't return non empty vector"
        << "if there is none matching words";
}

TEST(SearchServer, Sorting) {
    SearchServer search_server;
    AddDocuments(search_server);

    std::vector<int> found_ids;
    for (const Document& doc : search_server.FindTopDocuments("mouse round tail")) {
        found_ids.push_back(doc.id);
    }

    ASSERT_EQ(std::vector<int>({4, 3, 5}), found_ids)
        << "FindTopDocuments() must sort found documents by decrease of relevance";
}

TEST(SearchServer, FindTopDocumentsByUserPredicate) {
    SearchServer search_server;
    AddDocuments(search_server);

    const std::vector<Document> found_docs = search_server.FindTopDocuments(
        "snake",
        DocumentStatus::IRRELEVANT
    );

    ASSERT_EQ(found_docs[0].id, 6)
        << "FindTopDocuments() must work with user set document status";
}

TEST(SearchServer, TF_ITF) {
    SearchServer search_server;
    search_server.AddDocument(100, "white stily cat with ball", DocumentStatus::ACTUAL, {0});
    search_server.AddDocument(101, "cat with thin tail", DocumentStatus::ACTUAL, {0});
    search_server.AddDocument(102, "dog with sunglasses", DocumentStatus::ACTUAL, {0});
    search_server.AddDocument(103, "snake without tooth", DocumentStatus::IRRELEVANT, {0});
    search_server.AddDocument(104, "long fat snake", DocumentStatus::BANNED, {0});

    std::map<int, double> id_to_found_relevance;
    for (const Document& doc : search_server.FindTopDocuments("cat with ball")) {
        id_to_found_relevance[doc.id] = doc.relevance;
    }

    std::map<int, double> id_to_expected_relevance = {
        {100, 0.2*(log(5.0/2.0) + log(5.0/3.0) + log(5.0/1.0))},
        {101, 0.25*(log(5.0/2.0) + log(5.0/3.0))},
        {102, 1.0/3.0*log(5.0/3.0)}
    };

    ASSERT_EQ(id_to_expected_relevance, id_to_found_relevance)
        << "FindAllDocuments() must calculate relevance using the TF-ITF method";
}

TEST(SearchServer, GetWordFrequencies) {
    SearchServer search_server("fat"s);
    AddDocuments(search_server);

    std::vector<std::string> found_words;
    std::vector<double> found_freqs;
    for (const auto& [word, freq] : search_server.GetWordFrequencies(7)) {
        found_words.push_back(word);
        found_freqs.push_back(freq);
    }

    ASSERT_EQ(std::vector<std::string>({"long", "snake"}), found_words);
    ASSERT_EQ(
        std::vector<double>({2.6390573296152584, 1.9459101490553132}),
        found_freqs
    );
}

// /* ---------------------------- RemoveDuplicates --------------------------- */

TEST(RemoveDuplicates, RemoveDuplicates) {
    SearchServer search_server;
    AddDocuments(search_server);
    // RemoveDuplicates(search_server);

    ASSERT_EQ(search_server.GetDocumentCount(), 10)
        << "Duplicated documents must be excluded from the search server";
}


/* ------------------------------- Paginator ------------------------------- */

TEST(Paginator, Paginator) {
    SearchServer search_server;
    AddDocuments(search_server);

    const std::vector<Document> found_docs = search_server.FindTopDocuments(
        "mouse bird round"
    );

    const auto it = *Paginator(begin(found_docs), end(found_docs), 2).begin();

    EXPECT_EQ(it.end() - it.begin(), 2);
}

TEST(Paginator, Paginate) {
    SearchServer search_server;
    AddDocuments(search_server);

    const std::vector<Document> found_docs = search_server.FindTopDocuments(
        "mouse bird round"
    );

    const auto it = *Paginate(found_docs, 2).begin();

    EXPECT_EQ(it.end() - it.begin(), 2);
}


/* ----------------------------- Request Queue ----------------------------- */

TEST(RequestQueue, RequestQueue) {
    SearchServer search_server;
    RequestQueue request_queue(search_server);
    AddDocuments(search_server);

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request");
    }
    request_queue.AddFindRequest("sunglasses");
    request_queue.AddFindRequest("round");
    request_queue.AddFindRequest("bird");

    EXPECT_EQ(request_queue.GetNoResultRequests(), 1437);
}


int main(int argc, char **argv) {
    SearchServer search_server("and with"s);
    AddDocuments(search_server);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}