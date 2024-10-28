#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cmath>

using namespace std;

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

// string ReadLine() {
//     string s;
//     getline(cin, s);
//     return s;
// }

// int ReadLineWithNumber() {
//     int result = 0;
//     cin >> result;
//     ReadLine();
//     return result;
// }

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
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

enum class DocumentStatus{
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

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document,DocumentStatus status, vector<int> rating) {
        const map <string, int> words_freq = SplitIntoWordsNoStop(document); // map{слово,количество повторов слова в документе}

        id_to_rating_[document_id] = ComputeAverageRating(rating);
        id_to_status_[document_id] =status;

        int total_words = accumulate(words_freq.begin(), words_freq.end(), 0, [](int sum, const pair<string, int>& next) {
            return sum + next.second;
        });

        for (const auto& word : words_freq) {
            words_to_docs_with_freq[word.first][document_id] = word.second * 1.0 / total_words;
        }

        ++document_count_;
    }

    template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter conditions ) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, conditions);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance ||
                       (abs(lhs.relevance-rhs.relevance)<EPSILON && lhs.rating>rhs.rating);
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus needed_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query,[needed_status](int document_id, DocumentStatus status, int rating){
            return status == needed_status;
        });
    }

    int GetDocumentCount() const{
        return document_count_;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const{
        set<string> mathced_words;
        const Query query_words = ParseQuery(raw_query);

        for(const auto& word : query_words.plus){
            if(words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)){
                mathced_words.insert(word);
            }
        }

        for(const auto& word : query_words.minus){
            if(words_to_docs_with_freq.count(word) && words_to_docs_with_freq.at(word).count(document_id)){
                mathced_words.clear();
            }
        }
        
        vector<string> matched_words_vector;

        for(const auto& i : mathced_words){
            matched_words_vector.push_back(i);
        }
        sort(matched_words_vector.begin(), matched_words_vector.end());

        if (matched_words_vector.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_words_vector.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return {matched_words_vector,id_to_status_.at(document_id)};
    }

private:

    map<string, map<int, double>> words_to_docs_with_freq; //map{слово,map{id,TF}}

    set<string> stop_words_;

    map<int, int> id_to_rating_;

    map<int, DocumentStatus> id_to_status_;

    int document_count_ = 0;

// Existence required
    double CalculateIDF(const string& word) const{  
        return log(document_count_ * 1.0 / words_to_docs_with_freq.at(word).size());
    }
    
    static int ComputeAverageRating (const vector<int>& rating){
        if(rating.empty()){
            return 0;
        }
        return accumulate(rating.begin(),rating.end(),0)/static_cast<int>(rating.size());
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    map<string, int> SplitIntoWordsNoStop(const string& text) const {
        map<string, int> words;

        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                ++words[word];
            }
        }

        return words;
    }

    Query ParseQuery(const string& text) const {
        set<string> plus_words;
        set<string> minus_words;

        for (const auto& [word, freq] : SplitIntoWordsNoStop(text)) {
            if (word.at(0) == '-') {
                minus_words.insert(word.substr(1, word.size() - 1)); //минус-слова берутся без знака
            }
            else {
                plus_words.insert(word);
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
            if(conditions(id,id_to_status_.at(id), id_to_rating_.at(id))){
                const int rating = id_to_rating_.at(id); 
                matched_documents.push_back({ id,rel,rating });
            }
        }

        return matched_documents;
    }
};


// ==================== для примера =========================

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
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }


// void PrintDocument(const Document& document) {
//     cout << "{ "s
//          << "document_id = "s << document.id << ", "s
//          << "relevance = "s << document.relevance << ", "s
//          << "rating = "s << document.rating
//          << " }"s << endl;
// }

// int main() {
//     SearchServer search_server;
//     search_server.SetStopWords("и в на"s);

//     search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
//     search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
//     search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

//     for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s)) {
//         PrintDocument(document);
//     }
// } 
// vector<int> ReadRating(){
//     int n = 0 ;
//     cin >> n ;
//     vector <int> rating(n);
//     for(auto& i : rating){
//         cin >> i;
//     }
//     ReadLine();
//     return rating;
// }


// SearchServer CreateSearchServer() {
//     SearchServer search_server;

//     search_server.SetStopWords(ReadLine());

//     const int document_count = ReadLineWithNumber();
//     for (int document_id = 0; document_id < document_count; ++document_id) {
//         string text = ReadLine();
//         DocumentStatus status = ReadStatus();
//         vector<int> rating = ReadRating();
//         search_server.AddDocument(document_id, text, status, rating);
//     }

//     return search_server;
// }


// int main() {
//     const SearchServer search_server = CreateSearchServer();

//     const string query = ReadLine();

//     for (const auto& document : search_server.FindTopDocuments(query)) {
//         cout << "{ document_id = "s << document.id << ", "s
//             << "relevance = "s << document.relevance << ", "s
//             << "rating = "s << document.rating << " }"s << endl;
//     }
// }

