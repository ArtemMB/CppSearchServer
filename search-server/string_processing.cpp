#include "string_processing.h"

//#include "logduration.h"

using namespace std;

vector<string> SplitIntoWords(const string_view text) {    
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

vector<string_view> SplitIntoWordsView(const string_view str)
{
    //LOG_DURATION(string{"SplitIntoWordsView"});
    
    vector<string_view> result;
   
    int64_t pos{0};
    
    const int64_t pos_end{static_cast<int64_t>(str.npos)};
    
    while (true) {
        
        int64_t space{static_cast<int64_t>(str.find(' ', pos))};
        
        result.emplace_back((space == pos_end) ? 
                             str.substr(pos) : 
                             str.substr(pos, space - pos));
        
        if (space == pos_end) 
        {
            break;
        } 
        else 
        {
            pos = space + 1;
        }
    }

    return result;
}
