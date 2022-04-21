#include <iostream>
#include <algorithm>
#include <iterator>

#include "search_server.h"

#include "remove_duplicates.h"


using namespace std;

template <typename Key, typename Value>
set<Key> ExtractKeysFromMap(const map<Key, Value>& container)
{
    set<Key> out;
        
    transform(container.begin(), container.end(), 
              insert_iterator(out, out.begin()),
              [](const auto& key_value){
        return key_value.first;
    });
    
    return out;            
}

void RemoveDuplicates(SearchServer& search_server)
{
    set<int> duplicate_for_remove;
    
    set<set<string_view>> verified;
    
    for(const int document_id : search_server)
    {
        set<string_view> wordsOfDocument = ExtractKeysFromMap(
                                          search_server.GetWordFrequencies(
                                              document_id));
        
        if(0 == verified.count(wordsOfDocument))
        {
            verified.insert(wordsOfDocument);
            continue;
        }
        
        duplicate_for_remove.insert(document_id);
    }
    
    for(const int id: duplicate_for_remove)
    {
        search_server.RemoveDocument(id);
        cout<<"Found duplicate document id "<<id<<"\n";
    }
}










