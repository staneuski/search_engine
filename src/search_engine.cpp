#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

//1. Поправьте стилистические замечания. Старайтесь не записывать одну логическую еденицу в несколько строк, если это возможно
//2. Подумайте над функцией DropElementsByValue
//3. Скомпонуйте объекты в классе

//Работа сделана хорошо. все замечания по стилистике кода. Нужно исправить. Сейчас вы не сдали, но нужно всегда помнить, что чистый код делает
// его более выразительным, удобочитаемым и лучше сопровождаемым. Жду ваших изменений. Удачи!

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

//Имеет ли смысл создавать функцию, которую вы используете только в одном месте.
//Так же вы копируете вектор при передаче в функцию, что не есть хорошо.
template <typename T>
vector<T> DropElementsByValue(vector<T> v, const T& value) {
    //Используйте std::remove_if
    //https://ps-group.github.io/cxx/cxx_remove_if
    v.erase(remove(v.begin(), v.end(), value), v.end());
    return v;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (text.size() == 0) {
            break;
        }
        if (c == ' ') {
            words.push_back(word);
            //Используйте метод clear
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return DropElementsByValue(words, ""s);
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    //Если вы записываете аргументы в несколько строк, то каждый аргумент должен быть в отдельной строке.
    //Пример:
    //void myFunc(
    //              int first
    //              ,int second
    //              ,int third
    //            )
    //{
    //      ...function body...
    //}
    void AddDocument(int document_id, const string& document,
                     DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, 
            //Лучше второй аргумент записать в одну строку. Лучше будет читаться
            DocumentData{
                ComputeAverageRating(ratings), 
                status
            });
    }

    //Те же замечания что и в функции выше
    vector<Document> FindTopDocuments(const string& raw_query,
                                      DocumentStatus status_to_find
                                          = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(
            raw_query,
            [status_to_find](int document_id, DocumentStatus status, int rating) {
                return status == status_to_find;
            }
        );
    }

    //Шаблонные методы должны располагаться в конце блока
    //Те же замечания по расположению аргументов. Тут лучше записать в одну строку
    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                //Попробуйте использовать тернарный опреатор
                //https://ravesli.com/urok-41-sizeof-zapyataya-i-uslovnyj-ternarnyj-operator/
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            //Данное условие можно вынести в функцию, так как используется несколько раз
            //Что ниубдь IsContainWord
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            //Данное условие можно вынести в функцию, так как используется несколько раз
            //Что ниубдь IsWordContainId
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        //Это исключительно для вас.
        //В данном случае нет необходимости сравнивать резуьтат с нулем, т.к. в C++ true != 0, а false = 0
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        //Что будет если коллекция будет пустая?
        return rating_sum / static_cast<int>(ratings.size());
    }

    //Сгруппируйте структуры в одном месте
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            // Можно сделать инверсию условия и избавиться от двойной вложенности
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(static_cast<double>(documents_.size())
                   /word_to_document_freqs_.at(word).size());
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query,
                                      KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            //Лучше записать в одну строку
            const double inverse_document_freq
                = ComputeWordInverseDocumentFreq(word);
            //Лучше записать в одну строку
            for (const auto [document_id, term_freq]
                    : word_to_document_freqs_.at(word)) {
                //Закрывающую скобку на отдельную строку и открывающую кобку условия, тоже на отдельную строку
                if (key_mapper(document_id,
                               documents_.at(document_id).status,
                               documents_.at(document_id).rating)) {
                    //Лучше записать в одну строку
                    document_to_relevance[document_id]
                        += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            //Лучше записать в одну строку
            for (const auto [document_id, _]
                    : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};

// ===========================================================================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(
        0, "белый кот и модный ошейник"s,
        DocumentStatus::ACTUAL, {8, -3}
    );
    search_server.AddDocument(
        1, "пушистый кот пушистый хвост"s,
        DocumentStatus::ACTUAL, {7, 2, 7}
    );
    search_server.AddDocument(
        2, "ухоженный пёс выразительные глаза"s,
        DocumentStatus::ACTUAL, {5, -12, 2, 1}
    );

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document
            : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "ACTUAL:"s << endl;
    for (const Document& document
            : search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                             DocumentStatus::ACTUAL)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    //Написано красиво, но читать очень сложно. Где тут лямбда, где условие, где тело.
    //Лучше вынесите лямбду в переменную, это не критично и код станет понятней.
    for (const Document& document
            : search_server.FindTopDocuments(
                "пушистый ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) {
                    return document_id % 2 == 0;
                }
            )
    ) {
        PrintDocument(document);
    }

    cout << "Positive rating only:"s << endl;
    //Тоже что и выше
    for (const Document& document
            : search_server.FindTopDocuments(
                "пушистый  ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) {
                    return rating > 0;
                }
            )
    ) {
        PrintDocument(document);
    }
}