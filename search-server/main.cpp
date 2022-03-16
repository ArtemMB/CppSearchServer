#include <windows.h>

#include "search_server.h"
#include "remove_duplicates.h"
#include "test_example_functions.h"

using namespace std;


int main() {
    //выключить в конcоли винды utf-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    TestRemoveDuplication();
    
    return 0;
}


