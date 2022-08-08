#pragma once

#include "taichi/ir/ir.h"
#include "taichi/ir/frontend_ir.h"

namespace taichi {
namespace lang {

class TypeSystemError {
 public:
  virtual std::string to_string() const = 0;
};

class TyVar;

class TypeExpression {
 public:
  virtual void unify(int pos,
                     DataType dt,
                     std::map<Identifier, DataType> &solutions) const = 0;
  virtual DataType resolve(std::map<Identifier, DataType> const &solutions) const = 0;
  virtual std::string to_string() const = 0;
};

using TypeExpr = std::shared_ptr<TypeExpression>;

class TyVar : public TypeExpression {
  const Identifier name;
 public:
  TyVar(Identifier id) : name(id) {}
  void unify(int pos,
             DataType dt,
             std::map<Identifier, DataType> &solutions) const override;
  DataType resolve(std::map<Identifier, DataType> const &solutions) const override;
  std::string to_string() const override;
};

class TyLub : public TypeExpression {
  const TypeExpr lhs, rhs;
 public:
  TyLub(TypeExpr lhs, TypeExpr rhs) : lhs(lhs), rhs(rhs) {}
  void unify(int pos,
             DataType dt,
             std::map<Identifier, DataType> &solutions) const override;
  DataType resolve(std::map<Identifier, DataType> const &solutions) const override;
  std::string to_string() const override;
};

class TyCompute : public TypeExpression {
  const TypeExpr exp;
 public:
  TyCompute(TypeExpr exp) : exp(exp) {}
  void unify(int pos,
             DataType dt,
             std::map<Identifier, DataType> &solutions) const override;
  DataType resolve(std::map<Identifier, DataType> const &solutions) const override;
  std::string to_string() const override;
};

class TyMono : public TypeExpression {
  const DataType monotype;
 public:
  TyMono(DataType dt) : monotype(dt) {}
  void unify(int pos,
             DataType dt,
             std::map<Identifier, DataType> &solutions) const override;
  DataType resolve(std::map<Identifier, DataType> const &solutions) const override;
  std::string to_string() const override;
};

class TyVarMismatch : public TypeSystemError {
  const Identifier var;
  const DataType original, conflicting;
 public:
  TyVarMismatch(Identifier var, DataType original, DataType conflicting) : var(var), original(original), conflicting(conflicting) {}
  std::string to_string() const override;
};

class TypeMismatch : public TypeSystemError {
  int position;
  const DataType param, arg;
 public:
  TypeMismatch(int pos, DataType param, DataType arg) : position(pos), param(param), arg(arg) {}
  std::string to_string() const override;
};

class TyVarUnsolved : public TypeSystemError {
  const Identifier var;
 public:
  TyVarUnsolved(Identifier var) : var(var){}
  std::string to_string() const override;
};

class Trait {
 public:
  virtual bool validate(const DataType dt) = 0;
  virtual std::string to_string() const = 0;
};

class TraitMismatch : public TypeSystemError {
  const DataType dt;
  const std::shared_ptr<Trait> trait;
 public:
  TraitMismatch(DataType dt, std::shared_ptr<Trait> trait) : dt(dt), trait(trait) {}
  std::string to_string() const override;
};

class ArgLengthMismatch : public TypeSystemError {
  const int param, arg;
 public:
  ArgLengthMismatch(int param, int arg) : param(param), arg(arg) {}
  std::string to_string() const override;
};

class Constraint {
 public:
  const std::shared_ptr<TyVar> tyvar;
  const std::shared_ptr<Trait> trait;
};

class Signature {
  const std::vector<Constraint> constraints;
  const std::vector<TypeExpr> parameters;
  const TypeExpr ret_type;

 public:
  DataType type_check(std::vector<DataType> arguments) const;
};

class Operation {
  const Signature sig;
  const std::string name;
 public:
  void type_check(std::vector<DataType> arg_types) const;
  virtual void flatten(Expression::FlattenContext *ctx, std::vector<Expr> args) const = 0;
};

}
}