#pragma once

#include <vector>

#ifndef WIN32

#ifndef isfinite
#define isfinite(val) (val <= std::numeric_limits<double>::max())
#endif

#endif

#undef COMPILER
#include "src/v8.h"
#include "src/ast.h"
#include "src/scopes.h"
#include "src/assembler.h"

#include "utf8.h"

#include "PrettyPrinter.h"
#include "Wrapper.h"

namespace v8i = v8::internal;

using namespace v8::internal;

template <typename T>
inline py::object to_python(T& obj)
{
  return py::object(py::handle<>(py::to_python_value<T&>()(obj)));
}

template <typename T>
inline py::object to_python(v8i::Zone *zone, T *node);

template <typename T>
inline py::list to_python(v8i::Zone *zone, v8i::ZoneList<T *>* lst);

template <typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst);

template <typename T>
T _to_string(v8i::Handle<v8i::String> str)
{
  if (str.is_null()) return T();

  v8i::String::FlatContent content = str->GetFlatContent();

  if (content.IsFlat())
  {
    if (content.IsAscii())
    {
        v8i::Vector<const uint8_t> buf = content.ToOneByteVector();

      return T((const char *) buf.start(), buf.length());
    }
    else
    {
      v8i::Vector<const v8i::uc16> buf = content.ToUC16Vector();
      std::vector<char> out;

      utf8::utf16to8(buf.start(), buf.start() + buf.length(), out.begin());

      return T(&out[0], out.size());
    }
  }
  else
  {
    int len = 0;
    v8i::SmartArrayPointer<char> buf = str->ToCString(v8i::DISALLOW_NULLS, v8i::FAST_STRING_TRAVERSAL, &len);

    return T(buf.get(), len);
  }
}

inline py::object to_python(v8i::Handle<v8i::String> str)
{
  return _to_string<py::str>(str);
}

inline const std::string to_string(v8i::Handle<v8i::String> str)
{
  return _to_string<std::string>(str);
}

class CAstVisitor;
class CAstVariable;
class CAstVariableProxy;
class CAstFunctionLiteral;

class CAstScope
{
  v8i::Scope *m_scope;
public:
  CAstScope(v8i::Scope *scope) : m_scope(scope) {}

  bool IsEval(void) const { return m_scope->is_eval_scope(); }
  bool IsFunction(void) const { return m_scope->is_function_scope(); }
  bool IsModule(void) const { return m_scope->is_module_scope(); }
  bool IsGlobal(void) const { return m_scope->is_global_scope(); }
  bool IsCatch(void) const { return m_scope->is_catch_scope(); }
  bool IsBlock(void) const { return m_scope->is_block_scope(); }
  bool IsWith(void) const { return m_scope->is_with_scope(); }
  bool IsDeclaration(void) const { return m_scope->is_declaration_scope(); }
  bool IsStrictOrExtendedEval(void) const { return m_scope->is_strict_or_extended_eval_scope(); }

  bool IsClassicMode(void) const { return m_scope->is_classic_mode(); }
  bool IsExtendedMode(void) const { return m_scope->is_extended_mode(); }

  bool CallsEval(void) const { return m_scope->calls_eval(); }
  bool OuterScopeCallsEval(void) const { return m_scope->calls_eval(); }

  bool InsideWith(void) const { return m_scope->inside_with(); }
  bool ContainsWith(void) const { return m_scope->contains_with(); }

  py::object GetOuter(void) const { v8i::Scope *scope = m_scope->outer_scope(); return scope ? py::object(CAstScope(scope)) : py::object(); }

  py::list GetDeclarations(void) const { return to_python(m_scope->zone(), m_scope->declarations()); }

  py::object GetReceiver(void) const;
  py::object GetFunction(void) const { return to_python(m_scope->zone(), m_scope->function()); }
  int GetParametersNumer(void) const { return m_scope->num_parameters(); }
  CAstVariable GetParameter(int index) const;
  py::object GetArguments(void) const;
};

class CAstVariable
{
  v8i::Variable *m_var;
public:
  CAstVariable(v8i::Variable *var) : m_var(var) {}

  v8i::Variable *GetVariable(void) const { return m_var; }

  bool IsValidLeftHandSide(void) const { return m_var->IsValidLeftHandSide(); }

  CAstScope scope(void) const { return CAstScope(m_var->scope()); }
  py::object name(void) const { return to_python(m_var->name()); }
  v8i::VariableMode mode(void) const { return m_var->mode(); }

  bool is_this(void) const { return m_var->is_this(); }
  bool is_arguments(void) const { return m_var->is_arguments(); }
  bool is_possibly_eval(void) const;
  v8i::Variable::Location location(void) const { return m_var->location(); }
  int index(void) const { return m_var->index(); }
};

class CAstLabel
{
  v8i::Label *m_label;
public:
  CAstLabel(v8i::Label *label) : m_label(label)
  {
  }

  int GetPosition(void) const { return m_label->pos(); }
  bool IsBound(void) const { return m_label->is_bound(); }
  bool IsUnused(void) const { return m_label->is_unused(); }
  bool IsLinked(void) const { return m_label->is_linked(); }
};

class CAstNode
{
protected:
  v8i::Zone *m_zone;
  v8i::AstNode *m_node;

  CAstNode(v8i::Zone *zone, v8i::AstNode *node) : m_zone(zone), m_node(node) {}

  void Visit(py::object handler);
public:
  virtual ~CAstNode() {}

  template <typename T>
  T *as(void) const { return static_cast<T *>(m_node); }

  v8i::Zone *zone(void) const { return m_zone; }

  static void Expose(void);

  v8i::AstNode::NodeType GetType(void) const { return m_node->node_type(); }

  int GetPosition(void) const { return as<v8i::AstNode>()->position(); }

  const std::string ToString(void) const { return v8i::PrettyPrinter(m_zone).Print(m_node); }
};

class CAstStatement : public CAstNode
{
protected:
  CAstStatement(v8i::Zone *zone, v8i::Statement *stat) : CAstNode(zone, stat) {}
public:
  operator bool(void) const { return !as<v8i::Statement>()->IsEmpty(); }
};

class CAstExpression : public CAstNode
{
protected:
  CAstExpression(v8i::Zone *zone, v8i::Expression *expr) : CAstNode(zone, expr) {}
public:
  bool IsPropertyName(void) { return as<v8i::Expression>()->IsPropertyName(); }

  // True iff the expression is a literal represented as a smi.
  bool IsSmiLiteral(void) { return as<v8i::Expression>()->IsSmiLiteral(); }

  // True iff the expression is a string literal.
  bool IsStringLiteral(void) { return as<v8i::Expression>()->IsStringLiteral(); }

  // True iff the expression is the null literal.
  bool IsNullLiteral(void) { return as<v8i::Expression>()->IsNullLiteral(); }
};

class CAstBreakableStatement : public CAstStatement
{
protected:
  CAstBreakableStatement(v8i::Zone *zone, v8i::BreakableStatement *stat) : CAstStatement(zone, stat) {}
public:
  bool IsTargetForAnonymous(void) const { return as<v8i::BreakableStatement>()->is_target_for_anonymous(); }

  CAstLabel GetBreakTarget(void) { return CAstLabel(as<v8i::BreakableStatement>()->break_target()); }
};

class CAstBlock : public CAstBreakableStatement
{
public:
  CAstBlock(v8i::Zone *zone, v8i::Block *block) : CAstBreakableStatement(zone, block) {}

  void AddStatement(CAstStatement& stmt) { as<v8i::Block>()->AddStatement(stmt.as<v8i::Statement>(), v8i::Isolate::Current()->runtime_zone()); }

  py::list GetStatements(void) { return to_python(m_zone, as<v8i::Block>()->statements()); }

  bool IsInitializerBlock(void) const { return as<v8i::Block>()->is_initializer_block(); }
};

class CAstDeclaration : public CAstNode
{
public:
  CAstDeclaration(v8i::Zone *zone, v8i::Declaration *decl) : CAstNode(zone, decl) {}

  py::object GetProxy(void) const;
  v8i::VariableMode GetMode(void) const { return as<v8i::Declaration>()->mode(); }
  CAstScope GetScope(void) const { return CAstScope(as<v8i::Declaration>()->scope()); }
};

class CAstVariableDeclaration : public CAstDeclaration
{
public:
  CAstVariableDeclaration(v8i::Zone *zone, v8i::VariableDeclaration *decl) : CAstDeclaration(zone, decl) {}
};

class CAstFunctionDeclaration : public CAstDeclaration
{
public:
	CAstFunctionDeclaration(v8i::Zone *zone, v8i::FunctionDeclaration *decl) : CAstDeclaration(zone, decl) {}

	py::object GetFunction(void) const { return to_python(m_zone, as<v8i::FunctionDeclaration>()->fun()); }
};

class CAstModule : public CAstNode
{
public:
  CAstModule(v8i::Zone *zone, v8i::Module *mod) : CAstNode(zone, mod) {}

  py::object GetBody(void) const { return to_python(m_zone, as<v8i::ModuleLiteral>()->body()); }
};

class CAstModuleLiteral : public CAstModule
{
public:
  CAstModuleLiteral(v8i::Zone *zone, v8i::ModuleLiteral *mod) : CAstModule(zone, mod) {}
};

class CAstModuleVariable : public CAstModule
{
public:
  CAstModuleVariable(v8i::Zone *zone, v8i::ModuleVariable *mod) : CAstModule(zone, mod) {}

  py::object GetProxy(void) const { return to_python(m_zone, as<v8i::ModuleVariable>()->proxy()); }
};

class CAstModulePath : public CAstModule
{
public:
  CAstModulePath(v8i::Zone *zone, v8i::ModulePath *mod) : CAstModule(zone, mod) {}

  py::object GetModule(void) const { return to_python(m_zone, as<v8i::ModulePath>()->module()); }
  const std::string GetName(void) const { return to_string(as<v8i::ModulePath>()->name()); }
};

class CAstModuleUrl : public CAstModule
{
public:
  CAstModuleUrl(v8i::Zone *zone, v8i::ModuleUrl *mod) : CAstModule(zone, mod) {}

  const std::string GetUrl(void) const { return to_string(as<v8i::ModuleUrl>()->url()); }
};

class CAstModuleDeclaration : public CAstDeclaration
{
public:
  CAstModuleDeclaration(v8i::Zone *zone, v8i::ModuleDeclaration *decl) : CAstDeclaration(zone, decl) {}

  py::object GetModule(void) const { return to_python(m_zone, as<v8i::ModuleDeclaration>()->module()); }
};

class CAstImportDeclaration : public CAstDeclaration
{
public:
	CAstImportDeclaration(v8i::Zone *zone, v8i::ImportDeclaration *decl) : CAstDeclaration(zone, decl) {}

	py::object GetModule(void) const { return to_python(m_zone, as<v8i::ImportDeclaration>()->module()); }
};

class CAstExportDeclaration : public CAstDeclaration
{
public:
	CAstExportDeclaration(v8i::Zone *zone, v8i::ExportDeclaration *decl) : CAstDeclaration(zone, decl) {}
};

class CAstModuleStatement : public CAstStatement
{
public:
    CAstModuleStatement(v8i::Zone *zone, v8i::ModuleStatement *stat) : CAstStatement(zone, stat) {}

    py::object GetProxy(void) const { return to_python(m_zone, as<v8i::ModuleStatement>()->proxy()); }

    py::object GetBody(void) const { return to_python(m_zone, as<v8i::ModuleStatement>()->body()); }
};

class CAstIterationStatement : public CAstBreakableStatement
{
protected:
  CAstIterationStatement(v8i::Zone *zone, v8i::IterationStatement *stat) : CAstBreakableStatement(zone, stat) {}
public:
  py::object GetBody(void) const { return to_python(m_zone, as<v8i::IterationStatement>()->body()); }

  CAstLabel GetContinueTarget(void) { return CAstLabel(as<v8i::IterationStatement>()->continue_target()); }
};

class CAstDoWhileStatement : public CAstIterationStatement
{
public:
  CAstDoWhileStatement(v8i::Zone *zone, v8i::DoWhileStatement *stat) : CAstIterationStatement(zone, stat) {}

  py::object GetCondition(void) const { return to_python(m_zone, as<v8i::DoWhileStatement>()->cond()); }
};

class CAstWhileStatement : public CAstIterationStatement
{
public:
  CAstWhileStatement(v8i::Zone *zone, v8i::WhileStatement *stat) : CAstIterationStatement(zone, stat) {}

  py::object GetCondition(void) const { return to_python(m_zone, as<v8i::WhileStatement>()->cond()); }
};

class CAstForStatement : public CAstIterationStatement
{
public:
  CAstForStatement(v8i::Zone *zone, v8i::ForStatement *stat) : CAstIterationStatement(zone, stat) {}

  py::object GetInit(void) const { return to_python(m_zone, as<v8i::ForStatement>()->init()); }
  py::object GetCondition(void) const { return to_python(m_zone, as<v8i::ForStatement>()->cond()); }
  py::object GetNext(void) const { return to_python(m_zone, as<v8i::ForStatement>()->next()); }

  CAstVariable GetLoopVariable(void) const { return CAstVariable(as<v8i::ForStatement>()->loop_variable()); }
  void SetLoopVariable(CAstVariable& var) { as<v8i::ForStatement>()->set_loop_variable(var.GetVariable()); }

  bool IsFastLoop(void) const { return as<v8i::ForStatement>()->is_fast_smi_loop(); }
};

class CAstForEachStatement : public CAstIterationStatement
{
public:
  CAstForEachStatement(v8i::Zone *zone, v8i::ForEachStatement *stat) : CAstIterationStatement(zone, stat) {}

  py::object GetEach(void) const { return to_python(m_zone, as<v8i::ForEachStatement>()->each()); }
  py::object GetSubject(void) const { return to_python(m_zone, as<v8i::ForEachStatement>()->subject()); }
};

class CAstForInStatement : public CAstForEachStatement
{
public:
  CAstForInStatement(v8i::Zone *zone, v8i::ForInStatement *stat) : CAstForEachStatement(zone, stat) {}

  py::object GetEnumerable(void) const { return to_python(m_zone, as<v8i::ForInStatement>()->enumerable()); }
};

class CAstForOfStatement : public CAstForEachStatement
{
public:
  CAstForOfStatement(v8i::Zone *zone, v8i::ForOfStatement *stat) : CAstForEachStatement(zone, stat) {}

  py::object GetIterable(void) const { return to_python(m_zone, as<v8i::ForOfStatement>()->iterable()); }
  py::object AssignIterator(void) const { return to_python(m_zone, as<v8i::ForOfStatement>()->assign_iterator()); }
  py::object NextResult(void) const { return to_python(m_zone, as<v8i::ForOfStatement>()->next_result()); }
  py::object ResultDone(void) const { return to_python(m_zone, as<v8i::ForOfStatement>()->result_done()); }
  py::object AssignEach(void) const { return to_python(m_zone, as<v8i::ForOfStatement>()->assign_each()); }
};

class CAstExpressionStatement : public CAstStatement
{
public:
  CAstExpressionStatement(v8i::Zone *zone, v8i::ExpressionStatement *stat) : CAstStatement(zone, stat) {}

  py::object GetExpression(void) const { return to_python(m_zone, as<v8i::ExpressionStatement>()->expression()); }
};

class CAstContinueStatement : public CAstStatement
{
public:
  CAstContinueStatement(v8i::Zone *zone, v8i::ContinueStatement *stat) : CAstStatement(zone, stat) {}

  py::object GetTarget(void) const { return to_python(m_zone, as<v8i::ContinueStatement>()->target()); }
};

class CAstBreakStatement : public CAstStatement
{
public:
  CAstBreakStatement(v8i::Zone *zone, v8i::BreakStatement *stat) : CAstStatement(zone, stat) {}

  py::object GetTarget(void) const { return to_python(m_zone, as<v8i::BreakStatement>()->target()); }
};

class CAstReturnStatement : public CAstStatement
{
public:
  CAstReturnStatement(v8i::Zone *zone, v8i::ReturnStatement *stat) : CAstStatement(zone, stat) {}

  py::object expression(void) const { return to_python(m_zone, as<v8i::ReturnStatement>()->expression()); }
};

class CAstWithStatement : public CAstStatement
{
public:
  CAstWithStatement(v8i::Zone *zone, v8i::WithStatement *stat) : CAstStatement(zone, stat) {}

  py::object expression(void) const { return to_python(m_zone, as<v8i::WithStatement>()->expression()); }
  py::object statement(void) const { return to_python(m_zone, as<v8i::WithStatement>()->statement()); }
};

class CAstCaseClause : public CAstNode
{
public:
  CAstCaseClause(v8i::Zone *zone, v8i::CaseClause *clause) : CAstNode(zone, clause) {}

  bool is_default(void) { return as<v8i::CaseClause>()->is_default(); }

  py::object label(void) { return as<v8i::CaseClause>()->is_default() ? py::object() : to_python(m_zone, as<v8i::CaseClause>()->label()); }

  CAstLabel body_target(void) { return CAstLabel(as<v8i::CaseClause>()->body_target()); }

  py::object statements(void) { return to_python(m_zone, as<v8i::CaseClause>()->statements()); }
};

class CAstSwitchStatement : public CAstBreakableStatement
{
public:
  CAstSwitchStatement(v8i::Zone *zone, v8i::SwitchStatement *stat) : CAstBreakableStatement(zone, stat) {}

  py::object tag(void) const { return to_python(m_zone, as<v8i::SwitchStatement>()->tag()); }

  py::list cases(void) const { return to_python(m_zone, as<v8i::SwitchStatement>()->cases()); }
};

class CAstIfStatement : public CAstStatement
{
public:
  CAstIfStatement(v8i::Zone *zone, v8i::IfStatement *stat) : CAstStatement(zone, stat) {}

  bool HasThenStatement(void) const { return as<v8i::IfStatement>()->HasThenStatement(); }
  bool HasElseStatement(void) const { return as<v8i::IfStatement>()->HasElseStatement(); }

  py::object GetCondition(void) const { return to_python(m_zone, as<v8i::IfStatement>()->condition()); }

  py::object GetThenStatement(void) const { return to_python(m_zone, as<v8i::IfStatement>()->then_statement()); }
  py::object GetElseStatement(void) const { return to_python(m_zone, as<v8i::IfStatement>()->else_statement()); }
};

class CAstTargetCollector : public CAstNode
{
public:
  CAstTargetCollector(v8i::Zone *zone, v8i::TargetCollector *collector) : CAstNode(zone, collector) {}

  py::list GetTargets(void) const;
};

class CAstTryStatement : public CAstStatement
{
protected:
  CAstTryStatement(v8i::Zone *zone, v8i::TryStatement *stat) : CAstStatement(zone, stat) {}
public:
  CAstBlock GetTryBlock(void) const { return CAstBlock(m_zone, as<v8i::TryStatement>()->try_block()); }
  py::list GetEscapingTargets(void) const;
};

class CAstTryCatchStatement : public CAstTryStatement
{
public:
  CAstTryCatchStatement(v8i::Zone *zone, v8i::TryCatchStatement *stat) : CAstTryStatement(zone, stat) {}

  CAstBlock GetCatchBlock(void) const { return CAstBlock(m_zone, as<v8i::TryCatchStatement>()->catch_block()); }
  CAstScope GetScope(void) const { return CAstScope(as<v8i::TryCatchStatement>()->scope()); }
  CAstVariable GetVariable(void) const { return CAstVariable(as<v8i::TryCatchStatement>()->variable()); }
};

class CAstTryFinallyStatement : public CAstTryStatement
{
public:
  CAstTryFinallyStatement(v8i::Zone *zone, v8i::TryFinallyStatement *stat) : CAstTryStatement(zone, stat) {}

  CAstBlock GetFinallyBlock(void) const { return CAstBlock(m_zone, as<v8i::TryFinallyStatement>()->finally_block()); }
};

class CAstDebuggerStatement : public CAstStatement
{
public:
  CAstDebuggerStatement(v8i::Zone *zone, v8i::DebuggerStatement *stat) : CAstStatement(zone, stat) {}
};

class CAstEmptyStatement : public CAstStatement
{
public:
  CAstEmptyStatement(v8i::Zone *zone, v8i::EmptyStatement *stat) : CAstStatement(zone, stat) {}
};

class CAstLiteral : public CAstExpression
{
public:
  CAstLiteral(v8i::Zone *zone, v8i::Literal *lit) : CAstExpression(zone, lit) {}

  const std::string AsPropertyName(void) const { return to_string(as<v8i::Literal>()->AsPropertyName()); }
  bool IsNull(void) const { return as<v8i::Literal>()->IsNull(); }
  bool IsTrue(void) const { return as<v8i::Literal>()->IsTrue(); }
  bool IsFalse(void) const { return as<v8i::Literal>()->IsFalse(); }
};

class CAstMaterializedLiteral : public CAstExpression
{
protected:
  CAstMaterializedLiteral(v8i::Zone *zone, v8i::MaterializedLiteral *lit) : CAstExpression(zone, lit) {}
public:
  int GetIndex(void) const { return as<v8i::MaterializedLiteral>()->literal_index(); }
};

class CAstObjectProperty
{
  v8i::Zone *m_zone;
  v8i::ObjectLiteral::Property *m_prop;
public:
  CAstObjectProperty(v8i::Zone *zone, v8i::ObjectLiteral::Property *prop) : m_zone(zone), m_prop(prop)
  {

  }

  py::object GetKey(void) { return to_python(m_zone, m_prop->key()); }
  py::object GetValue(void) { return to_python(m_zone, m_prop->value()); }
  v8i::ObjectLiteral::Property::Kind GetKind(void) { return m_prop->kind(); }

  bool IsCompileTimeValue(void) { return m_prop->IsCompileTimeValue(); }
};

class CAstObjectLiteral : public CAstMaterializedLiteral
{
public:
  CAstObjectLiteral(v8i::Zone *zone, v8i::ObjectLiteral *lit) : CAstMaterializedLiteral(zone, lit) {}

  py::list GetProperties(void) const { return to_python(as<v8i::ObjectLiteral>()->properties()); }
};

class CAstRegExpLiteral : public CAstMaterializedLiteral
{
public:
  CAstRegExpLiteral(v8i::Zone *zone, v8i::RegExpLiteral *lit) : CAstMaterializedLiteral(zone, lit) {}

  const std::string GetPattern(void) const { return to_string(as<v8i::RegExpLiteral>()->pattern()); }
  const std::string GetFlags(void) const { return to_string(as<v8i::RegExpLiteral>()->flags()); }
};

class CAstArrayLiteral : public CAstMaterializedLiteral
{
public:
  CAstArrayLiteral(v8i::Zone *zone, v8i::ArrayLiteral *lit) : CAstMaterializedLiteral(zone, lit) {}

  py::list GetValues(void) const { return to_python(m_zone, as<v8i::ArrayLiteral>()->values()); }
};

class CAstVariableProxy : public CAstExpression
{
public:
  CAstVariableProxy(v8i::Zone *zone, v8i::VariableProxy *proxy) : CAstExpression(zone, proxy) {}

  bool IsValidLeftHandSide(void) const { return as<v8i::VariableProxy>()->IsValidLeftHandSide(); }
  bool IsArguments(void) const { return as<v8i::VariableProxy>()->IsArguments(); }
  const std::string name(void) const { return to_string(as<v8i::VariableProxy>()->name()); }
  py::object var(void) const { v8i::Variable *var = as<v8i::VariableProxy>()->var(); return var ? py::object(CAstVariable(var)) : py::object();  }
  bool is_this() const  { return as<v8i::VariableProxy>()->is_this(); }
};

class CAstProperty : public CAstExpression
{
public:
  CAstProperty(v8i::Zone *zone, v8i::Property *prop) : CAstExpression(zone, prop) {}
};

class CAstCall : public CAstExpression
{
public:
  CAstCall(v8i::Zone *zone, v8i::Call *call) : CAstExpression(zone, call) {}

  py::object GetExpression(void) const { return to_python(m_zone, as<v8i::Call>()->expression()); }
  py::list GetArguments(void) const { return to_python(m_zone, as<v8i::Call>()->arguments()); }
};

class CAstCallNew : public CAstExpression
{
public:
  CAstCallNew(v8i::Zone *zone, v8i::CallNew *call) : CAstExpression(zone, call) {}

  py::object GetExpression(void) const { return to_python(m_zone, as<v8i::CallNew>()->expression()); }
  py::list GetArguments(void) const { return to_python(m_zone, as<v8i::CallNew>()->arguments()); }
};

class CAstCallRuntime : public CAstExpression
{
public:
  CAstCallRuntime(v8i::Zone *zone, v8i::CallRuntime *call) : CAstExpression(zone, call) {}

  const std::string GetName(void) const { return to_string(as<v8i::CallRuntime>()->name()); }
  py::list GetArguments(void) const { return to_python(m_zone, as<v8i::CallRuntime>()->arguments()); }
  bool IsJSRuntime(void) const { return as<v8i::CallRuntime>()->is_jsruntime(); }
};

class CAstUnaryOperation : public CAstExpression
{
public:
  CAstUnaryOperation(v8i::Zone *zone, v8i::UnaryOperation *op) : CAstExpression(zone, op) {}

  v8i::Token::Value op(void) const { return as<v8i::UnaryOperation>()->op(); }
  py::object expression(void) const { return to_python(m_zone, as<v8i::UnaryOperation>()->expression()); }
};

class CAstBinaryOperation : public CAstExpression
{
public:
  CAstBinaryOperation(v8i::Zone *zone, v8i::BinaryOperation *op) : CAstExpression(zone, op) {}

  v8i::Token::Value op(void) const { return as<v8i::BinaryOperation>()->op(); }
  py::object left(void) const { return to_python(m_zone, as<v8i::BinaryOperation>()->left()); }
  py::object right(void) const { return to_python(m_zone, as<v8i::BinaryOperation>()->right()); }
};

class CAstCountOperation : public CAstExpression
{
public:
  CAstCountOperation(v8i::Zone *zone, v8i::CountOperation *op) : CAstExpression(zone, op) {}

  bool is_prefix(void) const { return as<v8i::CountOperation>()->is_prefix(); }
  bool is_postfix(void) const { return as<v8i::CountOperation>()->is_postfix(); }

  v8i::Token::Value op(void) const { return as<v8i::CountOperation>()->op(); }
  v8i::Token::Value binary_op(void) const { return as<v8i::CountOperation>()->binary_op(); }

  py::object expression(void) const { return to_python(m_zone, as<v8i::CountOperation>()->expression()); }
};

class CAstCompareOperation : public CAstExpression
{
public:
  CAstCompareOperation(v8i::Zone *zone, v8i::CompareOperation *op) : CAstExpression(zone, op) {}

  v8i::Token::Value op(void) const { return as<v8i::CompareOperation>()->op(); }
  py::object left(void) const { return to_python(m_zone, as<v8i::CompareOperation>()->left()); }
  py::object right(void) const { return to_python(m_zone, as<v8i::CompareOperation>()->right()); }
};

class CAstConditional : public CAstExpression
{
public:
  CAstConditional(v8i::Zone *zone, v8i::Conditional *cond) : CAstExpression(zone, cond) {}

  py::object condition(void) const { return to_python(m_zone, as<v8i::Conditional>()->condition()); }
  py::object then_expression(void) const { return to_python(m_zone, as<v8i::Conditional>()->then_expression()); }
  py::object else_expression(void) const { return to_python(m_zone, as<v8i::Conditional>()->else_expression()); }
};

class CAstAssignment : public CAstExpression
{
public:
  CAstAssignment(v8i::Zone *zone, v8i::Assignment *assignment) : CAstExpression(zone, assignment) {}

  v8i::Token::Value binary_op(void) const { return as<v8i::Assignment>()->binary_op(); }

  v8i::Token::Value op(void) const { return as<v8i::Assignment>()->op(); }
  py::object target(void) const { return to_python(m_zone, as<v8i::Assignment>()->target()); }
  py::object value(void) const { return to_python(m_zone, as<v8i::Assignment>()->value()); }
  CAstBinaryOperation binary_operation(void) const { return CAstBinaryOperation(m_zone, as<v8i::Assignment>()->binary_operation()); }

  int is_compound(void) const { return as<v8i::Assignment>()->is_compound(); }
};

class CAstYield : public CAstExpression
{
public:
  CAstYield(v8i::Zone *zone, v8i::Yield *yield) : CAstExpression(zone, yield) {}

  py::object expression(void) const { return to_python(m_zone, as<v8i::Yield>()->expression()); }
  bool yield_kind(void) const { return as<v8i::Yield>()->yield_kind(); }
};

class CAstThrow : public CAstExpression
{
public:
  CAstThrow(v8i::Zone *zone, v8i::Throw *th) : CAstExpression(zone, th) {}

  py::object GetException(void) const { return to_python(m_zone, as<v8i::Throw>()->exception()); }
};

class CAstFunctionLiteral : public CAstExpression
{
public:
  CAstFunctionLiteral(v8i::Zone *zone, v8i::FunctionLiteral *func) : CAstExpression(zone, func) {}

  const std::string GetName(void) const { return to_string(as<v8i::FunctionLiteral>()->name()); }
  CAstScope GetScope(void) const { return CAstScope(as<v8i::FunctionLiteral>()->scope()); }
  py::list GetBody(void) const { return to_python(m_zone, as<v8i::FunctionLiteral>()->body()); }

  int GetStartPosition(void) const { return as<v8i::FunctionLiteral>()->start_position(); }
  int GetEndPosition(void) const { return as<v8i::FunctionLiteral>()->end_position(); }
  bool IsExpression(void) const { return as<v8i::FunctionLiteral>()->is_expression(); }

  const std::string ToAST(void) const { return v8i::AstPrinter(m_zone).PrintProgram(as<v8i::FunctionLiteral>()); }
  const std::string ToJSON(void) const { return v8i::JsonAstBuilder(m_zone).BuildProgram(as<v8i::FunctionLiteral>()); }
};


class CAstNativeFunctionLiteral : public CAstExpression
{
public:
  CAstNativeFunctionLiteral(v8i::Zone *zone, v8i::NativeFunctionLiteral *func) : CAstExpression(zone, func) {}

  const std::string GetName(void) const { return to_string(as<v8i::NativeFunctionLiteral>()->name()); }
};

class CAstThisFunction : public CAstExpression
{
public:
  CAstThisFunction(v8i::Zone *zone, v8i::ThisFunction *func) : CAstExpression(zone, func) {}
};

class CAstVisitor : public v8i::AstVisitor
{
  py::object m_handler;
public:
  CAstVisitor(v8i::Zone *zone, py::object handler) : m_handler(handler)
  {
    InitializeAstVisitor(zone);
  }
#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { \
  if (::PyObject_HasAttrString(m_handler.ptr(), "on"#type)) { \
    py::object callback = m_handler.attr("on"#type); \
    if (::PyCallable_Check(callback.ptr())) { \
      callback(py::object(CAst##type(zone(), node))); }; } }

  AST_NODE_LIST(DECLARE_VISIT)

#undef DECLARE_VISIT

  DEFINE_AST_VISITOR_SUBCLASS_MEMBERS();
};

struct CAstObjectCollector : public v8i::AstVisitor
{
  py::object m_obj;

  CAstObjectCollector(v8i::Zone *zone) {
    InitializeAstVisitor(zone);
  }

#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { m_obj = py::object(CAst##type(zone(), node)); }
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

  DEFINE_AST_VISITOR_SUBCLASS_MEMBERS();
};

template <typename T>
inline py::object to_python(v8i::Zone *zone, T *node)
{
  if (!node) return py::object();

  CAstObjectCollector collector(zone);

  node->Accept(&collector);

  return collector.m_obj;
}

struct CAstListCollector : public v8i::AstVisitor
{
  py::list m_nodes;

  CAstListCollector(v8i::Zone *zone) {
    InitializeAstVisitor(zone);
  }

#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { m_nodes.append(py::object(CAst##type(zone(), node))); }
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

  DEFINE_AST_VISITOR_SUBCLASS_MEMBERS();
};

template <typename T>
inline py::list to_python(v8i::Zone *zone, v8i::ZoneList<T *>* lst)
{
  if (!lst) return py::list();

  CAstListCollector collector(zone);

  for (int i=0; i<lst->length(); i++)
  {
    lst->at(i)->Accept(&collector);
  }

  return collector.m_nodes;
}

template <typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst)
{
  if (!lst) return py::list();

  py::list targets;

  for (int i=0; i<lst->length(); i++)
  {
    targets.append(to_python(lst->at(i)));
  }

  return targets;
}

inline py::object CAstScope::GetReceiver(void) const
{
  if (m_scope->receiver())
  {
    CAstVariable proxy(m_scope->receiver());

    return to_python(proxy);
  }

  return py::object();
}
inline CAstVariable CAstScope::GetParameter(int index) const
{
  return CAstVariable(m_scope->parameter(index));
}
inline py::object CAstScope::GetArguments(void) const
{
  if (m_scope->arguments())
  {
    CAstVariable var(m_scope->arguments());

    return to_python(var);
  }

  return py::object();
}

inline void CAstNode::Visit(py::object handler) { CAstVisitor(m_zone, handler).Visit(m_node); }

inline py::object CAstDeclaration::GetProxy(void) const { return to_python(m_zone, as<v8i::Declaration>()->proxy()); }

inline py::list CAstTargetCollector::GetTargets(void) const { return to_python(as<v8i::TargetCollector>()->targets()); }

inline py::list CAstTryStatement::GetEscapingTargets(void) const { return to_python(as<v8i::TryStatement>()->escaping_targets()); }
