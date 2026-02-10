// Source file for PartialAssignment class implementation

#include "solver/partial_assignment.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <iomanip>

using namespace std;

XORSolutionResult PartialAssignment::solveXORSystem(const vector<XORConstraint>& xors, int numVariables) {
    // if there are no XOR constraints, this is trivially satisfiable and all variables are free
    if (xors.empty()) {
        XORSolutionResult result;
        result.satisfiable = true;
        for (int i = 1; i <= numVariables; i++) {
            result.freeVariables.push_back(i);
        }
        return result;
    }
    
    // build augmented matrix for Gaussian elimination
    // rows are XOR constraints, columns are variables + RHS
    vector<vector<int>> matrix;
    vector<int> rhs;
    for (const auto& xor_constraint : xors) {
        vector<int> row(numVariables, 0);
        for (int var : xor_constraint.variables) {
            row[var - 1] = 1;
        }
        matrix.push_back(row);
        rhs.push_back(xor_constraint.value ? 1 : 0);
    }
    
    return gaussianElimination(matrix, rhs, numVariables);
}

XORSolutionResult PartialAssignment::gaussianElimination(vector<vector<int>>& matrix, vector<int>& rhs, int numVariables) {
    XORSolutionResult result;

    // if there are no rows (no constraints), this is trivially satisfiable
    int numRows = matrix.size();
    if (numRows == 0) {
        result.satisfiable = true;
        for (int i = 1; i <= numVariables; i++) {
            result.freeVariables.push_back(i);
        }
        return result;
    }
    
    vector<int> pivot_col(numRows, -1); // get pivot rows for each column
    int current_row = 0;
    
    // 1. forward elimination to RREF
    for (int col = 0; col < numVariables && current_row < numRows; col++) {
        int pivot_row = -1;
        for (int row = current_row; row < numRows; row++) {
            if (matrix[row][col] == 1) {
                pivot_row = row;
                break;
            }
        }
        
        if (pivot_row == -1) {
            continue;
        }
        
        // swap rows
        if (pivot_row != current_row) {
            swap(matrix[pivot_row], matrix[current_row]);
            swap(rhs[pivot_row], rhs[current_row]);
        }
        
        pivot_col[current_row] = col;
        
        // row reduction - eliminate other rows using xor
        for (int row = 0; row < numRows; row++) {
            if (row != current_row && matrix[row][col] == 1) {
                for (int c = 0; c < numVariables; c++) {
                    matrix[row][c] ^= matrix[current_row][c];
                }
                rhs[row] ^= rhs[current_row];
            }
        }
        
        current_row++;
    }
    
    // 2. find contradictions
    for (int row = 0; row < numRows; row++) {
        bool all_zero = true;
        for (int col = 0; col < numVariables; col++) {
            if (matrix[row][col] != 0) {
                all_zero = false;
                break;
            }
        }
        
        // if all coeffs are 0 but rhs is 1, unsat
        if (all_zero && rhs[row] == 1) {
            result.satisfiable = false;
            return result;
        }
    }
    
    // 3. get (partial) assignment and free variables
    vector<bool> is_assigned(numVariables, false);
    
    for (int row = 0; row < numRows; row++) {
        if (pivot_col[row] != -1) {
            int var = pivot_col[row] + 1; // add 1 - variables are 1-indexed
            result.assignment[var] = rhs[row];
            is_assigned[pivot_col[row]] = true;
        }
    }
    
    // get free variables
    for (int i = 0; i < numVariables; i++) {
        if (!is_assigned[i]) {
            result.freeVariables.push_back(i + 1);  // 1-indexed
        }
    }
    
    result.satisfiable = true;
    return result;
}
