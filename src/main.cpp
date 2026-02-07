// Main entry point for the program

#include <iostream>
#include <string>
#include <memory>
#include "cnf/cnf_parser.h"
#include "cnf/cnf_structure.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "GPU-Accelerated Approximate #SAT Solver" << endl << endl;

    string filename;
    cout << "Please enter the CNF file path: ";
    cin >> filename;
    
    try {
        // Parse the CNF file
        cout << "Parsing CNF file: " << filename << endl;
        auto formula = CNFParser::parseFile(filename);
        cout << "Successfully parsed CNF file!" << endl << endl;
        
        return 0;
    } catch (const std::exception& e) {
        cerr << "Something went wrong: " << e.what() << endl;
        return -1;
    }
}
