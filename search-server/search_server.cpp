#include <algorithm>
#include <cmath>

#include "search_server.h"

using namespace std;
    

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, 
                               const string& document, 
                               DocumentStatus status, 
                               const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument{"document_id is invalid"};
    }
    
    vector<string> words = SplitIntoWordsNoStop(document);        
    
    const double inv_word_count = 1.0 / words.size();
    map<string, double> wordFrequencies;
    
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        wordFrequencies[word] = word_to_document_freqs_[word][document_id];
    }
    documents_.emplace(document_id, 
                       DocumentData{ComputeAverageRating(ratings), 
                                    status});
    
    document_to_word_freqs_.emplace(document_id, wordFrequencies);
    
    document_ids_.insert(document_id);        
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, 
                                                DocumentStatus status) const {
    return FindTopDocuments(
                raw_query,
                [status](int , 
                DocumentStatus document_status, int ) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
        const string& raw_query, int document_id) const { 
    Query query = ParseQuery(raw_query);        
    
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
    tuple<vector<string>, DocumentStatus> out{matched_words, 
                documents_.at(document_id).status};
    return out;        
}

void SearchServer::RemoveDocument(int document_id)
{    
    for(auto&[word, freq]: document_to_word_freqs_.at(document_id))
    {
        word_to_document_freqs_[word].erase(document_id);  
        
        if(word_to_document_freqs_[word].empty())
        {
            word_to_document_freqs_.erase(word);//удаляем, т.к. слово больше не нужно
        }
    }
    
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const
{    
    static map<string, double> empty_word_frequencies;
    
    auto it = documents_.find(document_id);
    if(it != documents_.end())
    {
        return document_to_word_freqs_.at(document_id);
    }
    
    return empty_word_frequencies;
}

set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.cbegin();
}

set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.cend();
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument{"Contains invalid characters"};
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }        
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    if (text.empty()) {
        throw invalid_argument{text + " is empty"};
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw invalid_argument{text + " is invalid"};
    }
    
    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {        
    Query result;
    for (const string& word : SplitIntoWords(text)) {
        QueryWord query_word = ParseQueryWord(word);
        
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
