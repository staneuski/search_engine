#include "document.h"

//Лучше не выносить конструктор по умолчанию и конструктор, который не требует никаких зависимостей, чтобы скрыть их
//размещать в cpp файле. если бы конструктор имел в теле какой то код кроме инициализации это было бы оправдано.
//https://stackoverflow.com/questions/21303/in-c-can-constructor-and-destructor-be-inline-functions

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating)
{
}
