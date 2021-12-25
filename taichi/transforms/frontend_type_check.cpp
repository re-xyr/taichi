#include "taichi/ir/ir.h"
#include "taichi/ir/frontend_ir.h"
#include "taichi/ir/statements.h"

namespace taichi::lang {

class FrontendTypeCheck : public IRVisitor {
 public:
  explicit FrontendTypeCheck() {
    allow_undefined_visitor = true;
  }

  void visit(Block *block) override {
    std::vector<Stmt *> stmts;
    // Make a copy since type casts may be inserted for type promotion.
    for (auto &stmt : block->statements) {
      stmts.push_back(stmt.get());
    }
    for (auto stmt : stmts)
      stmt->accept(this);
  }

  void visit(FrontendExprStmt *stmt) override {
    // Noop
  }

  void visit(FrontendAllocaStmt *stmt) override {
    // Noop
  }

  void visit(FrontendSNodeOpStmt *stmt) override {
    // Noop
  }

  void visit(FrontendAssertStmt *stmt) override {
    if (not stmt->cond->ret_type->is_primitive(PrimitiveTypeID::i32))
      throw TaichiTypeError(fmt::format(
          "`assert` conditions must be of type int32; found {}. Consider using "
          "`if x != 0:` instead of `if x:` for float values.",
          stmt->cond->ret_type->to_string()));
  }

  void visit(FrontendAssignStmt *stmt) override {
    // No implicit cast at frontend for now
  }

  void visit(FrontendIfStmt *stmt) override {
    // TODO: use PrimitiveType::u1 when it's supported
    if (not stmt->condition->ret_type->is_primitive(PrimitiveTypeID::i32))
      throw TaichiTypeError(fmt::format(
          "`if` conditions must be of type int32; found {}. Consider using "
          "`assert x != 0` instead of `assert x` for float values.",
          stmt->condition->ret_type->to_string()));
    if (stmt->true_statements)
      stmt->true_statements->accept(this);
    if (stmt->false_statements) {
      stmt->false_statements->accept(this);
    }
  }

  void visit(FrontendPrintStmt *stmt) override {
    // Noop
  }

  void visit(FrontendEvalStmt *stmt) override {
    // Noop
  }

  void visit(FrontendForStmt *stmt) override {
    stmt->body->accept(this);
  }

  void visit(FrontendFuncDefStmt *stmt) override {
    stmt->body->accept(this);
    // Determine ret_type after this is actually used
  }

  void visit(FrontendBreakStmt *stmt) override {
    // Noop
  }

  void visit(FrontendContinueStmt *stmt) override {
    // Noop
  }

  void visit(FrontendWhileStmt *stmt) override {
    if (not stmt->cond->ret_type->is_primitive(PrimitiveTypeID::i32))
      throw TaichiTypeError(fmt::format(
          "`while` conditions must be of type int32; found {}. Consider using "
          "`while x != 0:` instead of `while x:` for float values.",
          stmt->cond->ret_type->to_string()));
    stmt->body->accept(this);
  }

  void visit(FrontendReturnStmt *stmt) override {
    // Noop
  }
};


namespace irpass {

void frontend_type_check(IRNode *root) {
  TI_AUTO_PROF;
  FrontendTypeCheck checker;
  root->accept(&checker);
}

}

}
