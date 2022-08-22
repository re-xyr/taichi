#include "taichi/ir/type_system.h"
#include "taichi/ir/statements.h"

namespace taichi {
namespace lang {

void TyVar::unify(int pos,
                  DataType dt,
                  std::map<Identifier, DataType> &solutions) const {
  if (solutions.find(name) != solutions.end()) {
    if (solutions[name] != dt) {
      throw TyVarMismatch(name, solutions[name], dt);
    }
  } else {
    solutions[name] = dt;
  }
}

DataType TyVar::resolve(const std::map<Identifier, DataType> &solutions) const {
  if (solutions.find(name) == solutions.end()) {
    throw TyVarUnsolved(name);
  } else {
    return solutions.at(name);
  }
}

std::string TyVar::to_string() const {
  return name.name();
}

void TyLub::unify(int pos,
                  DataType dt,
                  std::map<Identifier, DataType> &solutions) const {
  TyMono(resolve(solutions)).unify(pos, dt, solutions);
}

DataType TyLub::resolve(const std::map<Identifier, DataType> &solutions) const {
  return promoted_type(lhs->resolve(solutions)->get_compute_type(),
                       rhs->resolve(solutions)->get_compute_type());
}

std::string TyLub::to_string() const {
  return lhs->to_string() + " | " + rhs->to_string();
}

void TyCompute::unify(int pos,
                      DataType dt,
                      std::map<Identifier, DataType> &solutions) const {
  TyMono(resolve(solutions)).unify(pos, dt, solutions);
}

DataType TyCompute::resolve(
    const std::map<Identifier, DataType> &solutions) const {
  return exp->resolve(solutions)->get_compute_type();
}

std::string TyCompute::to_string() const {
  return "comp(" + exp->to_string() + ")";
}

void TyMono::unify(int pos,
                   DataType dt,
                   std::map<Identifier, DataType> &solutions) const {
  if (monotype != dt) {
    throw TypeMismatch(pos, monotype, dt);
  }
}

DataType TyMono::resolve(
    const std::map<Identifier, DataType> &solutions) const {
  return monotype;
}

std::string TyMono::to_string() const {
  return monotype.to_string();
}

std::string TypeMismatch::to_string() const {
  return "expected " + arg.to_string() + " for argument " +
         std::to_string(position) + ", but got " + param.to_string();
}

std::string TyVarUnsolved::to_string() const {
  return "cannot infer the type variable " + var.name() +
         ". this is not supposed to happen; please report this as a bug";
}

std::string TraitMismatch::to_string() const {
  return "the argument type " + dt.to_string() + " is not " +
         trait->to_string();
}

std::string ArgLengthMismatch::to_string() const {
  return std::to_string(arg) + " arguments were passed in but expected " +
         std::to_string(param) +
         ". this is not supposed to happen; please report this as a bug";
}

DataType Signature::type_check(std::vector<DataType> arguments) const {
  if (parameters.size() != arguments.size()) {
    throw ArgLengthMismatch(parameters.size(), arguments.size());
  }
  std::map<Identifier, DataType> solutions;
  for (int i = 0; i < parameters.size(); i++) {
    parameters[i]->unify(i, arguments[i], solutions);
  }
  for (auto &c : constraints) {
    auto dt = c.tyvar->resolve(solutions);
    if (!c.trait->validate(dt)) {
      throw TraitMismatch(dt, c.trait);
    }
  }
  return ret_type->resolve(solutions);
}

void Operation::type_check(std::vector<DataType> arg_types) const {
  try {
    sig.type_check(arg_types);
  } catch (TypeSystemError &err) {
    std::string msg;
    msg += "In a call to the operation `" + name + "`:\n";
    msg += "  " + err.to_string();
    msg += "  ( called with argument types ";
    for (int i = 0; i < arg_types.size(); i++) {
      msg += arg_types[i].to_string();
      if (i != arg_types.size())
        msg += ", ";
      else
        msg += " )\n";
    }
    throw TaichiTypeError(msg);
  }
}

bool DynamicTrait::validate(const DataType dt) const {
  return impl(dt);
}

std::string DynamicTrait::to_string() const {
  return name;
}

StaticTraits::StaticTraits() {
  real = new DynamicTrait("Real", is_real);
  integral = new DynamicTrait("Integral", is_integral);
  primitive = new DynamicTrait(
      "Primitive", [](const DataType dt) { return dt->is<PrimitiveType>(); });
  scalar = new DynamicTrait("Scalar", [](const DataType dt) {
    return is_real(dt) || is_integral(dt);
  });
}

std::shared_ptr<StaticTraits> StaticTraits::get() {
  if (traits_ == nullptr) {
    traits_ = std::make_shared<StaticTraits>();
  }
  return traits_;
}

std::vector<TypeExpr> type_exprs_from_dts(std::vector<DataType> params) {
  std::vector<TypeExpr> exprs;
  for (auto dt : params) {
    exprs.push_back(std::make_shared<TyMono>(dt));
  }
  return exprs;
}

std::vector<Stmt *> get_all_stmts(std::vector<Expr> args,
                                  Expression::FlattenContext *ctx) {
  std::vector<Stmt *> stmts;
  for (auto arg : args) {
    arg->flatten(ctx);
    stmts.push_back(ctx->back_stmt());
  }
  return stmts;
}

class InternalCallOperation : public Operation {
  const std::string internal_call_name_;
  const bool with_runtime_context_;

 public:
  InternalCallOperation(std::string internal_name,
                        std::vector<DataType> params,
                        DataType result,
                        bool with_runtime_context)
      : Operation(internal_name,
                  Signature({},
                            type_exprs_from_dts(params),
                            std::make_shared<TyMono>(result))),
        internal_call_name_(internal_name),
        with_runtime_context_(with_runtime_context) {
  }

  Stmt *flatten(Expression::FlattenContext *ctx,
                std::vector<Expr> args,
                DataType ret_type) const override {
    return ctx->push_back<InternalFuncStmt>(
        internal_call_name_, get_all_stmts(args, ctx), (Type *)ret_type,
        with_runtime_context_);
  }
};

InternalOps::InternalOps() {
  auto f32_ptr =
      TypeFactory::get_instance().get_pointer_type(PrimitiveType::f32, false);
#define COMPOSITE_EXTRACT(n)                         \
  composite_extract_##n = new InternalCallOperation( \
      "composite_extract_" #n, {f32_ptr}, PrimitiveType::f32, false)
  COMPOSITE_EXTRACT(0);
  COMPOSITE_EXTRACT(1);
  COMPOSITE_EXTRACT(2);
  COMPOSITE_EXTRACT(3);
#undef COMPOSITE_EXTRACT

#define INSERT_TRIPLET(dt)                                               \
  insert_triplet_##dt =                                                  \
      new InternalCallOperation("insert_triplet_" #dt,                   \
                                {PrimitiveType::u64, PrimitiveType::i32, \
                                 PrimitiveType::i32, PrimitiveType::dt}, \
                                PrimitiveType::i32, true)
  INSERT_TRIPLET(f32);
  INSERT_TRIPLET(f64);
#undef INSERT_TRIPLET

#define SIMPLE_OP(name) \
  name = new InternalCallOperation(#name, {}, PrimitiveType::i32, false)
  SIMPLE_OP(block_barrier);
  SIMPLE_OP(workgroupBarrier);
  SIMPLE_OP(workgroupMemoryBarrier);
  SIMPLE_OP(localInvocationId);
  SIMPLE_OP(vkGlobalThreadIdx);
  SIMPLE_OP(grid_memfence);
  SIMPLE_OP(subgroupBarrier);
  SIMPLE_OP(subgroupMemoryBarrier);
#undef SIMPLE_OP

#define UNARY_OP
}

}  // namespace lang
}  // namespace taichi
