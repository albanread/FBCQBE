//
// cfg_builder_functions.cpp
// FasterBASIC - Control Flow Graph Builder Function/Sub Handlers
//
// Contains FUNCTION, DEF FN, and SUB processing.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Function Handlers
// =============================================================================

void CFGBuilder::processFunctionStatement(const FunctionStatement& stmt, BasicBlock* currentBlock) {
    // Create a new CFG for this function
    ControlFlowGraph* funcCFG = m_programCFG->createFunctionCFG(stmt.functionName);
    
    // Store function metadata
    funcCFG->functionName = stmt.functionName;
    funcCFG->parameters = stmt.parameters;
    
    // Process parameter types - check both AS types and type suffixes
    for (size_t i = 0; i < stmt.parameters.size(); i++) {
        VariableType vt = VariableType::DOUBLE;  // Default type
        
        // First check if there's an AS typename declaration
        if (i < stmt.parameterAsTypes.size() && !stmt.parameterAsTypes[i].empty()) {
            std::string asType = stmt.parameterAsTypes[i];
            // Convert to uppercase for case-insensitive comparison
            std::string upperType = asType;
            std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);
            
            if (upperType == "INTEGER" || upperType == "INT") {
                vt = VariableType::INT;
            } else if (upperType == "DOUBLE") {
                vt = VariableType::DOUBLE;
            } else if (upperType == "SINGLE" || upperType == "FLOAT") {
                vt = VariableType::FLOAT;
            } else if (upperType == "STRING") {
                vt = VariableType::STRING;
            } else if (upperType == "LONG") {
                vt = VariableType::INT;
            }
            // TODO: Handle user-defined types
        } else if (i < stmt.parameterTypes.size()) {
            // Check type suffix
            switch (stmt.parameterTypes[i]) {
                case TokenType::TYPE_INT: vt = VariableType::INT; break;
                case TokenType::TYPE_FLOAT: vt = VariableType::FLOAT; break;
                case TokenType::TYPE_DOUBLE: vt = VariableType::DOUBLE; break;
                case TokenType::TYPE_STRING: vt = VariableType::STRING; break;
                default: break;
            }
        }
        
        funcCFG->parameterTypes.push_back(vt);
    }
    
    // Set return type
    if (stmt.hasReturnAsType) {
        // TODO: Map returnTypeAsName to VariableType
        funcCFG->returnType = VariableType::INT; // Default for now
    } else {
        switch (stmt.returnTypeSuffix) {
            case TokenType::TYPE_INT: funcCFG->returnType = VariableType::INT; break;
            case TokenType::TYPE_FLOAT: funcCFG->returnType = VariableType::FLOAT; break;
            case TokenType::TYPE_DOUBLE: funcCFG->returnType = VariableType::DOUBLE; break;
            case TokenType::TYPE_STRING: funcCFG->returnType = VariableType::STRING; break;
            default: funcCFG->returnType = VariableType::INT; break;
        }
    }
    
    // Save current CFG context
    ControlFlowGraph* savedCFG = m_currentCFG;
    BasicBlock* savedBlock = m_currentBlock;
    
    // Switch to function CFG
    m_currentCFG = funcCFG;
    
    // Create entry block for function
    BasicBlock* entryBlock = createNewBlock("Function Entry");
    funcCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // Process function body statements
    for (const auto& bodyStmt : stmt.body) {
        if (bodyStmt) {
            processStatement(*bodyStmt, m_currentBlock, 0);
        }
    }
    
    // Create exit block
    if (m_createExitBlock) {
        BasicBlock* exitBlock = createNewBlock("Function Exit");
        exitBlock->isTerminator = true;
        funcCFG->exitBlock = exitBlock->id;
        
        if (m_currentBlock && m_currentBlock->id != exitBlock->id) {
            addFallthroughEdge(m_currentBlock->id, exitBlock->id);
        }
    }
    
    // Restore context
    m_currentCFG = savedCFG;
    m_currentBlock = savedBlock;
}

void CFGBuilder::processDefStatement(const DefStatement& stmt, BasicBlock* currentBlock) {
    // DEF FN creates a simple single-expression function
    // Create a new CFG for this function
    ControlFlowGraph* funcCFG = m_programCFG->createFunctionCFG(stmt.functionName);
    
    // Store function metadata
    funcCFG->functionName = stmt.functionName;
    funcCFG->parameters = stmt.parameters;
    funcCFG->defStatement = &stmt;  // Store pointer to statement for codegen
    
    // Get return type and parameter types from semantic analyzer symbol table
    // The semantic analyzer has already inferred these types correctly
    const FunctionSymbol* funcSymbol = nullptr;
    if (m_symbols) {
        auto it = m_symbols->functions.find(stmt.functionName);
        if (it != m_symbols->functions.end()) {
            funcSymbol = &it->second;
        }
    }
    
    if (funcSymbol) {
        // Use types from semantic analyzer (already validated)
        funcCFG->returnType = funcSymbol->returnType;
        funcCFG->parameterTypes = funcSymbol->parameterTypes;
    } else {
        // Fallback if semantic analyzer didn't process this (shouldn't happen)
        funcCFG->returnType = inferTypeFromName(stmt.functionName);
        for (size_t i = 0; i < stmt.parameters.size(); ++i) {
            funcCFG->parameterTypes.push_back(inferTypeFromName(stmt.parameters[i]));
        }
    }
    
    // Save current CFG context
    ControlFlowGraph* savedCFG = m_currentCFG;
    BasicBlock* savedBlock = m_currentBlock;
    
    // Switch to function CFG
    m_currentCFG = funcCFG;
    
    // Create entry block for function - this will contain the RETURN expression
    BasicBlock* entryBlock = createNewBlock("DEF FN Entry");
    funcCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // DEF FN body is just a single expression - we'll handle it in codegen
    // Store the expression in a synthetic RETURN statement
    // (The codegen will need to access stmt.body directly)
    
    // Create exit block
    if (m_createExitBlock) {
        BasicBlock* exitBlock = createNewBlock("DEF FN Exit");
        exitBlock->isTerminator = true;
        funcCFG->exitBlock = exitBlock->id;
        
        // Entry flows to exit
        addFallthroughEdge(entryBlock->id, exitBlock->id);
    }
    
    // Build edges for this simple CFG
    ControlFlowGraph* edgeCFG = m_currentCFG;
    m_currentCFG = savedCFG;
    m_currentBlock = savedBlock;
    
    // We need to build edges for the DEF FN CFG
    ControlFlowGraph* tmpCFG = m_currentCFG;
    m_currentCFG = edgeCFG;
    buildEdges();
    m_currentCFG = tmpCFG;
    
    // Restore context
    m_currentCFG = savedCFG;
    m_currentBlock = savedBlock;
}

void CFGBuilder::processSubStatement(const SubStatement& stmt, BasicBlock* currentBlock) {
    // Create a new CFG for this SUB (similar to FUNCTION but no return value)
    ControlFlowGraph* subCFG = m_programCFG->createFunctionCFG(stmt.subName);
    
    // Store SUB metadata
    subCFG->functionName = stmt.subName;
    subCFG->parameters = stmt.parameters;
    
    // Process parameter types - check both AS types and type suffixes
    for (size_t i = 0; i < stmt.parameters.size(); i++) {
        VariableType vt = VariableType::DOUBLE;  // Default type
        
        // First check if there's an AS typename declaration
        if (i < stmt.parameterAsTypes.size() && !stmt.parameterAsTypes[i].empty()) {
            std::string asType = stmt.parameterAsTypes[i];
            // Convert to uppercase for case-insensitive comparison
            std::string upperType = asType;
            std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);
            
            if (upperType == "INTEGER" || upperType == "INT") {
                vt = VariableType::INT;
            } else if (upperType == "DOUBLE") {
                vt = VariableType::DOUBLE;
            } else if (upperType == "SINGLE" || upperType == "FLOAT") {
                vt = VariableType::FLOAT;
            } else if (upperType == "STRING") {
                vt = VariableType::STRING;
            } else if (upperType == "LONG") {
                vt = VariableType::INT;
            }
            // TODO: Handle user-defined types
        } else if (i < stmt.parameterTypes.size()) {
            // Check type suffix
            switch (stmt.parameterTypes[i]) {
                case TokenType::TYPE_INT: vt = VariableType::INT; break;
                case TokenType::TYPE_FLOAT: vt = VariableType::FLOAT; break;
                case TokenType::TYPE_DOUBLE: vt = VariableType::DOUBLE; break;
                case TokenType::TYPE_STRING: vt = VariableType::STRING; break;
                default: break;
            }
        }
        
        subCFG->parameterTypes.push_back(vt);
    }
    subCFG->returnType = VariableType::UNKNOWN; // SUBs don't return values
    
    // Save current CFG context
    ControlFlowGraph* savedCFG = m_currentCFG;
    BasicBlock* savedBlock = m_currentBlock;
    
    // Switch to SUB CFG
    m_currentCFG = subCFG;
    
    // Create entry block for SUB
    BasicBlock* entryBlock = createNewBlock("SUB Entry");
    subCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // Process SUB body statements
    for (const auto& bodyStmt : stmt.body) {
        if (bodyStmt) {
            processStatement(*bodyStmt, m_currentBlock, 0);
        }
    }
    
    // Create exit block
    if (m_createExitBlock) {
        BasicBlock* exitBlock = createNewBlock("SUB Exit");
        exitBlock->isTerminator = true;
        subCFG->exitBlock = exitBlock->id;
        
        if (m_currentBlock && m_currentBlock->id != exitBlock->id) {
            addFallthroughEdge(m_currentBlock->id, exitBlock->id);
        }
    }
    
    // Restore context
    m_currentCFG = savedCFG;
    m_currentBlock = savedBlock;
}

} // namespace FasterBASIC
