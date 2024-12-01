#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <sstream>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

using namespace std;

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const char FIRST_SPECIAL_SYMBOL = 0;
const char LAST_SPECIAL_SYMBOL = 31;

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

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else if (c >= FIRST_SPECIAL_SYMBOL && c <= LAST_SPECIAL_SYMBOL) {
            throw invalid_argument("Invalid char in query/stop words");
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
    int id = 0;
    double relevance = 0;
    int rating = 0;
};

struct Query {
    set<string> plus;
    set<string> minus;
};

class SearchServer {
public:
    template<typename Container>
    explicit SearchServer(const Container& words) {
        for (const auto& word : words) {
            for (auto c : word) {
                if (c >= FIRST_SPECIAL_SYMBOL && c <= LAST_SPECIAL_SYMBOL) {
                    throw invalid_argument("Invalid char in stop-words collection");
                }
            }
            stop_words_.insert(word);
        }
    }

    explicit SearchServer(const string& s) {
        SetStopWords(s);
    }

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, vector<int> rating) {
        const auto words_freq = SplitIntoWordsNoStop(document); // map{word,TF}


        if (document_id < 0) {
            throw invalid_argument("Try to add document with negative id");
        }
        //проверка по id_to_rating т.к поиск по map быстрее чем по вектору
        if (id_to_rating_.count(document_id)) {
            throw invalid_argument("Try to add document with existing id");
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
    }

    template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter conditions) const {
        const auto query_words = ParseQuery(raw_query);

        auto result = FindAllDocuments(query_words, conditions);

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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus needed_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [needed_status](int document_id, DocumentStatus status, int rating) {
            return status == needed_status;
            });
    }

    int GetDocumentCount() const {
        return document_count_;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        set<string> matched_words;
        const auto query_words = ParseQuery(raw_query);

        for (const auto& word : query_words.plus) {
            if (words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)) {
                matched_words.insert(word);
            }
        }

        for (const auto& word : query_words.minus) {
            if (words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)) {
                matched_words.clear();
            }
        }

        vector<string> matched_words_vector;

        for (const auto& word : matched_words) {
            matched_words_vector.push_back(word);
        }

        sort(matched_words_vector.begin(), matched_words_vector.end());

        if (matched_words_vector.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_words_vector.resize(MAX_RESULT_DOCUMENT_COUNT);
        }


        return { matched_words_vector, id_to_status_.at(document_id) };
    }

    int GetDocumentId(int index)const {
        if (index < 0 || index >= static_cast<int>(document_ids_.size())) {
            throw out_of_range("Invalid index");
        }
        return document_ids_.at(index);
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
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                ++result[word];
            }
        }
        return result;

    }

    Query ParseQuery(const string& text) const {
        set<string> plus_words;
        set<string> minus_words;
        if (auto words = SplitIntoWordsNoStop(text)) {
            for (const auto& [word, freq] : words.value()) {
                if (word.at(0) == '-') {
                    if (word.size() == 1 || word.at(1) == '-') {
                        throw invalid_argument("Query with invalid \"-\" or \"---\"");
                    }
                    minus_words.insert(word.substr(1)); //минус-слова берутся без знака
                }
                else {
                    plus_words.insert(word);
                }
            }
        }

        return { plus_words,minus_words };
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

void PrintDocument(const Document& document, ostringstream& out) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
}
template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    Iterator begin() const {
        return first_;
    }

    Iterator end() const {
        return last_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

template <typename Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({ begin, current_page_end });

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
};

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server): server(search_server) {
        // напишите реализацию
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        auto result = server.FindTopDocuments(raw_query, document_predicate);
        if (result.empty()) {
            ++empty_requests_count_;
        }
        requests_.push_back(raw_query,result);

        if (requests_.size() > 1440) {
            if (requests_.front().result.empty()) {
                --empty_requests_count_;
            }
            requests_.pop_front();
        }
        return result;
    }
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        auto result = server.FindTopDocuments(raw_query, status);
        if (result.empty()) {
            ++empty_requests_count_;
        }
        requests_.push_back({raw_query, result});

        if (requests_.size() > 1440) {
            if (requests_.front().result.empty()) {
                --empty_requests_count_;
            }
            requests_.pop_front();
        }
        return result;
    }
    vector<Document> AddFindRequest(const string& raw_query) {
        auto result = server.FindTopDocuments(raw_query);
        if (result.empty()) {
            ++empty_requests_count_;
        }
        requests_.push_back({raw_query, result});

        if (requests_.size() > 1440) {
            if (requests_.front().result.empty()) {
                --empty_requests_count_;
            }
            requests_.pop_front();
        }
        return result;
    }
    int GetNoResultRequests() const {
        return empty_requests_count_;
    }
private:
    const SearchServer& server;
    struct QueryResult {
        string query;
        vector<Document> result;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    size_t empty_requests_count_ = 0;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}