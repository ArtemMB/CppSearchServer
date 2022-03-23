#include <windows.h>


#include "test_example_functions.h"

using namespace std;


int main() {
    //выключить в конcоли винды utf-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    TestSingleLinkedList();
    
    return 0;
}


