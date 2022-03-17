#include <iostream>

#include "search_server.h"

#include "remove_duplicates.h"


using namespace std;


set<string> mapToSet(const map<string, double>& m)
{
    set<string> out;
    
    for(const auto& item: m)
    {
        out.insert(item.first);
    }
    
    return out;            
}

void RemoveDuplicates(SearchServer& search_server)
{
    set<int> duplicateForRemove;
    
    set<set<string>> verified;
    
    for(const int document_id : search_server)
    {
        set<string> wordsOfDocument = mapToSet(
                    search_server.GetWordFrequencies(document_id));
        
        if(0 == verified.count(wordsOfDocument))
        {
            verified.insert(wordsOfDocument);
            continue;
        }
        
        duplicateForRemove.insert(document_id);
    }
    
    for(const int id: duplicateForRemove)
    {
        search_server.RemoveDocument(id);
        cout<<"Found duplicate document id "<<id<<"\n";
    }
}

void AddDocument(SearchServer& search_server, 
                 int document_id, 
                 const std::string& document,
                 DocumentStatus status, 
                 const std::vector<int>& ratings)
{
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, 
                      const std::string& raw_query)
{
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, 
                    const std::string& query)
{
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;                    
        for(const int document_id: search_server)
        {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

void PrintDocument(const Document& document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, 
                              const std::vector<std::string>& words,
                              DocumentStatus status)
{
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
