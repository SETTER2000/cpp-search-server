#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>


using namespace std;

// https://practicum.yandex.ru/trainer/cpp/lesson/db917dd8-0bbf-4c25-ba23-474534ccc4aa/task/80b48a36-4d57-46ed-a783-788207e516e8/?hideTheory=1
// авторское решение: https://pastebin.com/VHJ91CJt
const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c: text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string &text) {
        for (const string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document) {
        /**
         *  Добавляет документ в db (типа db)
         *  Рассчитайте TF слова кот в документе 1.
         *  Всего слов в этом документе четыре, из них кот — только одно.
         *  1 / 4 = 0,25.
         *  А слово пушистый встречается дважды, так что его TF в этом документе
         *  будет 2 / 4 = 0,5.
         *  Вычисляется TF каждого слова запроса в документе
         *  сколько раз встречается слово в документе / всего слов в документе
         */
        // документ очищенный от стоп-слов
        const vector<string> words = SplitIntoWordsNoStop(document);
        Document doc;
        doc.id = document_id;
        for (const auto &word: words) {
            // Сколько раз встречается слово в документе
            doc.relevance = count_if(
                    words.begin(), words.end(), [&word](const auto &s) {
                        return s == word;
                    });
            word_to_document_freqs_[word].insert(
                    {doc.id, doc.relevance / words.size()});
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    // в поисковой системе количество документов
    int document_count_ = 0;

    // word_to_documents - в нём будет храниться инвертированный индекс документов.
    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    // Вычислите IDF слова кот. Оно встречается в двух документах из трёх: 0 и 1
    // 3 / 2 = 1,5
    // вычисляется IDF каждого слова в запросе
    map<int, double> IDFQuery(const Query &query) const {
        double idf_word = 0;
        map<int, double> documents_relevant;

        /**
         * https://practicum.yandex.ru/trainer/cpp/lesson/db917dd8-0bbf-4c25-ba23-474534ccc4aa/task/80b48a36-4d57-46ed-a783-788207e516e8/?hideTheory=1
         * Рассчитывают IDF так:
         * Количество всех документов делят на количество тех, где встречается слово.
         * Не встречающиеся нигде слова в расчёт не берут, поэтому деления на ноль опасаться не надо.
         * Важно, встречается ли слово в документе. А сколько раз встречается — всё равно.
         * К результату деления применяют логарифм — функцию log из библиотеки <cmath>.
         */
        for (const auto &plus_word: query.plus_words) {
            if (word_to_document_freqs_.count(plus_word)) {
                map document_ids = word_to_document_freqs_.at(plus_word);
                // К результату деления применяют логарифм — функцию log из библиотеки <cmath>
                // это IDF текущего слова
                // Количество всех документов делят на количество документов, где встречается
                idf_word = log(document_count_ / double(document_ids.size()));
                // IDF каждого слова запроса умножается на TF этого слова в этом документе,
                // все произведения IDF и TF в документе суммируются.
                for (const auto &document: document_ids) {
                    documents_relevant[document.first] += (idf_word * document.second);
                }
            }
        }
        return documents_relevant;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const {
        vector<string> words;
        for (const string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    Query ParseQuery(const string &text) const {
        Query query;
        for (const string &word: SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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

    vector<Document> FindAllDocuments(const Query &query) const {
        // релевантные документы
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        for (const auto &relevance: IDFQuery(query)) {
            matched_documents.push_back({relevance.first, double(relevance.second)});
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (const auto &[document_id, relevance]: search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}