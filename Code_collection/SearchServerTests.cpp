#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "SearchServer.cpp"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */


   template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)


// -------- Начало модульных тестов поисковой системы ----------

void TestAddDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends"s;
    const vector<int> ratings2 = {3, 2, 3};

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id);

    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    const auto found_docs2 = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs2.size(), 2u);
    const Document& doc2 = found_docs2[1];
    ASSERT_EQUAL(doc2.id, doc_id2);
    
}
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestExcludeDocumentsWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends"s;
    const vector<int> ratings2 = {3, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat"s).size() == 1);
        ASSERT_HINT(server.FindTopDocuments("cat -city"s).empty(),
                    "Minus words should exclude document from search"s);

        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        ASSERT(server.FindTopDocuments("cat"s).size() == 2);
        ASSERT_HINT(server.FindTopDocuments("cat -city"s).size() == 1,
                    "Minus words should exclude document from search"s);
    }
}

void TestMatchDocument() {
    const int doc_id = 42;
    const string content = "cat with big ears in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends with kind eyes. Cat is grey"s;
    const vector<int> ratings2 = {3, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(get<0>(server.MatchDocument("cat dog friends"s, 42)).size(),1u);
        ASSERT_HINT(get<0>(server.MatchDocument("cat dog friends -city"s, 42)).empty(),
                    "Minus words should exclude document from match"s);

        server.SetStopWords("in, the, with, and");
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        ASSERT_EQUAL(get<0>(server.MatchDocument("cat and dog with friends"s, 43)).size(),4u);
        ASSERT_HINT(!get<0>(server.MatchDocument("cat dog friends -city"s, 43)).empty(),
                    "Minus words should exclude document from match"s);
        ASSERT_HINT(get<1>(server.MatchDocument("cat dog friends"s, 43)) == DocumentStatus::BANNED,
                    "MatchDocument should return status"s);          
    }
}

void TestRelevanceSort() {
    const int doc_id = 42;
    const string content = "cat in the big and cold city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends with kind eyes. Cat is grey"s;
    const vector<int> ratings2 = {3, 2, 3};
    const int doc_id3 = 44;
    const string content3 = "grey dog without ear"s;
    const vector<int> ratings3 = {4, 2, 3};
    const int doc_id4 = 45;
    const string content4 = "big fluffy cat"s;
    const vector<int> ratings4 = {5, 2, 3};
    const int doc_id5 = 46;
    const string content5 = "deer in small willage"s;
    const vector<int> ratings5 = {1, 2, 3};
    const int doc_id6 = 47;
    const string content6 = "cat with nose and ears and cooking skills"s;
    const vector<int> ratings6 = {-3, 2, 3};
    const int doc_id7 = 48;
    const string content7 = "grey dog without ear fighting with big bear"s;
    const vector<int> ratings7 = {4, 0, -35};
    const int doc_id8 = 49;
    const string content8 = "big toy cat without eye"s;
    const vector<int> ratings8 = {5, 2, 3};
    {
        SearchServer server;
        server.SetStopWords("in the and is with"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        server.AddDocument(doc_id5, content5, DocumentStatus::ACTUAL, ratings5);
        server.AddDocument(doc_id6, content6, DocumentStatus::ACTUAL, ratings6);
        server.AddDocument(doc_id7, content7, DocumentStatus::ACTUAL, ratings7);
        server.AddDocument(doc_id8, content8, DocumentStatus::ACTUAL, ratings8);

        auto found_docs = server.FindTopDocuments("grey cat -city");
        ASSERT_HINT(abs(found_docs[0].relevance - 0.245207) < 1e-5, "Wrong relevance calculations");
        ASSERT(found_docs[0].relevance >= found_docs[1].relevance && found_docs[1].relevance >= found_docs[2].relevance
        && found_docs[2].relevance >= found_docs[3].relevance && found_docs[3].relevance >= found_docs[4].relevance);
    }
}

void TestRatingCalculations() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends"s;
    const vector<int> ratings2 = {3, 15, 3, 0, -3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        ASSERT_EQUAL(server.FindTopDocuments("cat and dog"s)[0].rating , (3+15+3+0-3)/5);
        ASSERT_EQUAL(server.FindTopDocuments("cat and dog"s)[1].rating , (1+2+3)/3);
    }
}

void TestPredicateFilters() {
    const int doc_id = 42;
    const string content = "cat in the big and cold city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 43;
    const string content2 = "cat and dog friends with kind eyes. Cat is grey"s;
    const vector<int> ratings2 = {-3, 2, 3};
    const int doc_id3 = 44;
    const string content3 = "grey dog without ear"s;
    const vector<int> ratings3 = {4, 0, -35};
    const int doc_id4 = 45;
    const string content4 = "big fluffy cat"s;
    const vector<int> ratings4 = {5, 2, 3};
    {
        SearchServer server;
        server.SetStopWords("in the and is with"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::IRRELEVANT, ratings4);
        ASSERT_EQUAL(server.FindTopDocuments("cat in city"s).size(),2u);
        ASSERT_EQUAL(server.FindTopDocuments("cat in city"s, DocumentStatus::BANNED).size(),0u);
        ASSERT_EQUAL(server.FindTopDocuments("cat in city"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }).size(),1u);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestRatingCalculations);
    RUN_TEST(TestPredicateFilters);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}