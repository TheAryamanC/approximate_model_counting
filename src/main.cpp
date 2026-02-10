// Main entry point for the program

#include <iostream>
#include <string>
#include <memory>
#include "cnf/cnf_parser.h"
#include "cnf/cnf_structure.h"
#include "xor/xor_hash_generator.h"
#include "solver/partial_assignment.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "GPU-Accelerated Approximate #SAT Solver" << endl << endl;

    string filename;
    cout << "Please enter the CNF file path: ";
    cin >> filename;
    
    try {
        // parse CNF file
        cout << "Parsing CNF file: " << filename << endl;
        auto formula = CNFParser::parseFile(filename);
        cout << "Successfully parsed CNF file!" << endl;
        
        // generate XOR constraints
        cout << "Generating XOR constraints" << endl;
        int numXORs = 3;
        double density = 0.1;
        int numVariables = formula->getNumVariables();
        auto xors = XORHashGenerator::generateXORFamily(numVariables, numXORs, density);
        cout << "Generated " << xors.size() << " XOR constraints" << endl;
        
        // solve XOR system using Gaussian elimination
        cout << "Solving XOR system using Gaussian elimination" << endl;
        auto solution = PartialAssignment::solveXORSystem(xors, numVariables);
        if (solution.satisfiable) {
            cout << "XOR system is satisfiable" << endl;
            cout << "  Assigned: " << solution.assignment.size() << " variables" << endl;
            cout << "  Free: " << solution.freeVariables.size() << " variables" << endl;
        } else {
            cout << "XOR system is unsatisfiable - no solutions exist" << endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        cerr << "Something went wrong: " << e.what() << endl;
        return -1;
    }
}
