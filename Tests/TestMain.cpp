#include <iostream>

void RunCatInspectorTests();
void RunCatSearchTests();

int main()
{
    std::cout << "===== CatManager Tests =====\n\n";

    RunCatInspectorTests();
    RunCatSearchTests();

    std::cout << "\n===== ALL TESTS PASSED =====\n";

    return 0;
}