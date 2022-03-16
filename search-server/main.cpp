#include <windows.h>

#include "document.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "remove_duplicates.h"

#include "test_example_functions.hpp"

using namespace std;


int main() {
    //выключить в конcоли винды utf-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    TestRemoveDuplication();
    
    return 0;
}


