// Header for CNF parser

#ifndef CNF_PARSER_H
#define CNF_PARSER_H

#include "cnf/cnf_structure.h"
#include <string>
#include <memory>

// Format: DIMACS CNF
// Lines starting with 'c' are comments
// Line starting with 'p' is the problem line: p cnf <num_vars> <num_clauses>
// Following lines are clauses: positive integers - 0 is end of line (e.g. "1 -3 0" means (x1 OR NOT x3))

class CNFParser {
public:
    // check if a file returns a valid CNF formula
    static bool validateFile(const std::string& filename);

    // parse file and return a CNFFormula object
    static std::unique_ptr<CNFFormula> parseFile(const std::string& filename);

    // parse a CNF formula from a string content
    static std::unique_ptr<CNFFormula> parseString(const std::string& content);
    
private:
    // parse one line
    static bool parseLine(const std::string& line, CNFFormula& formula, bool& foundProblemLine, int& expectedClauses);
    
    // parse the problem line (p cnf <vars> <clauses>)
    static bool parseProblemLine(const std::string& line, int& numVars, int& numClauses);
    
    // parse a clause line (integers - 0 denotes end of clause)
    static Clause parseClause(const std::string& line, CNFFormula& formula);
};

#endif // CNF_PARSER_H
