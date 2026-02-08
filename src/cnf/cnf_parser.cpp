// Source file for CNFParser class implementation

#include "cnf/cnf_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace std;

bool CNFParser::validateFile(const std::string& filename) {
    try {
        parseFile(filename);
        return true;
    } catch (const std::exception& e) {
        cerr << "Something went wrong: " << e.what() << endl;
        return false;
    }
}

unique_ptr<CNFFormula> CNFParser::parseFile(const std::string& filename) {
    // file must end with .cnf
    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".cnf") {
        throw runtime_error("File " + filename + " must have .cnf extension");
    }
    
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("File " + filename + " not found");
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return parseString(buffer.str());
}

unique_ptr<CNFFormula> CNFParser::parseString(const std::string& content) {
    auto formula = make_unique<CNFFormula>();
    istringstream stream(content);
    string line;
    bool foundProblemLine = false;
    int expectedClauses = 0;
    int parsedClauses = 0;
    
    while (std::getline(stream, line)) {
        // empty lines can be skipped
        if (line.empty()){
            continue;
        }
        
        // comment lines can be skipped
        if (line[0] == 'c'){
            continue;
        }
        
        // parse problem line
        if (line[0] == 'p') {
            // only one problem line should be present
            if (foundProblemLine) {
                throw runtime_error("Multiple problem lines found");
            }
            // if problem line is invalid, throw error
            if (!parseProblemLine(line, formula->numVariables, formula->numClauses)) {
                throw runtime_error("Problem line " + line + " is not in the expected format (p cnf <num_vars> <num_clauses>)");
            }
            foundProblemLine = true;
            expectedClauses = formula->numClauses;
            continue;
        }
        
        // clauses come after the problem line
        if (!foundProblemLine) {
            throw runtime_error("Clause found before problem line");
        }
        
        // add all clauses to the formula
        Clause clause = parseClause(line, *formula);
        if (!clause.empty()) {
            formula->addClause(clause);
            parsedClauses++;
        }
    }
    
    // if there is no problem line, the file is invalid
    if (!foundProblemLine) {
        throw runtime_error("No problem line found in CNF file");
    }
    
    //
    // if there is a mismatch between the problem statement and file, the file is invalid
    //
    //// number of clauses
    if (parsedClauses != expectedClauses) {
        throw runtime_error("Number of clauses parsed (" + to_string(parsedClauses) + ") does not match expected number of clauses (" + to_string(expectedClauses) + ")");
    }
    //// number of variables
    if (static_cast<int>(formula->variablesSeen.size()) != formula->numVariables) {
        throw runtime_error("Number of variables parsed (" + to_string(formula->variablesSeen.size()) + ") does not match expected number of variables (" + to_string(formula->numVariables) + ")");
    }
    
    return formula;
}

// problem line format: p cnf <num_vars> <num_clauses>
bool CNFParser::parseProblemLine(const std::string& line, int& numVars, int& numClauses) {
    istringstream iss(line);
    string p, cnf;
    
    // get the four expected parts and verify they were parsed correctly
    if (!(iss >> p >> cnf >> numVars >> numClauses)) {
        return false;
    }
    if (p != "p" || cnf != "cnf" || numVars < 0 || numClauses < 0) {
        return false;
    }
    
    // there should be no extra content after the expected four parts
    string extra;
    if (iss >> extra) {
        return false;
    }
    
    return true;
}

// clause line format: integers (0 denotes end of clause)
Clause CNFParser::parseClause(const std::string& line, CNFFormula& formula) {
    Clause clause;
    istringstream iss(line);
    int literal;
    
    while (iss >> literal) {
        if (literal == 0) {
            break;
        }
        
        // Validate that the variable ID is within the expected range
        int varId = std::abs(literal);
        if (varId > formula.numVariables) {
            throw runtime_error("Invalid literal " + to_string(literal) + ": variable ID " + to_string(varId) + " exceeds maximum " + to_string(formula.numVariables));
        }
        
        clause.addLiteral(literal);
        formula.variablesSeen.insert(varId);
    }
    
    return clause;
}
