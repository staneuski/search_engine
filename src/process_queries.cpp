#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries
)
{
    std::vector<std::vector<Document>> processed_queries(queries.size());
    std::transform(
        std::execution::par,
        queries.begin(),
        queries.end(),
        processed_queries.begin(),
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(query);
        }
    );
    return processed_queries;
}