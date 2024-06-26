#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(
    const std::string& raw_query,
    DocumentStatus status
)
{
    const std::vector<Document>& found_documents = search_server_.FindTopDocuments(raw_query, status);
    UpdateRequestQueue(found_documents.empty());
    return found_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const std::vector<Document> found_documents = search_server_.FindTopDocuments(raw_query);
    UpdateRequestQueue(found_documents.empty());
    return found_documents;
}

void RequestQueue::UpdateRequestQueue(const bool is_empty) {
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