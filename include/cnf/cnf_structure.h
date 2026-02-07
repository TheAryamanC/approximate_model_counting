// Header for CNF formula structure

#ifndef CNF_STRUCTURE_H
#define CNF_STRUCTURE_H

#include <vector>
#include <string>
#include <unordered_set>

// System of equations is: AND of ORS of literals

// Literals - positive integer for variable, negative integer for NOT variable
using Literal = int;

// Clause is OR of literals
class Clause {
public:
    std::vector<Literal> literals;
    
    Clause() = default;
    explicit Clause(const std::vector<Literal>& lits) : literals(lits) {}
    
    void addLiteral(Literal lit) { literals.push_back(lit); }
    size_t size() const { return literals.size(); }
    bool empty() const { return literals.empty(); }
    
    // Check if clause is satisfied by an assignment
    bool isSatisfied(const std::vector<int>& assignment) const;
    bool isUnsatisfiable(const std::vector<int>& assignment) const;
};

// CNF formula is AND of clauses
class CNFFormula {
public:
    int numVariables;
    int numClauses;
    std::vector<Clause> clauses;
    
    CNFFormula() : numVariables(0), numClauses(0) {}
    CNFFormula(int vars, int cls) : numVariables(vars), numClauses(cls) {}
    
    void addClause(const Clause& clause) { clauses.push_back(clause); };
    void addClause(const std::vector<Literal>& literals) { clauses.push_back(Clause(literals)); }
    
    // Get all variables
    std::unordered_set<int> getVariables() const;
    
    // Check if formula is satisfied by an assignment
    bool isSatisfied(const std::vector<int>& assignment) const;
    
    size_t getNumClauses() const { return clauses.size(); }
    int getNumVariables() const { return numVariables; }
    void clear();
};

#endif // CNF_STRUCTURE_H
