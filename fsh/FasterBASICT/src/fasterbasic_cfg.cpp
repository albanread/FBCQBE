//
// fasterbasic_cfg.cpp
// FasterBASIC - Control Flow Graph Builder Implementation
//
// Implements CFG construction from validated AST.
// Converts the tree structure into basic blocks connected by edges.
// This is Phase 4 of the compilation pipeline.
//

#include "fasterbasic_cfg.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Constructor/Destructor
// =============================================================================

CFGBuilder::CFGBuilder()
    : m_program(nullptr)
    , m_symbols(nullptr)
    , m_currentBlock(nullptr)
    , m_nextBlockId(0)
    , m_createExitBlock(true)
    , m_mergeBlocks(false)
    , m_blocksCreated(0)
    , m_edgesCreated(0)
{
}

CFGBuilder::~CFGBuilder() = default;

// =============================================================================
// Main Build Entry Point
// =============================================================================

std::unique_ptr<ProgramCFG> CFGBuilder::build(const Program& program,
                                               const SymbolTable& symbols) {
    m_program = &program;
    m_symbols = &symbols;
    m_programCFG = std::make_unique<ProgramCFG>();
    m_blocksCreated = 0;
    m_edgesCreated = 0;
    m_loopStack.clear();
    
    // Build main program CFG
    m_currentCFG = m_programCFG->mainCFG.get();
    
    // Phase 0: Pre-scan to collect jump targets (main program only)
    std::set<int> jumpTargets = collectJumpTargets(program);
    
    // Phase 1: Build basic blocks (main program + extract functions)
    buildBlocks(program, jumpTargets);
    
    // Phase 2: Build control flow edges for main program
    buildEdges();
    
    // Phase 3: Build edges for each function
    for (auto& pair : m_programCFG->functionCFGs) {
        m_currentCFG = pair.second.get();
        buildEdges();
    }
    
    // Phase 4: Identify loop structures in main
    m_currentCFG = m_programCFG->mainCFG.get();
    identifyLoops();
    
    // Phase 5: Identify loop structures in functions
    for (auto& pair : m_programCFG->functionCFGs) {
        m_currentCFG = pair.second.get();
        identifyLoops();
    }
    
    // Phase 6: Identify subroutines in main
    m_currentCFG = m_programCFG->mainCFG.get();
    identifySubroutines();
    
    // Phase 7: Optimize CFG (optional)
    if (m_mergeBlocks) {
        m_currentCFG = m_programCFG->mainCFG.get();
        optimizeCFG();
        for (auto& pair : m_programCFG->functionCFGs) {
            m_currentCFG = pair.second.get();
            optimizeCFG();
        }
    }
    
    return std::move(m_programCFG);
}

// =============================================================================
// Phase 0: Pre-scan to collect jump targets
// =============================================================================

// Helper function to recursively collect jump targets from statements
void collectJumpTargetsFromStatements(const std::vector<std::unique_ptr<Statement>>& statements, 
                                      std::set<int>& targets) {
    for (const auto& stmt : statements) {
        ASTNodeType type = stmt->getType();
        
        switch (type) {
            case ASTNodeType::STMT_GOTO: {
                const auto& gotoStmt = static_cast<const GotoStatement&>(*stmt);
                targets.insert(gotoStmt.lineNumber);
                break;
            }
            
            case ASTNodeType::STMT_GOSUB: {
                const auto& gosubStmt = static_cast<const GosubStatement&>(*stmt);
                targets.insert(gosubStmt.lineNumber);
                break;
            }
            
            case ASTNodeType::STMT_ON_EVENT: {
                const auto& onEventStmt = static_cast<const OnEventStatement&>(*stmt);
                std::cout << "DEBUG: Found ON_EVENT statement, event=" << onEventStmt.eventName 
                          << ", handlerType=" << static_cast<int>(onEventStmt.handlerType)
                          << ", target=" << onEventStmt.target 
                          << ", isLineNumber=" << onEventStmt.isLineNumber << std::endl;
                if ((onEventStmt.handlerType == EventHandlerType::GOSUB || 
                     onEventStmt.handlerType == EventHandlerType::GOTO) && 
                    onEventStmt.isLineNumber) {
                    int lineNum = std::stoi(onEventStmt.target);
                    std::cout << "DEBUG: Adding event GOSUB/GOTO target: " << lineNum << std::endl;
                    targets.insert(lineNum);
                }
                break;
            }
            
            case ASTNodeType::STMT_IF: {
                const auto& ifStmt = static_cast<const IfStatement&>(*stmt);
                if (ifStmt.hasGoto) {
                    targets.insert(ifStmt.gotoLine);
                }
                // Recursively scan THEN and ELSE blocks
                collectJumpTargetsFromStatements(ifStmt.thenStatements, targets);
                collectJumpTargetsFromStatements(ifStmt.elseStatements, targets);
                break;
            }
            
            default:
                break;
        }
    }
}

std::set<int> CFGBuilder::collectJumpTargets(const Program& program) {
    std::set<int> targets;
    
    for (const auto& line : program.lines) {
        collectJumpTargetsFromStatements(line->statements, targets);
    }
    
    return targets;
}

// =============================================================================
// Phase 1: Build Basic Blocks
// =============================================================================

void CFGBuilder::buildBlocks(const Program& program, const std::set<int>& jumpTargets) {
    // Create entry block for main program
    BasicBlock* entryBlock = createNewBlock("Entry");
    m_currentCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // Process each program line
    for (const auto& line : program.lines) {
        int lineNumber = line->lineNumber;
        
        // If this line is a jump target, start a new block
        if (lineNumber > 0 && jumpTargets.count(lineNumber) > 0) {
            // Only create new block if current block is not empty
            if (!m_currentBlock->statements.empty() || !m_currentBlock->lineNumbers.empty()) {
                BasicBlock* targetBlock = createNewBlock("Target_" + std::to_string(lineNumber));
                // Add fallthrough edge from previous block if it doesn't end with a jump
                if (!m_currentBlock->statements.empty()) {
                    const Statement* lastStmt = m_currentBlock->statements.back();
                    ASTNodeType lastType = lastStmt->getType();
                    if (lastType != ASTNodeType::STMT_GOTO && 
                        lastType != ASTNodeType::STMT_END &&
                        lastType != ASTNodeType::STMT_RETURN &&
                        lastType != ASTNodeType::STMT_EXIT) {
                        // Fallthrough will be added in buildEdges phase
                    }
                }
                m_currentBlock = targetBlock;
            }
        }
        
        // Map line number to current block
        if (lineNumber > 0) {
            m_currentCFG->mapLineToBlock(lineNumber, m_currentBlock->id);
            m_currentBlock->addLineNumber(lineNumber);
        }
        
        // Process each statement in the line
        for (const auto& stmt : line->statements) {
            processStatement(*stmt, m_currentBlock, lineNumber);
        }
    }
    
    // Create exit block if requested
    if (m_createExitBlock) {
        BasicBlock* exitBlock = createNewBlock("Exit");
        exitBlock->isTerminator = true;
        m_currentCFG->exitBlock = exitBlock->id;
        
        // Connect last block to exit
        if (m_currentBlock && m_currentBlock->id != exitBlock->id) {
            addFallthroughEdge(m_currentBlock->id, exitBlock->id);
        }
    }
}

// =============================================================================
// Statement Processing
// =============================================================================

void CFGBuilder::processStatement(const Statement& stmt, BasicBlock* currentBlock, int lineNumber) {
    // Handle control flow statements
    ASTNodeType type = stmt.getType();
    
    // Don't add FUNCTION/SUB/DEF statements to main CFG - they define separate CFGs
    if (type != ASTNodeType::STMT_FUNCTION && type != ASTNodeType::STMT_SUB && type != ASTNodeType::STMT_DEF) {
        // Add statement to current block with its line number
        currentBlock->addStatement(&stmt, lineNumber);
    }
    
    switch (type) {
        case ASTNodeType::STMT_GOTO:
            processGotoStatement(static_cast<const GotoStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_GOSUB:
            processGosubStatement(static_cast<const GosubStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_IF:
            processIfStatement(static_cast<const IfStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_FOR:
            processForStatement(static_cast<const ForStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_FOR_IN:
            processForInStatement(static_cast<const ForInStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_WHILE:
            processWhileStatement(static_cast<const WhileStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_REPEAT:
            processRepeatStatement(static_cast<const RepeatStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_DO:
            processDoStatement(static_cast<const DoStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_CASE:
            processCaseStatement(static_cast<const CaseStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_FUNCTION:
            processFunctionStatement(static_cast<const FunctionStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_SUB:
            processSubStatement(static_cast<const SubStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_DEF:
            processDefStatement(static_cast<const DefStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_NEXT:
            {
                // NEXT creates a back-edge to the loop header
                // Set current block to the loop's exit block (already created by FOR)
                // Find the matching loop context
                const NextStatement& nextStmt = static_cast<const NextStatement&>(stmt);
                LoopContext* matchingLoop = nullptr;
                
                // Find the matching loop by variable name (or innermost if no variable specified)
                for (auto it = m_loopStack.rbegin(); it != m_loopStack.rend(); ++it) {
                    if (nextStmt.variable.empty() || it->variable == nextStmt.variable) {
                        matchingLoop = &(*it);
                        break;
                    }
                }
                
                if (matchingLoop && matchingLoop->exitBlock >= 0) {
                    // Record the mapping from this NEXT's block to the loop header for buildEdges
                    m_nextToHeaderMap[currentBlock->id] = matchingLoop->headerBlock;
                    
                    // Use the exit block that was created by processForStatement
                    m_currentBlock = m_currentCFG->getBlock(matchingLoop->exitBlock);
                    
                    // Pop this loop context now - we're done with this loop
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [matchingLoop](const LoopContext& ctx) { return &ctx == matchingLoop; }), 
                        m_loopStack.end());
                } else {
                    // Fallback: create a new block if no matching loop found
                    BasicBlock* afterLoop = createNewBlock("After NEXT");
                    m_currentBlock = afterLoop;
                }
            }
            break;
            
        case ASTNodeType::STMT_WEND:
            // WEND ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After WHILE");
                m_currentBlock = nextBlock;
                
                // Don't pop loop context here - buildEdges() needs it to create back edge
            }
            break;
            
        case ASTNodeType::STMT_LOOP:
            // LOOP ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After DO");
                m_currentBlock = nextBlock;
                // Don't pop loop context - buildEdges() needs it
            }
            break;
            
        case ASTNodeType::STMT_UNTIL:
            // UNTIL ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After REPEAT");
                m_currentBlock = nextBlock;
                
                // Don't pop loop context here - buildEdges() needs it to create back edge
            }
            break;
            
        case ASTNodeType::STMT_RETURN:
        case ASTNodeType::STMT_END:
        case ASTNodeType::STMT_EXIT:
            currentBlock->isTerminator = true;
            break;
            
        default:
            // Regular statements don't affect control flow
            break;
    }
}

void CFGBuilder::processGotoStatement(const GotoStatement& stmt, BasicBlock* currentBlock) {
    // GOTO creates unconditional jump - start new block after this
    BasicBlock* nextBlock = createNewBlock();
    m_currentBlock = nextBlock;
    
    // Edge will be added in buildEdges phase when we know target block IDs
}

void CFGBuilder::processGosubStatement(const GosubStatement& stmt, BasicBlock* currentBlock) {
    // GOSUB is like a call - execution continues after it
    // Edge will be added in buildEdges phase
}

void CFGBuilder::processIfStatement(const IfStatement& stmt, BasicBlock* currentBlock) {
    // IF creates conditional branch
    
    if (stmt.hasGoto) {
        // IF ... THEN GOTO creates two-way branch
        BasicBlock* nextBlock = createNewBlock();
        m_currentBlock = nextBlock;
    } else if (!stmt.thenStatements.empty() || !stmt.elseIfClauses.empty() || !stmt.elseStatements.empty()) {
        // IF with structured statements - handled by structured IR opcodes
        // Don't create separate blocks; the IR generator will emit structured IF opcodes
        // Just continue with the current block
    }
}

void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    // FOR creates: init block (with FOR statement), check block, body block, exit block
    // Structure: FOR init → check (condition) → body → NEXT (increment) → check
    
    // Init block contains the FOR statement (initialization)
    BasicBlock* initBlock = createNewBlock("FOR Init");
    
    // Create edge from current block to init block (for nested loops)
    // This ensures the outer loop body flows into the inner loop init
    if (currentBlock->id != initBlock->id) {
        addFallthroughEdge(currentBlock->id, initBlock->id);
    }
    
    // Move the FOR statement to the init block (it was already added to currentBlock)
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        // Get the line number for this statement
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        // Remove from current block and add to init
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        initBlock->addStatement(&stmt, lineNum);
    }
    
    // Check block evaluates the loop condition (var <= end for positive STEP)
    BasicBlock* loopCheck = createNewBlock("FOR Loop Check");
    loopCheck->isLoopHeader = true;  // The check block is the actual loop header
    
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    BasicBlock* loopExit = createNewBlock("After FOR");
    loopExit->isLoopExit = true;
    
    // Track loop context - stores check block as header (for NEXT to jump back to)
    LoopContext ctx;
    ctx.headerBlock = loopCheck->id;  // NEXT jumps back to check block
    ctx.exitBlock = loopExit->id;
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // Store FOR loop structure for buildEdges to use
    ControlFlowGraph::ForLoopBlocks forBlocks;
    forBlocks.initBlock = initBlock->id;
    forBlocks.checkBlock = loopCheck->id;
    forBlocks.bodyBlock = loopBody->id;
    forBlocks.exitBlock = loopExit->id;
    forBlocks.variable = stmt.variable;
    m_currentCFG->forLoopStructure[initBlock->id] = forBlocks;
    
    // Keep legacy mapping for backwards compatibility
    m_currentCFG->forLoopHeaders[initBlock->id] = loopCheck->id;
    m_currentCFG->forLoopHeaders[loopCheck->id] = loopBody->id;
    
    // Continue building in the loop body
    m_currentBlock = loopBody;
}

void CFGBuilder::processForInStatement(const ForInStatement& stmt, BasicBlock* currentBlock) {
    // FOR...IN creates loop header similar to FOR
    BasicBlock* loopHeader = createNewBlock("FOR...IN Loop Header");
    loopHeader->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("FOR...IN Loop Body");
    BasicBlock* loopExit = createNewBlock("After FOR...IN");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // Remember this FOR...IN loop
    m_currentCFG->forLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;
}

void CFGBuilder::processWhileStatement(const WhileStatement& stmt, BasicBlock* currentBlock) {
    // WHILE creates loop header with condition
    BasicBlock* loopHeader = createNewBlock("WHILE Loop Header");
    loopHeader->isLoopHeader = true;
    
    // Add the WHILE statement to the header block (it was already added to currentBlock)
    // We need to move it to the header block
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        // Get the line number for this statement
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        // Remove from current block and add to header
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        loopHeader->addStatement(&stmt, lineNum);
    }
    
    BasicBlock* loopBody = createNewBlock("WHILE Loop Body");
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = -1;  // Will be set when we encounter WEND
    m_loopStack.push_back(ctx);
    
    m_currentCFG->whileLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;
}

void CFGBuilder::processCaseStatement(const CaseStatement& stmt, BasicBlock* currentBlock) {
    // SELECT CASE creates a multi-way branch structure
    // Structure:
    //   - SELECT block (current): Evaluates the SELECT CASE expression
    //   - Test blocks: One per CASE clause, contains comparison logic
    //   - Body blocks: One per CASE clause, executes CASE statements
    //   - ELSE block: Optional, for ELSE clause
    //   - Exit block: Continue after END SELECT
    
    // The SELECT statement stays in current block for expression evaluation
    
    // Create exit block
    BasicBlock* exitBlock = createNewBlock("After SELECT CASE");
    
    // For each CASE clause, create test block and body block
    std::vector<int> testBlockIds;
    std::vector<int> bodyBlockIds;
    
    for (size_t i = 0; i < stmt.whenClauses.size(); i++) {
        // Test block will contain the comparison logic
        BasicBlock* testBlock = createNewBlock("CASE " + std::to_string(i) + " Test");
        testBlockIds.push_back(testBlock->id);
        
        // Body block will contain the CASE statements
        BasicBlock* bodyBlock = createNewBlock("CASE " + std::to_string(i) + " Body");
        bodyBlockIds.push_back(bodyBlock->id);
        
        // Process statements in the body block
        m_currentBlock = bodyBlock;
        for (const auto& caseStmt : stmt.whenClauses[i].statements) {
            if (caseStmt) {
                processStatement(*caseStmt, bodyBlock, 0);
            }
        }
    }
    
    // Create ELSE block if there are OTHERWISE statements
    int elseBlockId = -1;
    if (!stmt.otherwiseStatements.empty()) {
        BasicBlock* elseBlock = createNewBlock("CASE ELSE");
        elseBlockId = elseBlock->id;
        
        m_currentBlock = elseBlock;
        for (const auto& elseStmt : stmt.otherwiseStatements) {
            if (elseStmt) {
                processStatement(*elseStmt, elseBlock, 0);
            }
        }
    }
    
    // Store SELECT CASE info for buildEdges phase
    SelectCaseContext ctx;
    ctx.selectBlock = currentBlock->id;
    ctx.testBlocks = testBlockIds;
    ctx.bodyBlocks = bodyBlockIds;
    ctx.elseBlock = elseBlockId;
    ctx.exitBlock = exitBlock->id;
    ctx.caseStatement = &stmt;
    m_selectCaseStack.push_back(ctx);
    
    // Continue with exit block
    m_currentBlock = exitBlock;
}

void CFGBuilder::processRepeatStatement(const RepeatStatement& stmt, BasicBlock* currentBlock) {
    // REPEAT creates loop body
    BasicBlock* loopBody = createNewBlock("REPEAT Loop Body");
    loopBody->isLoopHeader = true;
    
    BasicBlock* loopExit = createNewBlock("After REPEAT");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopBody->id;
    ctx.exitBlock = loopExit->id;
    m_loopStack.push_back(ctx);
    
    m_currentCFG->repeatLoopHeaders[loopBody->id] = loopBody->id;
    
    m_currentBlock = loopBody;
}

void CFGBuilder::processDoStatement(const DoStatement& stmt, BasicBlock* currentBlock) {
    // DO creates loop structure - behavior depends on condition type
    BasicBlock* loopHeader = createNewBlock("DO Loop Header");
    loopHeader->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("DO Loop Body");
    BasicBlock* loopExit = createNewBlock("After DO");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    m_loopStack.push_back(ctx);
    
    m_currentCFG->doLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;
}

void CFGBuilder::processFunctionStatement(const FunctionStatement& stmt, BasicBlock* currentBlock) {
    // Create a new CFG for this function
    ControlFlowGraph* funcCFG = m_programCFG->createFunctionCFG(stmt.functionName);
    
    // Store function metadata
    funcCFG->functionName = stmt.functionName;
    funcCFG->parameters = stmt.parameters;
    for (size_t i = 0; i < stmt.parameterTypes.size(); i++) {
        // Convert TokenType to VariableType
        VariableType vt = VariableType::UNKNOWN;
        switch (stmt.parameterTypes[i]) {
            case TokenType::TYPE_INT: vt = VariableType::INT; break;
            case TokenType::TYPE_FLOAT: vt = VariableType::FLOAT; break;
            case TokenType::TYPE_DOUBLE: vt = VariableType::DOUBLE; break;
            case TokenType::TYPE_STRING: vt = VariableType::STRING; break;
            default: break;
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
    
    // Infer return type from function name
    funcCFG->returnType = inferTypeFromName(stmt.functionName);
    
    // For DEF FN, parameter types are inferred from parameter names
    for (const auto& param : stmt.parameters) {
        funcCFG->parameterTypes.push_back(inferTypeFromName(param));
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
    for (size_t i = 0; i < stmt.parameterTypes.size(); i++) {
        // Convert TokenType to VariableType
        VariableType vt = VariableType::UNKNOWN;
        switch (stmt.parameterTypes[i]) {
            case TokenType::TYPE_INT: vt = VariableType::INT; break;
            case TokenType::TYPE_FLOAT: vt = VariableType::FLOAT; break;
            case TokenType::TYPE_DOUBLE: vt = VariableType::DOUBLE; break;
            case TokenType::TYPE_STRING: vt = VariableType::STRING; break;
            default: break;
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

// =============================================================================
// Phase 2: Build Control Flow Edges
// =============================================================================

void CFGBuilder::buildEdges() {
    // Walk through blocks and create edges based on statements
    for (const auto& block : m_currentCFG->blocks) {
        // Check if this is a FOR loop init block
        auto forStructIt = m_currentCFG->forLoopStructure.find(block->id);
        if (forStructIt != m_currentCFG->forLoopStructure.end()) {
            const auto& forBlocks = forStructIt->second;
            
            // FOR init block: unconditional jump to check block
            addUnconditionalEdge(block->id, forBlocks.checkBlock);
            
            // Also need to ensure predecessor blocks connect to this init block
            // This handles nested FOR loops where the outer body should flow to inner init
            if (block->id > 0) {
                const auto& prevBlock = m_currentCFG->blocks[block->id - 1];
                // If previous block doesn't already have this block as a successor, add fallthrough
                if (std::find(prevBlock->successors.begin(), prevBlock->successors.end(), block->id) == prevBlock->successors.end()) {
                    // Check if previous block is a body block that should flow here
                    bool isPreviousBodyBlock = false;
                    for (const auto& pair : m_currentCFG->forLoopStructure) {
                        if (pair.second.bodyBlock == prevBlock->id) {
                            isPreviousBodyBlock = true;
                            break;
                        }
                    }
                    
                    // If it's not already handled and is not a terminator block, add fallthrough
                    if (!prevBlock->isTerminator && !prevBlock->statements.empty()) {
                        const Statement* lastStmt = prevBlock->statements.back();
                        ASTNodeType lastType = lastStmt->getType();
                        // If the last statement isn't a control flow statement, add fallthrough
                        if (lastType != ASTNodeType::STMT_GOTO && 
                            lastType != ASTNodeType::STMT_RETURN &&
                            lastType != ASTNodeType::STMT_END &&
                            lastType != ASTNodeType::STMT_EXIT &&
                            lastType != ASTNodeType::STMT_NEXT &&
                            lastType != ASTNodeType::STMT_WEND &&
                            lastType != ASTNodeType::STMT_LOOP &&
                            lastType != ASTNodeType::STMT_UNTIL) {
                            addFallthroughEdge(prevBlock->id, block->id);
                        }
                    }
                }
            }
            
            continue;  // Skip regular processing for FOR init blocks
        }
        
        // Check if this is a FOR loop check block
        bool isForCheckBlock = false;
        for (const auto& pair : m_currentCFG->forLoopStructure) {
            if (pair.second.checkBlock == block->id) {
                isForCheckBlock = true;
                const auto& forBlocks = pair.second;
                
                // FOR check block: conditional branch
                // True condition: go to body
                addConditionalEdge(block->id, forBlocks.bodyBlock, "true");
                // False condition: go to exit
                addConditionalEdge(block->id, forBlocks.exitBlock, "false");
                break;
            }
        }
        if (isForCheckBlock) {
            continue;  // Skip regular processing for FOR check blocks
        }
        
        // Check if this is a SELECT CASE test block (empty but needs special handling)
        bool isSelectCaseTestBlock = false;
        for (const auto& ctx : m_selectCaseStack) {
            for (size_t i = 0; i < ctx.testBlocks.size(); i++) {
                if (block->id == ctx.testBlocks[i]) {
                    isSelectCaseTestBlock = true;
                    // Test block: conditional branch to body or next test/else/exit
                    // True: jump to body
                    addConditionalEdge(block->id, ctx.bodyBlocks[i], "true");
                    
                    // False: jump to next test, else, or exit
                    if (i + 1 < ctx.testBlocks.size()) {
                        // Next test block
                        addConditionalEdge(block->id, ctx.testBlocks[i + 1], "false");
                    } else if (ctx.elseBlock >= 0) {
                        // ELSE block
                        addConditionalEdge(block->id, ctx.elseBlock, "false");
                    } else {
                        // Exit (no match)
                        addConditionalEdge(block->id, ctx.exitBlock, "false");
                    }
                    break;
                }
            }
            if (isSelectCaseTestBlock) break;
        }
        
        if (isSelectCaseTestBlock) {
            continue;  // Skip regular processing for test blocks
        }
        
        if (block->statements.empty()) {
            // Empty block - fallthrough to next
            if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                addFallthroughEdge(block->id, block->id + 1);
            }
            continue;
        }
        
        // Check last statement in block for control flow
        const Statement* lastStmt = block->statements.back();
        ASTNodeType type = lastStmt->getType();
        
        switch (type) {
            case ASTNodeType::STMT_GOTO: {
                // Unconditional jump to target line (or next available line)
                const auto& gotoStmt = static_cast<const GotoStatement&>(*lastStmt);
                int targetBlock = m_currentCFG->getBlockForLineOrNext(gotoStmt.lineNumber);
                if (targetBlock >= 0) {
                    addUnconditionalEdge(block->id, targetBlock);
                }
                break;
            }
            
            case ASTNodeType::STMT_GOSUB: {
                // Call to subroutine (or next available line), then continue
                const auto& gosubStmt = static_cast<const GosubStatement&>(*lastStmt);
                int targetBlock = m_currentCFG->getBlockForLineOrNext(gosubStmt.lineNumber);
                if (targetBlock >= 0) {
                    addCallEdge(block->id, targetBlock);
                }
                // Also continue to next block
                if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                    addFallthroughEdge(block->id, block->id + 1);
                }
                break;
            }
            
            case ASTNodeType::STMT_IF: {
                // Conditional branch
                const auto& ifStmt = static_cast<const IfStatement&>(*lastStmt);
                if (ifStmt.hasGoto) {
                    // Branch to line (or next available line) or continue
                    int targetBlock = m_currentCFG->getBlockForLineOrNext(ifStmt.gotoLine);
                    if (targetBlock >= 0) {
                        addConditionalEdge(block->id, targetBlock, "true");
                    }
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "false");
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_WHILE: {
                // WHILE header with condition - needs conditional edges
                // Body should be the next block, exit will be found later
                auto it = m_currentCFG->whileLoopHeaders.find(block->id);
                if (it != m_currentCFG->whileLoopHeaders.end()) {
                    // This is a WHILE header
                    // True condition: go to body (next block)
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "true");
                    }
                    
                    // False condition: We need to find the exit block
                    // The exit block should be right after the matching WEND
                    // We need to count nesting levels to find the matching WEND
                    int nestingLevel = 0;
                    for (size_t i = block->id + 1; i < m_currentCFG->blocks.size(); i++) {
                        const auto& futureBlock = m_currentCFG->blocks[i];
                        if (!futureBlock->statements.empty()) {
                            for (const Statement* stmt : futureBlock->statements) {
                                if (stmt->getType() == ASTNodeType::STMT_WHILE) {
                                    nestingLevel++;
                                } else if (stmt->getType() == ASTNodeType::STMT_WEND) {
                                    if (nestingLevel == 0) {
                                        // Found the matching WEND
                                        // Exit is the block after WEND
                                        if (i + 1 < m_currentCFG->blocks.size()) {
                                            addConditionalEdge(block->id, i + 1, "false");
                                        }
                                        goto found_wend;
                                    }
                                    nestingLevel--;
                                }
                            }
                        }
                    }
                    found_wend:;
                } else {
                    // Fallthrough if not a loop header
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_UNTIL: {
                // UNTIL is the end of a REPEAT loop
                // Need to create:
                // 1. Conditional edge to exit block (next block) when condition is TRUE
                // 2. Conditional edge back to loop header (REPEAT block) when condition is FALSE
                
                // Find the matching REPEAT by looking for loop context with this block in its range
                LoopContext* loopCtx = nullptr;
                for (auto& ctx : m_loopStack) {
                    // The UNTIL block can be the same as or after the header block
                    if (block->id >= ctx.headerBlock) {
                        loopCtx = &ctx;
                        break;
                    }
                }
                
                if (loopCtx) {
                    // When condition is TRUE, exit loop (go to next block)
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "true");
                    }
                    // When condition is FALSE, repeat (go back to loop header)
                    addConditionalEdge(block->id, loopCtx->headerBlock, "false");
                    
                    // Pop this loop context now that we've handled the UNTIL
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [loopCtx](const LoopContext& ctx) { return &ctx == loopCtx; }), 
                        m_loopStack.end());
                } else {
                    // UNTIL without REPEAT - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_LOOP: {
                // LOOP is the end of a DO loop
                // Need to create:
                // 1. Conditional edge to exit block (next block) based on condition
                // 2. Conditional or unconditional edge back to loop header
                
                // Find the matching DO by looking for loop context
                LoopContext* loopCtx = nullptr;
                for (auto& ctx : m_loopStack) {
                    // The LOOP block can be the same as or after the header block
                    if (block->id >= ctx.headerBlock) {
                        loopCtx = &ctx;
                        break;
                    }
                }
                
                if (loopCtx) {
                    // Get the LOOP statement to check condition type
                    const auto& loopStmt = static_cast<const LoopStatement&>(*lastStmt);
                    
                    if (loopStmt.conditionType == LoopStatement::ConditionType::NONE) {
                        // Plain LOOP - unconditional back edge
                        addUnconditionalEdge(block->id, loopCtx->headerBlock);
                    } else {
                        // LOOP WHILE/UNTIL - conditional edges
                        // When condition is TRUE, exit loop (go to next block)
                        if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                            addConditionalEdge(block->id, block->id + 1, "true");
                        }
                        // When condition is FALSE, repeat (go back to loop header)
                        addConditionalEdge(block->id, loopCtx->headerBlock, "false");
                    }
                    
                    // Pop this loop context
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [loopCtx](const LoopContext& ctx) { return &ctx == loopCtx; }), 
                        m_loopStack.end());
                } else {
                    // LOOP without DO - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_WEND: {
                // WEND is the end of a WHILE loop
                // Need to create:
                // 1. Unconditional back edge to loop header (WHILE condition block)
                
                // Find the matching WHILE by looking for loop context
                // Search backwards to find the innermost (most recent) loop
                LoopContext* loopCtx = nullptr;
                for (auto it = m_loopStack.rbegin(); it != m_loopStack.rend(); ++it) {
                    // The WEND block can be the same as or after the header block
                    if (block->id >= it->headerBlock) {
                        loopCtx = &(*it);
                        break;
                    }
                }
                
                if (loopCtx) {
                    // Unconditional back edge to WHILE header (condition check)
                    addUnconditionalEdge(block->id, loopCtx->headerBlock);
                    
                    // Pop this loop context
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [loopCtx](const LoopContext& ctx) { return &ctx == loopCtx; }), 
                        m_loopStack.end());
                } else {
                    // WEND without WHILE - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_NEXT: {
                // NEXT is the end of a FOR loop
                // Create unconditional back edge to loop check block
                // Use the mapping recorded in processStatement
                
                auto it = m_nextToHeaderMap.find(block->id);
                if (it != m_nextToHeaderMap.end()) {
                    // Found the target header block - create back edge
                    addUnconditionalEdge(block->id, it->second);
                } else {
                    // NEXT without matching FOR - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_CASE: {
                // SELECT CASE - multi-way branch
                // Find the matching SELECT CASE context
                SelectCaseContext* caseCtx = nullptr;
                for (auto& ctx : m_selectCaseStack) {
                    if (ctx.selectBlock == block->id) {
                        caseCtx = &ctx;
                        break;
                    }
                }
                
                if (caseCtx && !caseCtx->testBlocks.empty()) {
                    // From SELECT block, jump to first test block
                    addUnconditionalEdge(block->id, caseCtx->testBlocks[0]);
                }
                break;
            }
            
            case ASTNodeType::STMT_RETURN:
            case ASTNodeType::STMT_END:
            case ASTNodeType::STMT_EXIT:
                // Terminators - no outgoing edges (or return edge)
                if (m_currentCFG->exitBlock >= 0) {
                    addReturnEdge(block->id, m_currentCFG->exitBlock);
                }
                break;
                
            default:
                // Check if this block is part of a SELECT CASE structure
                bool handledBySelectCase = false;
                
                for (const auto& ctx : m_selectCaseStack) {
                    // Check if this is a body block
                    for (size_t i = 0; i < ctx.bodyBlocks.size(); i++) {
                        if (block->id == ctx.bodyBlocks[i]) {
                            // Body block: jump to exit after executing
                            addUnconditionalEdge(block->id, ctx.exitBlock);
                            handledBySelectCase = true;
                            break;
                        }
                    }
                    
                    // Check if this is the else block
                    if (ctx.elseBlock >= 0 && block->id == ctx.elseBlock) {
                        // Else block: jump to exit after executing
                        addUnconditionalEdge(block->id, ctx.exitBlock);
                        handledBySelectCase = true;
                    }
                    
                    if (handledBySelectCase) break;
                }
                
                if (!handledBySelectCase) {
                    // Regular statement - fallthrough to next block
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
        }
    }
}

// =============================================================================
// Phase 3: Identify Loop Structures
// =============================================================================

void CFGBuilder::identifyLoops() {
    // Implement back-edge detection to identify GOTO-based loops
    // A back edge is an edge from block A to block B where B dominates A
    // or in simpler terms, B appears earlier in program order
    
    // For each edge, check if it's a back edge (target block has lower ID than source)
    for (const auto& edge : m_currentCFG->edges) {
        if (edge.type == EdgeType::UNCONDITIONAL && 
            edge.targetBlock < edge.sourceBlock) {
            // This is likely a back edge (GOTO to earlier line)
            BasicBlock* targetBlock = m_currentCFG->getBlock(edge.targetBlock);
            BasicBlock* sourceBlock = m_currentCFG->getBlock(edge.sourceBlock);
            
            if (targetBlock && sourceBlock) {
                // Mark the target as a loop header
                targetBlock->isLoopHeader = true;
                
                // Mark blocks in the loop body between target and source
                for (int blockId = edge.targetBlock; blockId <= edge.sourceBlock; blockId++) {
                    BasicBlock* loopBlock = m_currentCFG->getBlock(blockId);
                    if (loopBlock) {
                        // This block is part of a potential loop
                        // We'll use this information during code generation
                    }
                }
            }
        }
    }
    
    // Also detect cycles using simple DFS
    std::set<int> visited;
    std::set<int> recursionStack;
    
    std::function<void(int)> detectCycles = [&](int blockId) {
        if (recursionStack.count(blockId)) {
            // Found a cycle - mark the target block as a loop header
            BasicBlock* loopHeader = m_currentCFG->getBlock(blockId);
            if (loopHeader) {
                loopHeader->isLoopHeader = true;
            }
            return;
        }
        
        if (visited.count(blockId)) {
            return;
        }
        
        visited.insert(blockId);
        recursionStack.insert(blockId);
        
        BasicBlock* block = m_currentCFG->getBlock(blockId);
        if (block) {
            for (int successor : block->successors) {
                detectCycles(successor);
            }
        }
        
        recursionStack.erase(blockId);
    };
    
    // Start cycle detection from entry block
    if (m_currentCFG->entryBlock >= 0) {
        detectCycles(m_currentCFG->entryBlock);
    }
}

// =============================================================================
// Phase 4: Identify Subroutines
// =============================================================================

void CFGBuilder::identifySubroutines() {
    // Mark blocks that are GOSUB targets as subroutines
    for (const auto& edge : m_currentCFG->edges) {
        if (edge.type == EdgeType::CALL) {
            BasicBlock* target = m_currentCFG->getBlock(edge.targetBlock);
            if (target) {
                target->isSubroutine = true;
            }
        }
    }
}

// =============================================================================
// Phase 5: Optimize CFG
// =============================================================================

void CFGBuilder::optimizeCFG() {
    // Potential optimizations:
    // - Merge sequential blocks with single predecessor/successor
    // - Remove empty blocks
    // - Simplify edges
    // Not implemented yet
}

// =============================================================================
// Block Management
// =============================================================================

BasicBlock* CFGBuilder::createNewBlock(const std::string& label) {
    auto* block = m_currentCFG->createBlock(label);
    m_blocksCreated++;
    return block;
}

void CFGBuilder::finalizeBlock(BasicBlock* block) {
    // Any finalization needed for a block
}

// =============================================================================
// Edge Creation Helpers
// =============================================================================

void CFGBuilder::addFallthroughEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::FALLTHROUGH);
    m_edgesCreated++;
}

void CFGBuilder::addConditionalEdge(int source, int target, const std::string& label) {
    m_currentCFG->addEdge(source, target, EdgeType::CONDITIONAL, label);
    m_edgesCreated++;
}

void CFGBuilder::addUnconditionalEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::UNCONDITIONAL);
    m_edgesCreated++;
}

void CFGBuilder::addCallEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::CALL);
    m_edgesCreated++;
}

void CFGBuilder::addReturnEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::RETURN);
    m_edgesCreated++;
}

// =============================================================================
// Report Generation
// =============================================================================

std::string CFGBuilder::generateReport(const ControlFlowGraph& cfg) const {
    std::ostringstream oss;
    
    oss << "=== CFG BUILDER REPORT ===\n\n";
    
    // Build statistics
    oss << "Build Statistics:\n";
    oss << "  Blocks Created: " << m_blocksCreated << "\n";
    oss << "  Edges Created: " << m_edgesCreated << "\n";
    oss << "  Loop Headers: " << cfg.getLoopCount() << "\n";
    oss << "\n";
    
    // CFG summary
    oss << "CFG Summary:\n";
    oss << "  Total Blocks: " << cfg.getBlockCount() << "\n";
    oss << "  Total Edges: " << cfg.getEdgeCount() << "\n";
    oss << "  Entry Block: " << cfg.entryBlock << "\n";
    oss << "  Exit Block: " << cfg.exitBlock << "\n";
    oss << "\n";
    
    // Block types
    int loopHeaders = 0;
    int loopExits = 0;
    int subroutines = 0;
    int terminators = 0;
    
    for (const auto& block : cfg.blocks) {
        if (block->isLoopHeader) loopHeaders++;
        if (block->isLoopExit) loopExits++;
        if (block->isSubroutine) subroutines++;
        if (block->isTerminator) terminators++;
    }
    
    oss << "Block Analysis:\n";
    oss << "  Loop Headers: " << loopHeaders << "\n";
    oss << "  Loop Exits: " << loopExits << "\n";
    oss << "  Subroutines: " << subroutines << "\n";
    oss << "  Terminators: " << terminators << "\n";
    oss << "\n";
    
    // Edge types
    int fallthroughEdges = 0;
    int conditionalEdges = 0;
    int unconditionalEdges = 0;
    int callEdges = 0;
    int returnEdges = 0;
    
    for (const auto& edge : cfg.edges) {
        switch (edge.type) {
            case EdgeType::FALLTHROUGH: fallthroughEdges++; break;
            case EdgeType::CONDITIONAL: conditionalEdges++; break;
            case EdgeType::UNCONDITIONAL: unconditionalEdges++; break;
            case EdgeType::CALL: callEdges++; break;
            case EdgeType::RETURN: returnEdges++; break;
        }
    }
    
    oss << "Edge Analysis:\n";
    oss << "  Fallthrough: " << fallthroughEdges << "\n";
    oss << "  Conditional: " << conditionalEdges << "\n";
    oss << "  Unconditional: " << unconditionalEdges << "\n";
    oss << "  Call: " << callEdges << "\n";
    oss << "  Return: " << returnEdges << "\n";
    oss << "\n";
    
    // Full CFG details
    oss << cfg.toString();
    
    oss << "=== END CFG BUILDER REPORT ===\n";
    
    return oss.str();
}

// Helper function to infer type from variable name (suffix-based)
VariableType CFGBuilder::inferTypeFromName(const std::string& name) {
    if (name.empty()) return VariableType::FLOAT;
    
    char lastChar = name.back();
    switch (lastChar) {
        case '%': return VariableType::INT;
        case '!': return VariableType::FLOAT;
        case '#': return VariableType::DOUBLE;
        case '$': return VariableType::STRING;
        default: return VariableType::FLOAT;  // Default in BASIC
    }
}

} // namespace FasterBASIC