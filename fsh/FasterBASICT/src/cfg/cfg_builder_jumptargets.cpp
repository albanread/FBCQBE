//
// cfg_builder_jumptargets.cpp
// FasterBASIC - Control Flow Graph Builder Jump Target Collection
//
// Contains Phase 0: Pre-scan to collect all GOTO/GOSUB/ON GOTO/ON GOSUB targets.
// This identifies line numbers that need block boundaries (landing zones).
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

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
            
            case ASTNodeType::STMT_ON_GOTO: {
                const auto& onGotoStmt = static_cast<const OnGotoStatement&>(*stmt);
                for (size_t i = 0; i < onGotoStmt.isLabelList.size(); ++i) {
                    if (!onGotoStmt.isLabelList[i]) {
                        // Line number target
                        targets.insert(onGotoStmt.lineNumbers[i]);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_ON_GOSUB: {
                const auto& onGosubStmt = static_cast<const OnGosubStatement&>(*stmt);
                for (size_t i = 0; i < onGosubStmt.isLabelList.size(); ++i) {
                    if (!onGosubStmt.isLabelList[i]) {
                        // Line number target
                        targets.insert(onGosubStmt.lineNumbers[i]);
                    }
                    // For labels, we don't add to targets here since labels are collected separately
                }
                break;
            }
            
            case ASTNodeType::STMT_LABEL: {
                // Labels are potential jump targets - but we handle them separately
                // since they are collected in the semantic analyzer
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

} // namespace FasterBASIC