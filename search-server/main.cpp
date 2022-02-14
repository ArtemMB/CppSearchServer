#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
//#include <cassert>

#include <windows.h>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

template <typename KeyType, typename ValveType> 
ostream& operator<<(ostream& out, const pair<KeyType, ValveType>& container) 
{
    out<<container.first<<": "<<container.second;     
    return out;
}

template <typename T> void Print(ostream& out, const T& container)
{
    bool is_first{true};
    string prefix{""};
    
    for (const auto& item: container) 
    {
        out << prefix ;
        out << item ;
        
        if(is_first)
        {
            is_first = false;
            prefix = ", ";
        }
    }
}

template <typename T> 
ostream& operator<<(ostream& out, const vector<T>& container) 
{
    out<<"[";
    Print(out, container);
    out<<"]";
    return out;
} 

template <typename T> 
ostream& operator<<(ostream& out, const set<T>& container) 
{
    out<<"{";
    Print(out, container);
    out<<"}";
    return out;
} 

template <typename Key, typename Valve> 
ostream& operator<<(ostream& out, const map<Key, Valve>& container) 
{
    out<<"{";
    Print(out, container);
    out<<"}";
    return out;
}

void AssertImpl(const bool expr_value, const string& expr_str, 
            const string& file,
            const string& func, unsigned line, 
            const string& hint) {
    if (expr_value) {
        return;
    }
    
    cerr  << file << "("s << line << "): "s << func << ": "s;
    cerr  << "ASSERT("s << expr_str << ") failed."s;
    
    if (!hint.empty()) {
        cerr  << " Hint: "s << hint;
    }
    
    cerr  << endl;
    abort();
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str,
                     const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr  << boolalpha;
        cerr  << file << "("s << line << "): "s << func << ": "s;
        cerr  << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr  << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr  << " Hint: "s << hint;
        }
        cerr  << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Function>
void RunTestImpl(Function fun, const string& fun_name) {    
    fun();
    cerr << fun_name<< " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
/*
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}*/

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
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
    int id{0};
    double relevance{0.0};
    int rating{0};
    
    Document() = default;
    Document(const int _id, const double _relevance, const int _rating):
        id{_id},
        relevance{_relevance},
        rating{_rating}
    {        
    }
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
        inline static constexpr int INVALID_DOCUMENT_ID = -1;
        
    SearchServer() = default;
    explicit SearchServer(const string& stop_words)
    {
        SetStopWords(stop_words);
    }
    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {        
        for (const string& word : stop_words) {
            stop_words_.insert(word);
        }
    }
        
        
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }    
    
    [[nodiscard]] bool AddDocument(int document_id, 
                     const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count{ 1.0 / words.size()};
        
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        
        documents_.emplace(document_id, 
            DocumentData{
                ComputeAverageRating(ratings), 
                status
            });
    }
    
    [[nodiscard]] bool FindTopDocuments(const string& raw_query, 
                                      vector<Document>& result) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL, result);
    }
    
    template<typename DocumentPredicate>
    [[nodiscard]] bool FindTopDocuments(
            const string& raw_query, DocumentPredicate document_predicate,
            vector<Document>& result) const {            
        const Query query{ParseQuery(raw_query)};
        auto matched_documents{FindAllDocuments(query, document_predicate)};
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
             });
        
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        
        result = matched_documents;
        
        return true;
    }
    
    [[nodiscard]] bool FindTopDocuments(
            const string& raw_query, DocumentStatus status,
            vector<Document>& result) const {            
        return FindTopDocuments(raw_query, 
                                [status](int , DocumentStatus document_status, int )
        { return document_status == status; }, result);
    }
    
    int GetDocumentCount() const {
        return documents_.size();
    }
    int GetDocumentId(int index) const {
        
        }
    
    [[nodiscard]] bool MatchDocument(const string& raw_query, int document_id, 
            tuple<vector<string>, DocumentStatus>& result) const {
        const Query query{ParseQuery(raw_query)};
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
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        result = {matched_words, documents_.at(document_id).status};
        return true;
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
        return stop_words_.count(word) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;        
        const vector<string> raw_words{SplitIntoWords(text)};
        copy_if(raw_words.cbegin(), raw_words.cend(), 
                std::back_inserter(words),
                [this](const string& word){            
                        return !IsStopWord(word);
                });
        return words;
    }
    
    static bool IsValidWord(const string& word) {
            // A valid word must not contain special characters
            return none_of(word.begin(), word.end(), [](char c) {
                return c >= '\0' && c < ' ';
            });
        }
    
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        
        int rating_sum{ accumulate(ratings.begin(), ratings.end(), 0)};
        
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(string text) const {
        bool is_minus{false};
        // Word shouldn't be empty
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
            const QueryWord query_word{ParseQueryWord(word)};
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
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate mapper) const {
        map<int, double> document_to_relevance; 
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            
            const double inverse_document_freq{ComputeWordInverseDocumentFreq(word)};
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (mapper(document_id,
                           documents_.at(document_id).status,
                           documents_.at(document_id).rating)) {  
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;                
                }
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
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

// -------- Начало модульных тестов поисковой системы ----------
/*
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content{"cat in the city"s};
    const vector<int> ratings{ 1, 2, 3};
    
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs{ server.FindTopDocuments("in"s)};
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0{found_docs[0]};
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), 
                    "Stop words must be excluded from documents"s);
    }
}


//Тест проверяет, что поисковая система не добавляет документы с 
//минус-словами в результат поиска
void TestExcludeDocumentsWithMinusWordsFromResult(){
    const int full_string_doc_id = 42;
    const int bad_doc_id = 13;
    const int none_doc_id = 666;
    const string full_string_content{"cat in the city"s};
    const string bad_content{"cat city"s};
    const string none_content{"london is city"s};
    const vector<int> ratings{ 1, 2, 3};
    const set<int> ids{full_string_doc_id, bad_doc_id};
    
    
    {
        SearchServer server;
        server.AddDocument(full_string_doc_id, full_string_content, 
                           DocumentStatus::ACTUAL, ratings);
        server.AddDocument(bad_doc_id, bad_content, 
                           DocumentStatus::ACTUAL, ratings);
        server.AddDocument(none_doc_id, none_content, 
                           DocumentStatus::ACTUAL, ratings);
        
        // Сначала убеждаемся, что поиск находит все документы с + словом
        const auto plus_word_found_docs{ server.FindTopDocuments("cat"s)};
        ASSERT_EQUAL(plus_word_found_docs.size(), 2u); 
        ASSERT_EQUAL(ids.count(plus_word_found_docs[1].id), 1);
        ASSERT_EQUAL(ids.count(plus_word_found_docs[0].id), 1);
        
        // Затем убеждаемся, что поиск исключает документы с минус словом    
        const auto minus_word_found_docs{ server.FindTopDocuments("cat -in"s)};
        ASSERT_EQUAL(minus_word_found_docs.size(), 1u);
        ASSERT_EQUAL(minus_word_found_docs[0].id, bad_doc_id);
    }   
}

//Тест проверяет матчинг документов
void TestMatchDocument(){
    const int doc_id = 42;    
    const string content{"cat in the city"s};    
    const vector<int> ratings{ 1, 2, 3}; 
    
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        
        // Сначала убеждаемся, что поиск находит все документы с + словом
        const auto [plus_word_found_words, plus_word_status] 
                = server.MatchDocument("white cat big city"s, doc_id);  
        const vector<string> expected_words{"cat"s, "city"s};
        ASSERT_EQUAL(plus_word_found_words, expected_words);   
        
                
        // Затем убеждаемся, что поиск исключает документы с минус словом  
        const auto [minus_word_found_words, minus_word_status]
                = server.MatchDocument("white cat -city"s, doc_id);        
        ASSERT(minus_word_found_words.empty());
    }    
}

//Тест проверяет cортировку найденных документов по релевантности
void TestSortByRelevance(){    
    const vector<int> ratings{ 1, 2, 3};  
    
    SearchServer server;    
    server.AddDocument(0, "белый кот модный ошейник"s,          
                       DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       
                       DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, 
                       DocumentStatus::ACTUAL, ratings);
    server.AddDocument(3, "ухоженный скворец евгений"s,         
                       DocumentStatus::BANNED, ratings);    
    
    const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s)};
    ASSERT_EQUAL(found_docs.size(),  3u); 
    
    ASSERT_EQUAL(found_docs[0].id,  1);
    ASSERT_EQUAL(found_docs[1].id,  0);    
    ASSERT_EQUAL(found_docs[2].id,  2); 
}

//Тест проверяет вычисление рейтинга документов
void TestCalculationRating(){    
    SearchServer server;    
    server.AddDocument(0, "белый кот и модный ошейник"s,        
                       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       
                       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, 
                       DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         
                       DocumentStatus::BANNED, {9});
    
    const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s)};
    ASSERT_EQUAL(found_docs.size(),  3u);  
    
    ASSERT_EQUAL(found_docs[0].id,  1);
    ASSERT_EQUAL(found_docs[0].rating,  (7 + 2 + 7)/3);
    
    ASSERT_EQUAL(found_docs[1].id,  2);
    ASSERT_EQUAL(found_docs[1].rating,  (5 - 12 + 2 + 1)/4);
    
    ASSERT_EQUAL(found_docs[2].id, 0);
    ASSERT_EQUAL(found_docs[2].rating, (8 - 3)/2);    
}

//Тест проверяет использование предиката, задаваемого пользователем
void TestSearchDocumentsByUserPredicate(){    
    SearchServer server;    
    server.AddDocument(0, "белый кот и модный ошейник"s, 
                       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, 
                       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, 
                       DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s, 
                       DocumentStatus::BANNED, {9});
    
    //
    {
        const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s,                                                      
                            [](int document_id, DocumentStatus, int)
            { return document_id == 1; })};
        
        ASSERT_EQUAL(found_docs.size(), 1u);  
        
        ASSERT_EQUAL(found_docs[0].id, 1);
        ASSERT_EQUAL(found_docs[0].rating, 5);
    }
    
    //
    {
        const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s,                                                      
                            [](int , DocumentStatus document_status, int)
            { return document_status == DocumentStatus::BANNED; })};
        
        ASSERT_EQUAL(found_docs.size(), 1u);  
        
        ASSERT_EQUAL(found_docs[0].id, 3);
        ASSERT_EQUAL(found_docs[0].rating, 9);
    }    
}

//Тест проверяет поиск документов, имеющих заданный статус.
void TestSearchDocumentsByStatus(){
    SearchServer server;    
    server.AddDocument(0, "белый кот и модный ошейник"s,        
                       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       
                       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, 
                       DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         
                       DocumentStatus::BANNED, {9});
    
    {
        const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s,
                                                      DocumentStatus::ACTUAL)};
        ASSERT_EQUAL(found_docs.size(), 3u);  
        
        ASSERT_EQUAL(found_docs[0].id, 1);        
        ASSERT_EQUAL(found_docs[1].id, 2);  
        ASSERT_EQUAL(found_docs[2].id, 0);
         
    }
    
    {
        const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s,
                                                      DocumentStatus::BANNED)};
        ASSERT_EQUAL(found_docs.size(), 1u);  
        
        ASSERT_EQUAL(found_docs[0].id, 3);        
    }    
    
    {
        const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s,
                                                      DocumentStatus::REMOVED)};
        ASSERT(found_docs.empty());          
    }    
}

//Тест проверяет вычисление релевантности
void TestCalculationRelevance(){    
    SearchServer server;    
    server.AddDocument(0, "белый кот и модный ошейник"s, 
                       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, 
                       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, 
                       DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         
                       DocumentStatus::BANNED, {9});
    
    vector<double> relevancesOfDocuments;
    relevancesOfDocuments.push_back(log(server.GetDocumentCount()*1.0/2.0)*(0.25) +
                                    log(server.GetDocumentCount()*1.0/1.0)*(0.5));
    relevancesOfDocuments.push_back(log(server.GetDocumentCount()*1.0/2.0)*(0.25));
    relevancesOfDocuments.push_back(log(server.GetDocumentCount()*1.0/2.0)*(0.2));
    
    const auto found_docs{server.FindTopDocuments("пушистый ухоженный кот"s)};
    ASSERT_EQUAL(found_docs.size(), 3u);  
    
    ASSERT_EQUAL(found_docs[0].id, 1);
    ASSERT(abs(found_docs[0].relevance - relevancesOfDocuments[0]) < EPSILON);
    
    ASSERT_EQUAL(found_docs[1].id, 2);
    ASSERT(abs(found_docs[1].relevance - relevancesOfDocuments[1]) < EPSILON);
    
    ASSERT_EQUAL(found_docs[2].id, 0);
    ASSERT(abs(found_docs[2].relevance - relevancesOfDocuments[2]) < EPSILON);        
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromResult);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestCalculationRating);
    RUN_TEST(TestSearchDocumentsByUserPredicate);
    RUN_TEST(TestSearchDocumentsByStatus);
    RUN_TEST(TestCalculationRelevance);
}

*/

// --------- Окончание модульных тестов поисковой системы -----------
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    //выключить в конcоли винды utf-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    //TestSearchServer();
    SearchServer search_server("и в на"s);
    
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});

    if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
        cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    }

    if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
        cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    }

    if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2})) {
        cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    }

    vector<Document> documents;
    if (search_server.FindTopDocuments("--пушистый"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    } else {
        cout << "Ошибка в поисковом запросе"s << endl;
    }
        
    return 0;
}
