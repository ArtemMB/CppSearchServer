#include <iostream>
#include <cassert>
#include <random>
#include <execution>

#include "search_server.h"
#include "test_example_functions.h"


#include "processqueries.h"
#include "remove_duplicates.h"

#include "logduration.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, 
                                  int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, 
                     const vector<string>& dictionary, 
                     int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}



void TestParalelQuery()
{
    SearchServer search_server("and with"s);
    
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (
        const auto& documents : ProcessQueries(search_server, queries)
    ) {
        cout << documents.size() 
             << " documents for query ["s << queries[id++] 
             << "]"s << endl;
    }
}

void TestProcessQueriesJoined()
{
    SearchServer search_server("and with"s);
    
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
    }
}

void TestRemoveDocument()
{
    SearchServer search_server("and with"s);
    
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(execution::par, 2);
    report();
    
    //5 documents total, 4 documents for query [curly and funny]
    //4 documents total, 3 documents for query [curly and funny]
    //3 documents total, 2 documents for query [curly and funny]
    //2 documents total, 1 documents for query [curly and funny] 
}

void TestMatchDocument()
{
    SearchServer search_server("and with"s);
    
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(
                query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(
                execution::seq, 
                query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        /*const auto [words, status] = search_server.MatchDocument(
                execution::par, 
                query, 3);
        cout << words.size() << " words for document 3"s << endl;*/        
        // 0 words for document 3
        
       /* const auto [words, status] = search_server.MatchDocument(
                execution::seq, 
                query, 2);
        cout << words.size() << " words for document 2"s << endl;*/
    }
        }
    
template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, 
          const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}       
        
void TestFindTopDocuments()
{
    #define TEST(policy) Test(#policy, search_server, queries, execution::policy)  
    /*
    {
        
//        ACTUAL by default:
//        { document_id = 2, relevance = 0.866434, rating = 1 }
//        { document_id = 4, relevance = 0.231049, rating = 1 }
//        { document_id = 1, relevance = 0.173287, rating = 1 }
//        { document_id = 3, relevance = 0.173287, rating = 1 }
//        BANNED:
//        Even ids:
//        { document_id = 2, relevance = 0.866434, rating = 1 }
//        { document_id = 4, relevance = 0.231049, rating = 1 } 
        
        SearchServer search_server("and with"s);
        
        int id = 0;
        for (
            const string& text : {
                "white cat and yellow hat"s,
                "curly cat curly tail"s,
                "nasty dog with big eyes"s,
                "nasty pigeon john"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }
    
    
        cout << "ACTUAL by default:"s << endl;
        // последовательная версия
        for (const Document& document : 
             //search_server.FindTopDocuments("curly nasty cat"s)
             search_server.FindTopDocuments(execution::par, "curly nasty cat"s)
             ) 
        {
            PrintDocument(document);
        }
        cout << "BANNED:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments(
                 execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
            PrintDocument(document);
        }
    
        cout << "Even ids:"s << endl;
        // параллельная версия
        for (const Document& document : search_server.FindTopDocuments(
                 execution::par, "curly nasty cat"s, 
                 [](int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0; })) {
            PrintDocument(document);
        }
    }//*/
    
    {
        mt19937 generator;
        
        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
    
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
    
        const auto queries = GenerateQueries(generator, dictionary, 100, 70);
        /*{
            for (const string_view query : queries) {
                const auto _seq = search_server.FindTopDocuments(execution::seq, query);
                const auto _par = search_server.FindTopDocuments(execution::par, query);
                if(_seq.size() != _par.size())
                {
                    cout<<query<<'\n';
                    cout<<_seq.size()<<"\n";
                    cout<<_par.size()<<"\n";
                    break;
                }                
            }
            cout<<"All correct\n";
        }*/
        TEST(seq);
        TEST(par);
    }
}
        
