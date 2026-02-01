#ifndef AST_EMITTER_H
#define AST_EMITTER_H

#include <string>
#include <unordered_map>
#include "../fasterbasic_ast.h"
#include "../fasterbasic_semantic.h"
#include "qbe_builder.h"
#include "type_manager.h"
#include "symbol_mapper.h"
#include "runtime_library.h"

namespace fbc {

/**
 * ASTEmitter - Statement and expression code emission
 * 
 * Responsible for:
 * - Emitting code for expressions (binary ops, function calls, literals, etc.)
 * - Emitting code for statements (LET, PRINT, IF, FOR, etc.)
 * - Type checking and conversion
 * - Variable and array access
 * 
 * Works with QBEBuilder for low-level IL emission and RuntimeLibrary
 * for runtime function calls.
 */
class ASTEmitter {
public:
    ASTEmitter(QBEBuilder& builder, TypeManager& typeManager, 
               SymbolMapper& symbolMapper, RuntimeLibrary& runtime,
               FasterBASIC::SemanticAnalyzer& semantic);
    ~ASTEmitter() = default;

    // === Expression Emission ===
    
    /**
     * Emit code for an expression
     * @param expr Expression to emit
     * @return Temporary holding the result value
     */
    std::string emitExpression(const FasterBASIC::Expression* expr);
    
    /**
     * Emit code for an expression with expected type (auto-converts)
     * @param expr Expression to emit
     * @param expectedType Expected result type
     * @return Temporary holding the result (converted to expectedType)
     */
    std::string emitExpressionAs(const FasterBASIC::Expression* expr, 
                                  FasterBASIC::BaseType expectedType);

    // === Statement Emission ===
    
    /**
     * Emit code for a statement
     * @param stmt Statement to emit
     */
    void emitStatement(const FasterBASIC::Statement* stmt);
    
    /**
     * Emit LET assignment
     * @param stmt LET statement
     */
    void emitLetStatement(const FasterBASIC::LetStatement* stmt);
    
    /**
     * Emit PRINT statement
     * @param stmt PRINT statement
     */
    void emitPrintStatement(const FasterBASIC::PrintStatement* stmt);
    
    /**
     * Emit INPUT statement
     * @param stmt INPUT statement
     */
    void emitInputStatement(const FasterBASIC::InputStatement* stmt);
    
    /**
     * Emit READ statement
     * @param stmt READ statement
     */
    void emitReadStatement(const FasterBASIC::ReadStatement* stmt);
    
    /**
     * Emit RESTORE statement
     * @param stmt RESTORE statement
     */
    void emitRestoreStatement(const FasterBASIC::RestoreStatement* stmt);
    
    /**
     * Emit IF statement (handled by CFGEmitter for control flow)
     * This just emits the condition evaluation
     * @param stmt IF statement
     * @return Temporary holding condition result
     */
    std::string emitIfCondition(const FasterBASIC::IfStatement* stmt);
    
    /**
     * Emit WHILE loop condition check
     * @param stmt WHILE statement
     * @return Temporary holding condition result
     */
    std::string emitWhileCondition(const FasterBASIC::WhileStatement* stmt);
    
    /**
     * Emit FOR loop initialization
     * @param stmt FOR statement
     */
    void emitForInit(const FasterBASIC::ForStatement* stmt);
    
    /**
     * Emit FOR loop condition check
     * @param stmt FOR statement
     * @return Temporary holding condition result (loop variable <= end value)
     */
    std::string emitForCondition(const FasterBASIC::ForStatement* stmt);
    
    /**
     * Emit FOR loop increment
     * @param stmt FOR statement
     */
    void emitForIncrement(const FasterBASIC::ForStatement* stmt);
    
    /**
     * Emit END statement
     * @param stmt END statement
     */
    void emitEndStatement(const FasterBASIC::EndStatement* stmt);
    
    /**
     * Emit DIM statement (array declaration)
     * @param stmt DIM statement
     */
    void emitDimStatement(const FasterBASIC::DimStatement* stmt);
    
    /**
     * Emit CALL statement (SUB call)
     * @param stmt CALL statement
     */
    void emitCallStatement(const FasterBASIC::CallStatement* stmt);

    // === Variable Access ===
    
    /**
     * Get the address of a variable (for assignments)
     * @param varName Variable name
     * @return Temporary holding variable address
     */
    std::string getVariableAddress(const std::string& varName);
    
    /**
     * Load a variable value
     * @param varName Variable name
     * @return Temporary holding variable value
     */
    std::string loadVariable(const std::string& varName);
    
    /**
     * Store a value to a variable
     * @param varName Variable name
     * @param value Value to store (temporary)
     */
    void storeVariable(const std::string& varName, const std::string& value);

    // === Array Access ===
    
    /**
     * Emit array element access
     * @param arrayName Array name
     * @param indices Index expressions
     * @return Temporary holding element address
     */
    std::string emitArrayAccess(const std::string& arrayName,
                                const std::vector<FasterBASIC::ExpressionPtr>& indices);
    
    /**
     * Load array element value
     * @param arrayName Array name
     * @param indices Index expressions
     * @return Temporary holding element value
     */
    std::string loadArrayElement(const std::string& arrayName,
                                 const std::vector<FasterBASIC::ExpressionPtr>& indices);
    
    /**
     * Store value to array element
     * @param arrayName Array name
     * @param indices Index expressions
     * @param value Value to store
     */
    void storeArrayElement(const std::string& arrayName,
                          const std::vector<FasterBASIC::ExpressionPtr>& indices,
                          const std::string& value);

    // === Type Inference ===
    
    /**
     * Get the type of an expression
     * @param expr Expression
     * @return Type of the expression result
     */
    FasterBASIC::BaseType getExpressionType(const FasterBASIC::Expression* expr);
    
    /**
     * Get the type of a variable
     * @param varName Variable name
     * @return Variable type
     */
    FasterBASIC::BaseType getVariableType(const std::string& varName);

private:
    QBEBuilder& builder_;
    TypeManager& typeManager_;
    SymbolMapper& symbolMapper_;
    RuntimeLibrary& runtime_;
    FasterBASIC::SemanticAnalyzer& semantic_;
    
    // Global variable addresses cache
    std::unordered_map<std::string, std::string> globalVarAddresses_;
    
    // === Expression Emitters (by type) ===
    
    std::string emitBinaryExpression(const FasterBASIC::BinaryExpression* expr);
    std::string emitUnaryExpression(const FasterBASIC::UnaryExpression* expr);
    std::string emitNumberLiteral(const FasterBASIC::NumberExpression* expr);
    std::string emitStringLiteral(const FasterBASIC::StringExpression* expr);
    std::string emitVariableExpression(const FasterBASIC::VariableExpression* expr);
    std::string emitArrayAccessExpression(const FasterBASIC::ArrayAccessExpression* expr);
    std::string emitFunctionCall(const FasterBASIC::FunctionCallExpression* expr);
    std::string emitIIFExpression(const FasterBASIC::IIFExpression* expr);
    
    // === Binary Operation Helpers ===
    
    std::string emitArithmeticOp(const std::string& left, const std::string& right,
                                 FasterBASIC::TokenType op, FasterBASIC::BaseType type);
    std::string emitComparisonOp(const std::string& left, const std::string& right,
                                 FasterBASIC::TokenType op, FasterBASIC::BaseType type);
    std::string emitLogicalOp(const std::string& left, const std::string& right,
                             FasterBASIC::TokenType op);
    std::string emitStringOp(const std::string& left, const std::string& right,
                            FasterBASIC::TokenType op);
    
    // === Type Conversion Helpers ===
    
    std::string emitTypeConversion(const std::string& value, 
                                   FasterBASIC::BaseType fromType,
                                   FasterBASIC::BaseType toType);
    
    // === Helper: get QBE operator name ===
    
    std::string getQBEArithmeticOp(FasterBASIC::TokenType op);
    std::string getQBEComparisonOp(FasterBASIC::TokenType op);
};

} // namespace fbc

#endif // AST_EMITTER_H