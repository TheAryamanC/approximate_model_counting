// Header file for approximate counting

#ifndef APPROXIMATE_COUNTER_H
#define APPROXIMATE_COUNTER_H

#include <vector>
#include <memory>
#include "cnf/cnf_structure.h"
#include "xor/xor_hash_generator.h"

// Single counting trial result
struct TrialResult {
    bool satisfiable;
    uint64_t solutionCount;
    int numXORs;
    int freeVariables;
    int assignedVariables;
    
    TrialResult() : 
        satisfiable(false),
        solutionCount(0),
        numXORs(0),
        freeVariables(0),
        assignedVariables(0) {}
};

// Final approximation result - using multiple aggregated trials
struct ApproximationResult {
    uint64_t estimatedCount;
    double averageCount;
    int successfulTrials;
    int totalTrials;
    std::vector<uint64_t> trialCounts;
    
    ApproximationResult() : 
        estimatedCount(0), 
        averageCount(0.0), 
        successfulTrials(0), 
        totalTrials(0) {}
};

class ApproximateCounter {
public:
    // Run approximate counting with multiple trials
    static ApproximationResult approximateCount(const CNFFormula& formula, int numTrials = 10, int numXORs = 3, double density = 0.1);
        
    // Run trial with adaptive XOR count
    static TrialResult singleTrial(const CNFFormula& formula, double density, int threshold = 50);
    
    // Aggregate results from multiple trials
    static ApproximationResult aggregateResults(const std::vector<TrialResult>& trials);
    
private:
    struct CDCLAssignment {
        int value;           // -1 = unassigned, 0 = false, 1 = true
        int decisionLevel;   // Which level was this assigned at
        int antecedent;
    };
    
    struct WatchedLiterals {
        std::vector<std::vector<int>> watches;  // list of clause indices
        
        void init(int numVars, int numClauses) {
            watches.resize(numVars * 2);
        }
        
        int litToIndex(Literal lit, int numVars) const {
            int var = abs(lit) - 1;
            return (lit > 0) ? (2 * var) : (2 * var + 1);
        }
    };
    
    struct VSIDSScores {
        std::vector<double> scores;
        double decay = 0.95;
        double increment = 1.0;
        
        void init(int numVars) {
            scores.resize(numVars, 0.0);
        }
        
        void bump(int var) {
            scores[var] += increment;
        }
        
        void decayAll() {
            increment /= decay;
        }
        
        int selectUnassigned(const std::vector<CDCLAssignment>& assignment) {
            int bestVar = -1;
            double bestScore = -1.0;
            for (size_t i = 0; i < assignment.size(); i++) {
                if (assignment[i].value == -1 && scores[i] > bestScore) {
                    bestScore = scores[i];
                    bestVar = i;
                }
            }
            return bestVar;
        }
    };
    
    // Count solutions in simplified CNF up to maxCount (bounded enumeration)
    static uint64_t countSolutions(const CNFFormula& simplified, int maxCount);
    
    // Find next solution different from current assignment
    static bool findNextSolution(const CNFFormula& formula, std::vector<int>& assignment);
    
    // SAT solver
    static bool solveSAT(const CNFFormula& formula, std::vector<int>& assignment, int varIndex);
    
    // CDCL Helper Methods
    static bool cdclSolve(const CNFFormula& formula, std::vector<CDCLAssignment>& assignment, std::vector<Clause>& learnedClauses, WatchedLiterals& watches, VSIDSScores& vsids, int& conflicts, int& restartThreshold);
    
    static bool propagate(const CNFFormula& formula, const std::vector<Clause>& learnedClauses, std::vector<CDCLAssignment>& assignment, WatchedLiterals& watches, int currentLevel, int& conflictClause);
    
    static void analyzeConflict(const CNFFormula& formula, const std::vector<Clause>& learnedClauses, const std::vector<CDCLAssignment>& assignment, int conflictClause, Clause& learnedClause, int& backtrackLevel, VSIDSScores& vsids);
    
    static void initWatches(const CNFFormula& formula, const std::vector<Clause>& learnedClauses, WatchedLiterals& watches, int numVars);
    
    static bool updateWatch(const CNFFormula& formula, const std::vector<Clause>& learnedClauses, int clauseIdx, Literal falseLit, std::vector<CDCLAssignment>& assignment, WatchedLiterals& watches, int currentLevel, int& conflictClause);
};


#endif // APPROXIMATE_COUNTER_H
