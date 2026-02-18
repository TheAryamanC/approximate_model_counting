// Source file for approximate model counting

#include "solver/approximate_counter.h"
#include "solver/partial_assignment.h"
#include "solver/cnf_simplifier.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

// run multiple trials of approximate counting and aggregate results
ApproximationResult ApproximateCounter::approximateCount(const CNFFormula& formula, int numTrials, int numXORs, double density) {
    vector<TrialResult> trials;
    trials.reserve(numTrials);
    
    for (int i = 0; i < numTrials; i++) {
        TrialResult trial = singleTrial(formula, density, 50);
        trials.push_back(trial);
    }
    
    return aggregateResults(trials);
}

// run a single trial with adaptive XOR count
TrialResult ApproximateCounter::singleTrial(const CNFFormula& formula, double density, int threshold) {
    TrialResult result;
    int numVariables = formula.getNumVariables();
    int numXORs = 0;
    
    // add XORs until solution space is small enough
    while (numXORs < numVariables) {
        auto xors = XORHashGenerator::generateXORFamily(numVariables, numXORs, density);
        auto xorSolution = PartialAssignment::solveXORSystem(xors, numVariables);
        
        if (!xorSolution.satisfiable) {
            // too many XORs
            if (numXORs == 0) {
                result.satisfiable = false;
                result.solutionCount = 0;
                result.numXORs = 0;
                return result;
            }
            numXORs--;
            break;
        }
        
        auto simplified = CNFSimplifier::applyXORSolution(formula, xorSolution);
        
        if (simplified.isUnsatisfiable) {
            if (numXORs == 0) {
                result.satisfiable = false;
                result.solutionCount = 0;
                result.numXORs = 0;
                return result;
            }
            numXORs--;
            break;
        }
        
        uint64_t cellCount = countSolutions(simplified.simplified, threshold + 10);
        
        if (cellCount == 0) {
            if (numXORs == 0) {
                result.satisfiable = false;
                result.solutionCount = 0;
                result.numXORs = 0;
                return result;
            }
            numXORs--;
            break;
        }
        
        if (cellCount > 0 && cellCount <= threshold) {
            result.satisfiable = true;
            result.numXORs = numXORs;
            result.freeVariables = xorSolution.freeVariables.size();
            result.assignedVariables = xorSolution.assignment.size();
            
            // scale up get estimate based on number of XORs added
            uint64_t scaleFactor = (numXORs < 64) ? (1ULL << numXORs) : UINT64_MAX;
            if (cellCount > UINT64_MAX / scaleFactor) {
                result.solutionCount = UINT64_MAX;
            } else {
                result.solutionCount = cellCount * scaleFactor;
            }
            return result;
        }
        
        // cell count is still too high, add more XORs
        numXORs++;
    }
    
    // if we exit loop without returning, we have found a good number of XORs to get a small cell count, so do final count and return result
    auto xors = XORHashGenerator::generateXORFamily(numVariables, numXORs, density);
    auto xorSolution = PartialAssignment::solveXORSystem(xors, numVariables);
    auto simplified = CNFSimplifier::applyXORSolution(formula, xorSolution);
    
    result.numXORs = numXORs;
    result.freeVariables = xorSolution.freeVariables.size();
    result.assignedVariables = xorSolution.assignment.size();
    
    uint64_t cellCount = countSolutions(simplified.simplified, threshold + 10);
    uint64_t scaleFactor = (numXORs < 64) ? (1ULL << numXORs) : UINT64_MAX;
    
    result.satisfiable = (cellCount > 0);
    if (cellCount > UINT64_MAX / scaleFactor) {
        result.solutionCount = UINT64_MAX;
    } else {
        result.solutionCount = cellCount * scaleFactor;
    }
    
    return result;
}

// aggregate results from multiple trials to get final approximation
ApproximationResult ApproximateCounter::aggregateResults(const vector<TrialResult>& trials) {
    ApproximationResult result;
    result.totalTrials = trials.size();
    
    // get counts from successful trials
    for (const auto& trial : trials) {
        if (trial.satisfiable) {
            result.successfulTrials++;
            result.trialCounts.push_back(trial.solutionCount);
        }
    }
    
    if (result.successfulTrials == 0) {
        // all unsatisfiable - return 0
        result.estimatedCount = 0;
        result.averageCount = 0.0;
        return result;
    }
    
    // median used - more robust to outliers than mean
    vector<uint64_t> sortedCounts = result.trialCounts;
    sort(sortedCounts.begin(), sortedCounts.end());
    
    size_t medianIndex = sortedCounts.size() / 2;
    if (sortedCounts.size() % 2 == 0) {
        result.estimatedCount = (sortedCounts[medianIndex - 1] + sortedCounts[medianIndex]) / 2;
    } else {
        result.estimatedCount = sortedCounts[medianIndex];
    }
    
    // get average
    uint64_t sum = accumulate(result.trialCounts.begin(), result.trialCounts.end(), 0ULL);
    result.averageCount = static_cast<double>(sum) / result.trialCounts.size();
    
    return result;
}

// count solutions in simplified CNF up to maxCount
uint64_t ApproximateCounter::countSolutions(const CNFFormula& formula, int maxCount) {
    if (formula.clauses.empty()) {
        // empty formula is always true
        if (formula.numVariables >= 64) return UINT64_MAX;
        return 1ULL << formula.numVariables;
    }
    
    uint64_t count = 0;
    vector<int> assignment(formula.numVariables, -1);
    
    // make sure there is at least one solution
    if (!solveSAT(formula, assignment, 0)) {
        return 0;  // UNSAT
    }
    
    count = 1;
    
    while (count < maxCount) {
        if (!findNextSolution(formula, assignment)) {
            break;
        }
        count++;
    }
    
    return count;
}

// find next solution different from current assignment by adding blocking clause and solving again
bool ApproximateCounter::findNextSolution(const CNFFormula& formula, vector<int>& assignment) {
    // keep flipping variables until we find a new solution or exhaust all possibilities -> to be changed in the future
    for (int i = formula.numVariables - 1; i >= 0; i--) {
        int originalValue = assignment[i];
        
        // flip variable to block current solution
        assignment[i] = 1 - originalValue;
        vector<int> newAssignment = assignment;
        if (solveSAT(formula, newAssignment, i + 1)) {
            assignment = newAssignment;
            return true;
        }
        
        // restore original value and try next variable
        assignment[i] = originalValue;
    }
    
    return false;
}

// cdcl sat solver
bool ApproximateCounter::solveSAT(const CNFFormula& formula, vector<int>& assignment, int varIndex) {
    // Ensure assignment vector is properly sized
    if (assignment.size() < formula.numVariables) {
        assignment.resize(formula.numVariables, -1);
    }
    
    vector<CDCLAssignment> cdclAssignment(formula.numVariables);
    for (int i = 0; i < formula.numVariables; i++) {
        cdclAssignment[i].value = assignment[i];
        cdclAssignment[i].decisionLevel = (assignment[i] != -1) ? 0 : -1;
        cdclAssignment[i].antecedent = -1;
    }
    
    vector<Clause> learnedClauses;
    WatchedLiterals watches;
    watches.init(formula.numVariables, formula.clauses.size());
    
    VSIDSScores vsids;
    vsids.init(formula.numVariables);
    
    initWatches(formula, learnedClauses, watches, formula.numVariables);
    
    int conflicts = 0;
    int restartThreshold = 100;
    
    bool result = cdclSolve(formula, cdclAssignment, learnedClauses, watches, vsids, conflicts, restartThreshold);
    
    for (int i = 0; i < formula.numVariables; i++) {
        assignment[i] = cdclAssignment[i].value;
    }
    
    return result;
}

bool ApproximateCounter::cdclSolve(const CNFFormula& formula, vector<CDCLAssignment>& assignment, vector<Clause>& learnedClauses, WatchedLiterals& watches, VSIDSScores& vsids, int& conflicts, int& restartThreshold) {
    int decisionLevel = 0;
    vector<int> trail;        // stack of assigned variables in order of assignment
    vector<int> trailLevels;  // index in trail where each decision level starts
    trailLevels.push_back(0);
    
    while (true) {
        // 1. Propagation
        int conflictClause = -1;
        if (!propagate(formula, learnedClauses, assignment, watches, decisionLevel, conflictClause)) {
            // Conflict occurred
            if (decisionLevel == 0) {
                return false;  // UNSAT at root level
            }
            
            // analyze conflict and learn clause
            Clause learnedClause;
            int backtrackLevel = 0;
            analyzeConflict(formula, learnedClauses, assignment, conflictClause, learnedClause, backtrackLevel, vsids);
            
            // backtrack to appropriate level BEFORE adding learned clause
            while (decisionLevel > backtrackLevel) {
                int levelStart = trailLevels[decisionLevel];
                for (int i = trail.size() - 1; i >= levelStart; i--) {
                    assignment[trail[i]].value = -1;
                    assignment[trail[i]].decisionLevel = -1;
                    assignment[trail[i]].antecedent = -1;
                }
                trail.resize(levelStart);
                trailLevels.pop_back();
                decisionLevel--;
            }
            
            // Now add and propagate the learned clause
            learnedClauses.push_back(learnedClause);
            int learnedIdx = formula.clauses.size() + learnedClauses.size() - 1;
            
            // Set up watches for learned clause
            if (learnedClause.literals.size() >= 2) {
                int lit0 = learnedClause.literals[0];
                int lit1 = learnedClause.literals[1];
                int idx0 = watches.litToIndex(lit0, formula.numVariables);
                int idx1 = watches.litToIndex(lit1, formula.numVariables);
                watches.watches[idx0].push_back(learnedIdx);
                watches.watches[idx1].push_back(learnedIdx);
            } else if (learnedClause.literals.size() == 1) {
                // Unit clause - propagate immediately
                Literal lit = learnedClause.literals[0];
                int var = abs(lit) - 1;
                if (var >= 0 && var < formula.numVariables && assignment[var].value == -1) {
                    assignment[var].value = (lit > 0) ? 1 : 0;
                    assignment[var].decisionLevel = decisionLevel;
                    assignment[var].antecedent = learnedIdx;
                    trail.push_back(var);
                }
            }
            
            conflicts++;
            vsids.decayAll();
            
            // restart if too many conflicts
            if (conflicts >= restartThreshold) {
                for (int i = 0; i < formula.numVariables; i++) {
                    if (assignment[i].decisionLevel > 0) {
                        assignment[i].value = -1;
                        assignment[i].decisionLevel = -1;
                        assignment[i].antecedent = -1;
                    }
                }
                trail.clear();
                trailLevels.clear();
                trailLevels.push_back(0);
                decisionLevel = 0;
                conflicts = 0;
                restartThreshold = (int)(restartThreshold * 1.5);
            }
            
            continue;
        }
        
        // 2. Check if all variables assigned
        int unassignedVar = -1;
        for (int i = 0; i < formula.numVariables; i++) {
            if (assignment[i].value == -1) {
                unassignedVar = i;
                break;
            }
        }
        
        // all variables assigned and no conflict -> SAT
        if (unassignedVar == -1) {
            return true;
        }
        
        // 3. Decision - pick unassigned variable with highest VSIDS score
        int decisionVar = vsids.selectUnassigned(assignment);
        if (decisionVar == -1) {
            return true;
        }
        
        int decisionValue = 1;
        decisionLevel++;
        trailLevels.push_back(trail.size());
        
        assignment[decisionVar].value = decisionValue;
        assignment[decisionVar].decisionLevel = decisionLevel;
        assignment[decisionVar].antecedent = -1;
        trail.push_back(decisionVar);
    }
}

// propagate to deduce new assignments
bool ApproximateCounter::propagate(const CNFFormula& formula, const vector<Clause>& learnedClauses, vector<CDCLAssignment>& assignment, WatchedLiterals& watches, int currentLevel, int& conflictClause) {
    vector<int> propagationQueue;
    
    // find all assigned variables at or below current level that haven't been propagated
    // (assignments at current level definitely need propagation)
    for (int i = 0; i < formula.numVariables; i++) {
        if (assignment[i].value != -1 && assignment[i].decisionLevel == currentLevel) {
            propagationQueue.push_back(i);
        }
    }
    
    // If queue is empty (e.g., after backtracking), check for unit clauses
    if (propagationQueue.empty()) {
        // Scan all clauses for unit clauses
        auto checkForUnitClauses = [&](const vector<Clause>& clauses, int offset) {
            for (size_t ci = 0; ci < clauses.size(); ci++) {
                const Clause& clause = clauses[ci];
                int unassignedLit = 0;
                int unassignedCount = 0;
                bool satisfied = false;
                
                for (Literal lit : clause.literals) {
                    int var = abs(lit) - 1;
                    if (var < 0 || var >= formula.numVariables) continue;
                    
                    if (assignment[var].value == -1) {
                        unassignedLit = lit;
                        unassignedCount++;
                    } else {
                        bool litVal = (lit > 0) ? (assignment[var].value == 1) : (assignment[var].value == 0);
                        if (litVal) {
                            satisfied = true;
                            break;
                        }
                    }
                }
                
                if (!satisfied && unassignedCount == 1 && unassignedLit != 0) {
                    // Found unit clause - assign it
                    int var = abs(unassignedLit) - 1;
                    if (var >= 0 && var < formula.numVariables) {
                        assignment[var].value = (unassignedLit > 0) ? 1 : 0;
                        assignment[var].decisionLevel = currentLevel;
                        assignment[var].antecedent = offset + ci;
                        propagationQueue.push_back(var);
                    }
                } else if (!satisfied && unassignedCount == 0) {
                    // Conflict
                    conflictClause = offset + ci;
                    return false;
                }
            }
            return true;
        };
        
        if (!checkForUnitClauses(formula.clauses, 0)) return false;
        if (!checkForUnitClauses(learnedClauses, formula.clauses.size())) return false;
    }
    
    size_t queuePos = 0;
    while (queuePos < propagationQueue.size()) {
        int var = propagationQueue[queuePos++];
        
        // Bounds check
        if (var < 0 || var >= assignment.size() || var >= formula.numVariables) {
            continue;
        }
        
        int value = assignment[var].value;
        
        // get the literal that is now false due to this assignment
        Literal falseLit = (value == 1) ? (-(var + 1)) : (var + 1);
        int watchIdx = watches.litToIndex(falseLit, formula.numVariables);
        
        // check clauses with this literal
        auto& watchList = watches.watches[watchIdx];
        size_t i = 0;
        while (i < watchList.size()) {
            int clauseIdx = watchList[i];
            
            // update watch and check for conflict or propagation
            if (!updateWatch(formula, learnedClauses, clauseIdx, falseLit, assignment, watches, currentLevel, conflictClause)) {
                if (conflictClause != -1) {
                    return false;  // Conflict
                }
                // add to queue to propagate
                for (int j = 0; j < formula.numVariables; j++) {
                    if (assignment[j].value != -1 && assignment[j].decisionLevel == currentLevel && 
                        assignment[j].antecedent == clauseIdx) {
                        bool alreadyInQueue = false;
                        for (int k = queuePos; k < propagationQueue.size(); k++) {
                            if (propagationQueue[k] == j) {
                                alreadyInQueue = true;
                                break;
                            }
                        }
                        if (!alreadyInQueue) {
                            propagationQueue.push_back(j);
                        }
                    }
                }
            }
            
            // check if watch was removed
            if (i < watchList.size() && watchList[i] == clauseIdx) {
                i++;
            }
        }
    }
    
    return true;
}

bool ApproximateCounter::updateWatch(const CNFFormula& formula, const vector<Clause>& learnedClauses, int clauseIdx, Literal falseLit, vector<CDCLAssignment>& assignment, WatchedLiterals& watches, int currentLevel, int& conflictClause) {
    // get clause
    const Clause* clause = nullptr;
    if (clauseIdx < formula.clauses.size()) {
        clause = &formula.clauses[clauseIdx];
    } else {
        clause = &learnedClauses[clauseIdx - formula.clauses.size()];
    }
    
    // find the two watched literals
    Literal watch1 = 0, watch2 = 0;
    int watch1Pos = -1, watch2Pos = -1;
    
    for (size_t i = 0; i < clause->literals.size(); i++) {
        Literal lit = clause->literals[i];
        int var = abs(lit) - 1;
        
        if (var >= formula.numVariables) continue;
        
        // check if this literal is watched
        int litIdx = watches.litToIndex(lit, formula.numVariables);
        bool isWatched = false;
        for (int cidx : watches.watches[litIdx]) {
            if (cidx == clauseIdx) {
                isWatched = true;
                break;
            }
        }
        
        if (isWatched) {
            if (watch1 == 0) {
                watch1 = lit;
                watch1Pos = i;
            } else {
                watch2 = lit;
                watch2Pos = i;
                break;
            }
        }
    }
    
    if (watch1 != falseLit && watch2 != falseLit) {
        return true;
    }
    
    // find a new watch
    for (size_t i = 0; i < clause->literals.size(); i++) {
        if (i == watch1Pos || i == watch2Pos) continue;
        
        Literal lit = clause->literals[i];
        int var = abs(lit) - 1;
        if (var < 0 || var >= formula.numVariables || var >= assignment.size()) continue;
        
        // check if this literal is not false
        bool isUnassigned = (assignment[var].value == -1);
        bool isSatisfied = false;
        if (!isUnassigned) {
            isSatisfied = (lit > 0) ? (assignment[var].value == 1) : (assignment[var].value == 0);
        }
        
        if (isUnassigned || isSatisfied) {
            // found a new watch -> remove old watch and add new watch
            int oldIdx = watches.litToIndex(falseLit, formula.numVariables);
            auto& oldList = watches.watches[oldIdx];
            for (auto it = oldList.begin(); it != oldList.end(); ++it) {
                if (*it == clauseIdx) {
                    oldList.erase(it);
                    break;
                }
            }
            int newIdx = watches.litToIndex(lit, formula.numVariables);
            watches.watches[newIdx].push_back(clauseIdx);
            return true;
        }
    }
    
    // no new watch found, check if other watch is satisfied or if we have a conflict
    Literal otherWatch = (watch1 == falseLit) ? watch2 : watch1;
    int otherVar = abs(otherWatch) - 1;
    
    if (otherVar < 0 || otherVar >= formula.numVariables || otherVar >= assignment.size()) {
        conflictClause = clauseIdx;
        return false;
    }
    
    // Check if the other watch is satisfied
    bool otherSatisfied = false;
    if (assignment[otherVar].value != -1) {
        otherSatisfied = (otherWatch > 0) ? (assignment[otherVar].value == 1) : (assignment[otherVar].value == 0);
    }
    
    if (assignment[otherVar].value == -1) {
        // propagate
        assignment[otherVar].value = (otherWatch > 0) ? 1 : 0;
        assignment[otherVar].decisionLevel = currentLevel;
        assignment[otherVar].antecedent = clauseIdx;
        return false;
    } else if (otherSatisfied) {
        return true; // clause satisfied
    } else {
        conflictClause = clauseIdx;
        return false; // conflict
    }
}

void ApproximateCounter::analyzeConflict(const CNFFormula& formula, const vector<Clause>& learnedClauses, const vector<CDCLAssignment>& assignment, int conflictClause, Clause& learnedClause, int& backtrackLevel, VSIDSScores& vsids) {
    const Clause* clause = nullptr;
    if (conflictClause < formula.clauses.size()) {
        clause = &formula.clauses[conflictClause];
    } else {
        clause = &learnedClauses[conflictClause - formula.clauses.size()];
    }
    
    // find highest decision level in the conflict clause
    int conflictLevel = 0;
    for (Literal lit : clause->literals) {
        int var = abs(lit) - 1;
        if (var < formula.numVariables && assignment[var].decisionLevel > conflictLevel) {
            conflictLevel = assignment[var].decisionLevel;
        }
    }
    
    // learn conflict
    learnedClause.literals = clause->literals;
    for (Literal lit : learnedClause.literals) {
        int var = abs(lit) - 1;
        if (var < formula.numVariables) {
            vsids.bump(var);
        }
    }
    
    // find backtrack level
    vector<int> levels;
    for (Literal lit : learnedClause.literals) {
        int var = abs(lit) - 1;
        if (var < formula.numVariables) {
            int level = assignment[var].decisionLevel;
            if (level >= 0 && find(levels.begin(), levels.end(), level) == levels.end()) {
                levels.push_back(level);
            }
        }
    }
    
    sort(levels.begin(), levels.end());
    backtrackLevel = (levels.size() > 1) ? levels[levels.size() - 2] : 0;
}

void ApproximateCounter::initWatches(const CNFFormula& formula, const vector<Clause>& learnedClauses, WatchedLiterals& watches, int numVars) {
    // two watches per clause
    for (size_t i = 0; i < formula.clauses.size(); i++) {
        const Clause& clause = formula.clauses[i];
        if (clause.literals.size() >= 2) {
            Literal lit0 = clause.literals[0];
            Literal lit1 = clause.literals[1];
            int idx0 = watches.litToIndex(lit0, numVars);
            int idx1 = watches.litToIndex(lit1, numVars);
            watches.watches[idx0].push_back(i);
            watches.watches[idx1].push_back(i);
        } else if (clause.literals.size() == 1) {
            Literal lit0 = clause.literals[0];
            int idx0 = watches.litToIndex(lit0, numVars);
            watches.watches[idx0].push_back(i);
        }
    }
}
