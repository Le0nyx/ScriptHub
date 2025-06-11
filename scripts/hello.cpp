#include <iostream>
#include <string>

int main() {
    std::cout << "Hello World!" << std::endl;
    std::cout << "This is a test program." << std::endl;
    
    std::cout << "Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
    
    std::cout << "You pressed Enter! Program will now exit." << std::endl;
    
    return 0;
}
