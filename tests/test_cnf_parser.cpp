// Test suite for CNFParser class

#include <iostream>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include "cnf/cnf_parser.h"
#include "cnf/cnf_structure.h"

using namespace std;

// Helper functions for file operations
void createTestFile(const string& filename, const string& content) {
    ofstream file(filename);
    file << content;
    file.close();
}

void deleteTestFile(const string& filename) {
    remove(filename.c_str());
}

//
// parseString tests
//

void testParseString_validWithComments() {
    string validCNF = 
        "c This is a comment\n"
        "p cnf 3 2\n"
        "1 -2 0\n"
        "2 3 0\n";
    auto formula = CNFParser::parseString(validCNF);
    assert(formula->numVariables == 3);
    assert(formula->numClauses == 2);
    assert(formula->clauses.size() == 2);
}

void testParseString_validWithEmptyLines() {
    string validCNF = 
        "\n"
        "c Comment\n"
        "\n"
        "p cnf 2 1\n"
        "\n"
        "1 2 0\n";
    auto formula = CNFParser::parseString(validCNF);
    assert(formula->numVariables == 2);
    assert(formula->numClauses == 1);
}

void testParseString_validWithMultipleComments() {
    string validCNF = 
        "c First comment\n"
        "c Second comment\n"
        "p cnf 4 2\n"
        "1 2 3 4 0\n"
        "-1 -2 -3 -4 0\n";
    auto formula = CNFParser::parseString(validCNF);
    assert(formula->numVariables == 4);
    assert(formula->clauses.size() == 2);
}

void testParseString_validEmptyFormula() {
    string validCNF = "p cnf 0 0\n";
    auto formula = CNFParser::parseString(validCNF);
    assert(formula->numVariables == 0);
    assert(formula->numClauses == 0);
    assert(formula->clauses.size() == 0);
}

void testParseString_validVariousClauseFormats() {
    string validCNF = 
        "p cnf 3 3\n"
        "1 -2 3 0\n"
        "-3 0\n"
        "1 2 -3 0\n";
    auto formula = CNFParser::parseString(validCNF);
    assert(formula->clauses.size() == 3);
    assert(formula->clauses[0].size() == 3);
    assert(formula->clauses[1].size() == 1);
    assert(formula->clauses[2].size() == 3);
}

void testParseString_errorMultipleProblemLines() {
    string invalidCNF = 
        "p cnf 2 1\n"
        "p cnf 3 2\n"
        "1 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for multiple problem lines");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("Multiple problem lines") != string::npos);
    }
}

void testParseString_errorInvalidProblemLineFormat() {
    string invalidCNF = 
        "p cnf abc 2\n"
        "1 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for invalid problem line");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorClauseBeforeProblemLine() {
    string invalidCNF = 
        "1 2 0\n"
        "p cnf 2 1\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for clause before problem line");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("Clause found before problem line") != string::npos);
    }
}

void testParseString_errorNoProblemLineWithClause() {
    string invalidCNF = 
        "c Just a comment\n"
        "1 2 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for missing problem line");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("Clause found before problem line") != string::npos);
    }
}

void testParseString_errorEmptyFileNoProblemLine() {
    string invalidCNF = "c Only comments\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for no problem line");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("No problem line found") != string::npos);
    }
}

void testParseString_errorClauseCountTooFew() {
    string invalidCNF = 
        "p cnf 2 3\n"
        "1 2 0\n"
        "-1 -2 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for clause count mismatch");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match expected number of clauses") != string::npos);
    }
}

void testParseString_errorClauseCountTooMany() {
    string invalidCNF = 
        "p cnf 2 1\n"
        "1 2 0\n"
        "-1 -2 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for clause count mismatch");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match expected number of clauses") != string::npos);
    }
}

void testParseString_errorVariableCountTooFew() {
    string invalidCNF = 
        "p cnf 5 2\n"
        "1 2 0\n"
        "-1 -2 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for variable count mismatch");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match expected number of variables") != string::npos);
    }
}

void testParseString_errorVariableCountTooMany() {
    string invalidCNF = 
        "p cnf 2 2\n"
        "1 2 3 0\n"
        "-1 -2 -3 0\n";
    try {
        CNFParser::parseString(invalidCNF);
        assert(false && "Should have thrown exception for variable count mismatch");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match expected number of variables") != string::npos);
    }
}

void testParseString_errorWrongFirstToken() {
    string invalidProblem = "q cnf 4 3\n1 0\n2 0\n3 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject wrong first token");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("Clause found before problem line") != string::npos);
    }
}

void testParseString_errorWrongSecondToken() {
    string invalidProblem = "p sat 4 3\n1 0\n2 0\n3 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject wrong second token");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorNonIntegerVariables() {
    string invalidProblem = "p cnf abc 3\n1 0\n2 0\n3 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject non-integer variables");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorNonIntegerClauses() {
    string invalidProblem = "p cnf 4 xyz\n1 0\n2 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject non-integer clauses");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorNegativeVariables() {
    string invalidProblem = "p cnf -4 3\n1 0\n2 0\n3 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject negative variables");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorZeroVariablesWithClauses() {
    string invalidProblem = "p cnf 0 3\n1 0\n2 0\n3 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject zero variables with clauses that use variables");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match expected number of variables") != string::npos);
    }
}

void testParseString_errorNegativeClauses() {
    string invalidProblem = "p cnf 4 -3\n1 0\n2 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject negative clauses");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorExtraContentAfterProblemLine() {
    string invalidProblem = "p cnf 4 2 extra\n1 0\n2 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject extra content");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

void testParseString_errorIncompleteProblemLine() {
    string invalidProblem = "p cnf 4\n1 0\n2 0\n";
    try {
        CNFParser::parseString(invalidProblem);
        assert(false && "Should reject incomplete problem line");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not in the expected format") != string::npos);
    }
}

// orchestrator
void testParseString() {
    cout << "Testing parseString..." << endl;
    testParseString_validWithComments();
    testParseString_validWithEmptyLines();
    testParseString_validWithMultipleComments();
    testParseString_validEmptyFormula();
    testParseString_validVariousClauseFormats();
    testParseString_errorMultipleProblemLines();
    testParseString_errorInvalidProblemLineFormat();
    testParseString_errorClauseBeforeProblemLine();
    testParseString_errorNoProblemLineWithClause();
    testParseString_errorEmptyFileNoProblemLine();
    testParseString_errorClauseCountTooFew();
    testParseString_errorClauseCountTooMany();
    testParseString_errorVariableCountTooFew();
    testParseString_errorVariableCountTooMany();
    testParseString_errorWrongFirstToken();
    testParseString_errorWrongSecondToken();
    testParseString_errorNonIntegerVariables();
    testParseString_errorNonIntegerClauses();
    testParseString_errorNegativeVariables();
    testParseString_errorZeroVariablesWithClauses();
    testParseString_errorNegativeClauses();
    testParseString_errorExtraContentAfterProblemLine();
    testParseString_errorIncompleteProblemLine();
    cout << "  All parseString tests passed!" << endl;
}

//
// parseFile tests
//

void testParseFile_validFile() {
    string testFile = "test_valid.cnf";
    createTestFile(testFile, 
        "c Test file\n"
        "p cnf 3 2\n"
        "1 -2 3 0\n"
        "-1 2 -3 0\n");
    
    auto formula = CNFParser::parseFile(testFile);
    assert(formula->numVariables == 3);
    assert(formula->numClauses == 2);
    deleteTestFile(testFile);
}

void testParseFile_errorFileNotFound() {
    try {
        CNFParser::parseFile("nonexistent_file.cnf");
        assert(false && "Should have thrown exception for file not found");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("not found") != string::npos);
    }
}

void testParseFile_errorInvalidFileContent() {
    string testFile = "test_invalid.cnf";
    createTestFile(testFile, 
        "p cnf 2 2\n"
        "1 0\n");  // Only 1 clause, expected 2
    
    try {
        CNFParser::parseFile(testFile);
        assert(false && "Should have thrown exception for invalid content");
    } catch (const runtime_error& e) {
        assert(string(e.what()).find("does not match") != string::npos);
    }
    deleteTestFile(testFile);
}

// Orchestrator function for parseFile tests
void testParseFile() {
    cout << "Testing parseFile..." << endl;
    testParseFile_validFile();
    testParseFile_errorFileNotFound();
    testParseFile_errorInvalidFileContent();
    cout << "  All parseFile tests passed!" << endl;
}

//
// validateFile tests
//

void testValidateFile_validFile() {
    string testFile = "test_validate_valid.cnf";
    createTestFile(testFile, 
        "p cnf 2 1\n"
        "1 -2 0\n");
    assert(CNFParser::validateFile(testFile) == true);
    deleteTestFile(testFile);
}

void testValidateFile_errorFileDoesNotExist() {
    assert(CNFParser::validateFile("nonexistent.cnf") == false);
}

void testValidateFile_errorInvalidFormat() {
    string testFile = "test_validate_invalid.cnf";
    createTestFile(testFile, 
        "p cnf 2 2\n"
        "1 0\n");  // Wrong number of clauses
    assert(CNFParser::validateFile(testFile) == false);
    deleteTestFile(testFile);
}

// Orchestrator function for validateFile tests
void testValidateFile() {
    cout << "Testing validateFile..." << endl;
    testValidateFile_validFile();
    testValidateFile_errorFileDoesNotExist();
    testValidateFile_errorInvalidFormat();
    cout << "  All validateFile tests passed!" << endl;
}

//
// Main test runner
//

int main() {
    cout << "**Running CNF Parser Tests..." << endl;
    
    testParseString();
    testParseFile();
    testValidateFile();
    
    cout << "**All CNF Parser tests passed!" << endl;
    
    return 0;
}
