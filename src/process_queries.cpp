#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries
)
{
    std::vector<std::vector<Document>> found_documents_by_queries(queries.size());
    std::transform(
        std::execution::par,
        queries.begin(),
        queries.end(),
        found_documents_by_queries.begin(),
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(query);
        }
    );
    return found_documents_by_queries;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries
)
{
    std::vector<std::vector<Document>> found_documents_by_queries = ProcessQueries(
        search_server,
        queries
    );

    std::list<Document> joined_documents_by_queries;
    for (const std::vector<Document>& found_documents : found_documents_by_queries)
        joined_documents_by_queries.splice(
            joined_documents_by_queries.end(),
            std::list<Document>{found_documents.begin(), found_documents.end()}
        );

    return joined_documents_by_queries;
}