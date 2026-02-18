// Main entry point for the program

#include <iostream>
#include <string>
#include <memory>

#include "cnf/cnf_parser.h"
#include "cnf/cnf_structure.h"
#include "xor/xor_hash_generator.h"
#include "solver/partial_assignment.h"
#include "solver/cnf_simplifier.h"
#include "solver/approximate_counter.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "GPU-Accelerated Approximate #SAT Solver" << endl << endl;

    string filename;
    cout << "Please enter the CNF file path: ";
    cin >> filename;
    
    try {
        // Phase 1: Parse CNF file
        cout << "=== Phase 1: Parsing CNF ===" << endl;
        auto formula = CNFParser::parseFile(filename);
        cout << "Successfully parsed CNF file!" << endl;
        cout << "  Variables: " << formula->getNumVariables() << endl;
        cout << "  Clauses: " << formula->getNumClauses() << endl << endl;
        
        // Phase 2: Generate XOR constraints and solve
        cout << "=== Phase 2: XOR Hash Generation ===" << endl;
        int numXORs = 3;
        double density = 0.1;
        int numVariables = formula->getNumVariables();
        
        auto xors = XORHashGenerator::generateXORFamily(numVariables, numXORs, density);
        cout << "Generated " << xors.size() << " XOR constraints with density " << density << endl;
        
        cout << "Solving XOR system using Gaussian elimination..." << endl;
        auto solution = PartialAssignment::solveXORSystem(xors, numVariables);
        
        if (solution.satisfiable) {
            cout << "  XOR system is satisfiable" << endl;
            cout << "  Assigned: " << solution.assignment.size() << " variables" << endl;
            cout << "  Free: " << solution.freeVariables.size() << " variables" << endl << endl;
        } else {
            cout << "  XOR system is unsatisfiable - no solutions exist" << endl;
            return 0;
        }
        
        // Phase 3: Apply XOR solution to simplify CNF
        cout << "=== Phase 3: CNF Simplification ===" << endl;
        cout << "Applying partial assignment to CNF..." << endl;
        auto simplificationResult = CNFSimplifier::applyXORSolution(*formula, solution);
        cout << endl;
        
        if (simplificationResult.isUnsatisfiable) {
            cout << "  Formula is UNSATISFIABLE with this XOR configuration" << endl;
        } else if (simplificationResult.isTriviallyTrue) {
            cout << "  Formula is SATISFIABLE (trivially true after simplification)" << endl;
        } else {
            cout << "  Simplified CNF:" << endl;
            cout << "    Original: " << formula->getNumClauses() << " clauses" << endl;
            cout << "    Simplified: " << simplificationResult.simplified.getNumClauses() << " clauses" << endl;
            double reductionPercent = 100.0 * simplificationResult.clausesRemoved / formula->getNumClauses();
            cout << "    Reduction: " << reductionPercent << "%" << endl;
        }
        
        // Phase 4: Approximate Model Counting with Multiple Trials
        cout << "=== Phase 4: Approximate Model Counting ===" << endl;
        
        int numTrials = 10;
        int trialsXORs = 3;
        double trialsDensity = 0.1;
        
        auto countResult = ApproximateCounter::approximateCount(*formula, numTrials, trialsXORs, trialsDensity);

        cout << "Approximate Count Results:" << endl;
        cout << "  Estimated Solutions: " << countResult.estimatedCount << endl;
        cout << "  Average Solutions (successful trials): " << countResult.averageCount << endl;
        cout << "  Successful Trials: " << countResult.successfulTrials << "/" << countResult.totalTrials << endl;
        cout << "  Trial Counts: ";
        for (size_t i = 0; i < countResult.trialCounts.size(); i++) {
            cout << countResult.trialCounts[i];
            if (i < countResult.trialCounts.size() - 1) {
                cout << ", ";
            }
        }
        
        return 0;
    } catch (const std::exception& e) {
        cerr << "Something went wrong: " << e.what() << endl;
        return -1;
    }
}
