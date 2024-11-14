#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

using namespace std;

const double EPSILON = 1e-6;
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

optional<vector<string>> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else if (c >= 0 && c <= 31) {
            return nullopt;
        }
        else {
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id;
    double relevance;
    int rating;
};

struct Query {
    set<string> plus;
    set<string> minus;
};

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template<typename Container>

    explicit SearchServer(const Container& words) {
        for (const auto& word : words) {
            stop_words_.insert(word);
        }
    }
    explicit SearchServer(const string& s) {
        SetStopWords(s);
    }

    void SetStopWords(const string& text) {
        if (const auto words = SplitIntoWords(text)) {
            for (const string& word : words.value()) {
                stop_words_.insert(word);
            }
        }
    }

    [[nodiscard]] bool AddDocument(int document_id, const string& document, DocumentStatus status, vector<int> rating) {
        const auto words_freq = SplitIntoWordsNoStop(document); // map{word,TF}

        //проверка по id_to_rating т.к поиск по map быстрее чем по вектору
        if (document_id < 0 || id_to_rating_.count(document_id) || !words_freq) {
            return false;
        }
        document_ids_.push_back(document_id);
        id_to_rating_[document_id] = ComputeAverageRating(rating);
        id_to_status_[document_id] = status;

        //реализация SplitIntoWordsNoStop изменена, возвращает map{слово,количество повторов слова в документе},
        //поэтому для расчета суммарного количества слов используется accumulate
        int total_words = accumulate(words_freq.value().begin(), words_freq.value().end(), 0, [](int sum, const pair<string, int>& next) {
            return sum + next.second;
            });

        for (const auto& word : words_freq.value()) {
            words_to_docs_with_freq[word.first][document_id] = word.second * 1.0 / total_words;
        }

        ++document_count_;
        return true;
    }

    template <typename Filter>
    optional<vector<Document>> FindTopDocuments(const string& raw_query, Filter conditions) const {
        const auto query_words = ParseQuery(raw_query);
        if (!query_words) {
            return nullopt;
        }

        auto result = FindAllDocuments(query_words.value(), conditions);

        sort(result.begin(), result.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance ||
                    (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
            });

        if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
            result.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return result;
    }
    //status template specification

    optional<vector<Document>> FindTopDocuments(const string& raw_query, DocumentStatus needed_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [needed_status](int document_id, DocumentStatus status, int rating) {
            return status == needed_status;
            });
    }

    int GetDocumentCount() const {
        return document_count_;
    }

    optional<tuple<vector<string>, DocumentStatus>> MatchDocument(const string& raw_query, int document_id) const {
        set<string> matched_words;
        const auto query_words = ParseQuery(raw_query);
        if (!query_words) {
            return nullopt;
        }
        for (const auto& word : query_words.value().plus) {
            if (words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)) {
                matched_words.insert(word);
            }
        }

        for (const auto& word : query_words.value().minus) {
            if (words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)) {
                matched_words.clear();
            }
        }

        vector<string> matched_words_vector;

        for (const auto& i : matched_words) {
            matched_words_vector.push_back(i);
        }

        sort(matched_words_vector.begin(), matched_words_vector.end());

        if (matched_words_vector.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_words_vector.resize(MAX_RESULT_DOCUMENT_COUNT);
        }


        return tuple<vector<string>, DocumentStatus>{matched_words_vector, id_to_status_.at(document_id)};
    }

    int GetDocumentId(int index)const {
        if (index < 0 || index >= static_cast<int>(document_ids_.size())) {
            return INVALID_DOCUMENT_ID;
        }
        return document_ids_[index];
    }

private:

    map<string, map<int, double>> words_to_docs_with_freq; //map{слово,map{id,TF}}

    set<string> stop_words_;

    map<int, int> id_to_rating_;

    map<int, DocumentStatus> id_to_status_;

    vector<int> document_ids_;

    int document_count_ = 0;

    // Existence required
    double CalculateIDF(const string& word) const {
        return log(document_count_ * 1.0 / words_to_docs_with_freq.at(word).size());
    }

    static int ComputeAverageRating(const vector<int>& rating) {
        if (rating.empty()) {
            return 0;
        }
        return accumulate(rating.begin(), rating.end(), 0) / static_cast<int>(rating.size());
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    optional<map<string, int>> SplitIntoWordsNoStop(const string& text) const {
        map<string, int> result;
        if (auto words = SplitIntoWords(text)) {
            for (const string& word : words.value()) {
                if (!IsStopWord(word)) {
                    ++result[word];
                }
            }
            return result;
        }
        else {
            return nullopt;
        }
    }

    optional<Query> ParseQuery(const string& text) const {
        set<string> plus_words;
        set<string> minus_words;
        if (auto words = SplitIntoWordsNoStop(text)) {
            for (const auto& [word, freq] : words.value()) {
                if (word.at(0) == '-') {
                    if (word.size() == 1 || word.at(1) == '-') {
                        return nullopt;
                    }
                    minus_words.insert(word.substr(1)); //минус-слова берутся без знака
                }
                else {
                    plus_words.insert(word);
                }
            }
        }
        else {
            return nullopt;
        }

        return Query{ plus_words,minus_words };
    }


    template <typename Filter>
    vector<Document> FindAllDocuments(const Query& query_words, Filter conditions) const {
        vector<Document> matched_documents;
        map<int, double> potential_documents;

        for (const auto& plus_word : query_words.plus) {
            if (words_to_docs_with_freq.count(plus_word)) {
                for (auto [id, tf] : words_to_docs_with_freq.at(plus_word)) {
                    potential_documents[id] += (tf * CalculateIDF(plus_word));  //relevance = tf * idf
                }
            }

        }

        for (const auto& minus_word : query_words.minus) {
            if (words_to_docs_with_freq.count(minus_word)) {
                for (auto [id, _] : words_to_docs_with_freq.at(minus_word)) {
                    potential_documents.erase(id);
                }
            }
        }

        for (const auto& [id, rel] : potential_documents) {
            if (conditions(id, id_to_status_.at(id), id_to_rating_.at(id))) {
                const int rating = id_to_rating_.at(id);
                matched_documents.push_back({ id,rel,rating });
            }
        }

        return matched_documents;
    }
};
void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    setlocale(LC_ALL, "Russian");
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    }
    if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    }
    if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 })) {
        cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    }
    if (const auto documents = search_server.FindTopDocuments("--пушистый"s)) {
        for (const Document& document : *documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка в поисковом запросе"s << endl;
    }
}