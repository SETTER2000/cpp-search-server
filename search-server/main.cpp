#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <numeric>

using namespace std;
//https://practicum.yandex.ru/trainer/cpp/lesson/acde8277-00df-49d7-b5dc-f24df945227d/task/1eb3e274-a62a-4701-bc33-11019f96eea2/?hideTheory=1

const int MAX_RESULT_DOCUMENT_COUNT = 5;


string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine(); // пропуск пустой строки после cin
    return result;
}

vector<int> ReadLineRating() {
    int num_rating = 0;
    vector<int> rating;
    cin >> num_rating; // число оценок (первое число в строке ввода)
    for (int i = 0; i < num_rating; i++) {
        int f = 0; // оценка
        cin >> f;
        rating.push_back(f);
    }
    ReadLine();
    return rating;
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
    int rating;
};

class SearchServer {
public:
    void SetStopWords(const string &text) {
        for (const string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, const vector<int> &ratings) {
        // документ очищенный от стоп-слов
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_ratings_.emplace(document_id, ComputeAverageRating(ratings));
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
    int document_count_ = 0;
    set<string> stop_words_;
    map<int, int> document_ratings_;
    map<string, map<int, double>> word_to_document_freqs_;

    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
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
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

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

    double ComputeWordInverseDocumentFreq(const string &word) const {
        return log(document_count_ * 1.0 / word_to_document_freqs_.at(word).size());
    }

    vector<Document> FindAllDocuments(const Query &query) const {
        map<int, double> document_to_relevance;
        for (const string &word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        for (const string &word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.push_back({document_id, relevance, document_ratings_.at(document_id)});
        }
        return matched_documents;
    }

    static int ComputeAverageRating(const vector<int> &ratings) {
        /*
        * Первая цифра — это количество оценок. Считайте их, передайте в AddDocument в
        * виде вектора целых чисел и вычислите средний рейтинг документа,
        * разделив суммарный рейтинг на количество оценок.
        * Рейтинг документа, не имеющего оценок, равен нулю.
        * */
        int rating = ratings.size();

        if (!rating) return rating;

        return accumulate(ratings.begin(), ratings.end(), 0) / rating;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();

    for (int document_id = 0; document_id < document_count; ++document_id) {
        string document = ReadLine();
        vector<int> rating = ReadLineRating();
        search_server.AddDocument(document_id, document, rating);
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine(); // это запрос считывается не документ! бл... 😝

    for (const auto &[document_id, relevance, rating]: search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "s
             << "relevance = "s << relevance << ", "s
             << "rating = "s << rating << " }"s << endl;
    }
}