#include "search_server.h"

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(
    int document_id
) const {
    const static std::map<std::string_view, double> document_to_empty_freqs;
    return document_to_word_freqs_.count(document_id)
           ? document_to_word_freqs_.at(document_id)
           : document_to_empty_freqs;
}

void SearchServer::AddDocument(
    int document_id,
    const std::string_view& text,
    DocumentStatus status,
    const std::vector<int>& ratings
)
{
    if (documents_.count(document_id))
        throw std::invalid_argument(
            "already used id --> " + std::to_string(document_id)
        );
    else if (document_id < 0)
        throw std::invalid_argument(
            "negative document id --> " + std::to_string(document_id)
        );

    const std::vector<std::string> words = ThrowInvalidWords(
        SplitIntoWordsNoStop(text)
    );
    const double inv_word_count = 1.0/words.size();
    for (const std::string& word : words)
        word_to_document_freqs_[word][document_id] += inv_word_count;

    documents_.emplace(document_id, DocumentData(words, status, ratings));
    documents_ids_.push_back(document_id);

    UpdateInverseDocumentFreqs();
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(
    const std::string_view& text
) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text))
        if (!IsStopWord(word))
            words.push_back(word);
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.size()) {
        int rating_sum = 0;
        for (const int rating : ratings)
            rating_sum += rating;
        return rating_sum/static_cast<int>(ratings.size());
    } else {
        return 0;
    }
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
    bool is_minus = (word[0] == '-');
    if (is_minus)
        word = word.substr(1);
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ThrowInvalidQuery(const Query& query) {
    ThrowInvalidWords(query.plus_words);
    ThrowInvalidWords(query.minus_words);
    ThrowInvalidWords(
        query.minus_words,
        [](const std::string& word){ return word[0] == '-' || word.empty(); }
    );
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(
    const std::string& word
) const
{
    return (word_to_document_freqs_.at(word).size())
            ? log(static_cast<double>(GetDocumentCount())/word_to_document_freqs_.at(word).size())
            : 0;
}

std::vector<Document> SearchServer::GetMatchedDocuments(
    std::map<int, double> document_to_relevance
) const
{
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance)
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
        });
    return matched_documents;
}