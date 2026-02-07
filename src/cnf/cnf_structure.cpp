// Source file for CNF formula and clause structure implementation

#include "cnf/cnf_structure.h"
#include <algorithm>
#include <cmath>

using namespace std;

//
// CLAUSE IMPLEMENTATION
//
// Note: certain functions are already in header - non-trivial ones are implemented here

bool Clause::isSatisfied(const vector<int>& assignment) const {
    for (Literal l : literals) {
        int var = std::abs(l);
        if (var <= static_cast<int>(assignment.size())) {
            int value = assignment[var - 1]; // get variable assignment (1 for true, 0 for false, -1 for unassigned)
            if (value == -1) {
                continue;
            }
            bool satisfied = (l > 0) ? (value == 1) : (value == 0);
            if (satisfied) {
                return true;
            }
        }
    }
    return false;
}

bool Clause::isUnsatisfiable(const vector<int>& assignment) const {
    bool hasUnassignedLiterals = false;
    for (Literal l : literals) {
        int var = std::abs(l);
        if (var <= static_cast<int>(assignment.size())) {
            int value = assignment[var - 1];
            if (value == -1) {
                hasUnassignedLiterals = true;
            } else {
                bool satisfied = (l > 0) ? (value == 1) : (value == 0);
                if (satisfied) {
                    return false;
                }
            }
        }
    }
    // the clause is unsatisfiable if all literals are assigned and none are satisfied
    return !hasUnassignedLiterals;
}


//
// CNFFormula IMPLEMENTATION
//
// Note: certain functions are already in header - non-trivial ones are implemented here

unordered_set<int> CNFFormula::getVariables() const {
    std::unordered_set<int> variables;
    for (const auto& clause : clauses) {
        for (Literal l : clause.literals) {
            variables.insert(std::abs(l));
        }
    }
    return variables;
}

bool CNFFormula::isSatisfied(const std::vector<int>& assignment) const {
    for (const auto& clause : clauses) {
        if (!clause.isSatisfied(assignment)) {
            return false;
        }
    }
    return true;
}

// STL containers have clear() method that deallocates memory, so we can just call that and reset counts
void CNFFormula::clear() {
    clauses.clear();
    variablesSeen.clear();
    numVariables = 0;
    numClauses = 0;
}
