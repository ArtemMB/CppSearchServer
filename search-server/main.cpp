#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>

#include <windows.h>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;



string ReadLine() {
    string s;
    getline(cin, s);
    return s;
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

ostream& operator<<(ostream& os, const Document& doc)
{
    os<<"{";
    os<<" document_id = "<<doc.id;
    os<<", relevance = "<<doc.relevance;
    os<<", rating = "<<doc.rating;
    os<<" }";
    return os;
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}


enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        if(!all_of(stop_words_.cbegin(), stop_words_.cend(),
                  [](const string& word){
                        return IsValidWord(word);
                    }))
        {
            throw invalid_argument{"aaa Contains invalid characters in stop words"};
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, 
                                   DocumentStatus status,
                                   const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument{"document_id is invalid"};
        }
        
        vector<string> words = SplitIntoWordsNoStop(document);        

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, 
                           DocumentData{ComputeAverageRating(ratings), 
                                                     status});
        document_ids_.push_back(document_id);        
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, 
                                        DocumentPredicate document_predicate) const {        
        Query query = ParseQuery(raw_query);        
        
        auto matched_documents = FindAllDocuments(query, document_predicate);

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
        
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, 
                                                DocumentStatus status) const {
        return FindTopDocuments(
            raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (index == 0)
        {
            throw invalid_argument{"index is null"};
        }
        return document_ids_.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
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

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {        
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
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
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }       
};


void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

template <typename Iterator>
class IteratorRange
{
    public:
        IteratorRange(Iterator range_begin, Iterator range_end, 
                      size_t page_size):
            m_begin{range_begin},
            m_end{range_end},
            m_size{page_size}
        {}
        
        Iterator begin() const
        {
            return m_begin;
        }
        
        Iterator end() const
        {
            return m_end;
        }
        
        int size() const
        {
            return m_size;
        }
        
    protected:
        Iterator m_begin;
        Iterator m_end;
        size_t m_size;
};        
template <typename Iterator>
ostream& operator<<(ostream& os, const IteratorRange<Iterator>& page)
{
    Iterator b{page.begin()};
    while(b != page.end())
    {
        os<<*b;
        advance(b, 1);
    }
    
    return os;
} 

template <typename Iterator>
class Paginator {
        // тело класса
    public:
        Paginator(Iterator range_begin, Iterator range_end, size_t page_size)
        {     
            Iterator first;
            uint8_t state = 0;
            size_t counter = 0;
            while(range_begin != range_end)
            {
                switch (state)
                {
                    case 0:
                    {
                        first = range_begin;
                        state = 1;
                        counter = 1;
                    }
                    break;
                        
                    case 1:
                    {                   
                        counter++;
                        if(page_size >= counter )
                        {
                            break;
                        }
                        
                        counter = 1;                        
                        
                        IteratorRange<Iterator> item(first, range_begin, 
                                                page_size);
                        //cout<<item<<endl;
                        m_pages.push_back(item);
                        
                        first = range_begin;
                    }
                    break;
                }
                
                advance(range_begin, 1);                
            }
            
            if(counter)
            {
                IteratorRange<Iterator> item(first, range_end, 
                                        page_size);
                //cout<<"last "<<item<<endl;
                m_pages.push_back(item);
            }
        }
                
        auto begin() const 
        {
            return m_pages.cbegin();
        }
        
        auto end() const
        {
            return m_pages.cend();
        }
        
    protected:
        vector<IteratorRange<Iterator>> m_pages;
}; 

 

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator{begin(c), end(c), page_size};
}
        
int main() {
    //выключить в конcоли винды utf-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    SearchServer search_server("and with"s);
    
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
    
    return 0;
}


