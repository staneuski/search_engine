#include "output_functions.h"

//Тут тоже не думаю, что имеет смысл раздел¤ть определение с объ¤влением. Лучше все реализовать в h файле.
//Вы также можете вначале определить все функции в h файле, а ниже их объ¤вить, если вам так больше нравитс¤.

void PrintMatchDocumentResult(
    int document_id,
    const std::vector<std::string>& words,
    DocumentStatus status
)
{
    std::cout << "{ "
              << "document_id = " << document_id << ", "
              << "status = " << static_cast<int>(status) << ", "
              << "words =";
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << '}' << std::endl;
}

std::ostream& operator<<(std::ostream& out,
                         const Document& document) {
    out << "{ document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating
        << " }";
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         const std::vector<Document>& documents) {
    using namespace std::string_literals;
    return Print(out, documents, "\n"s);
}