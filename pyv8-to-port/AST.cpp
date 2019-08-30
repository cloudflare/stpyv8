#include "AST.h"

#include "V8Internal.h"

bool CAstVariable::is_possibly_eval(void) const
{
    return m_var->is_possibly_eval(v8i::Isolate::Current());
}

void CAstNode::Expose(void)
{
  py::class_<CAstScope>("AstScope", py::no_init)
    .add_property("isEval", &CAstScope::IsEval)
    .add_property("isFunc", &CAstScope::IsFunction)
    .add_property("isModule", &CAstScope::IsModule)
    .add_property("isGlobal", &CAstScope::IsGlobal)
    .add_property("isCatch", &CAstScope::IsCatch)
    .add_property("isBlock", &CAstScope::IsBlock)
    .add_property("isWith", &CAstScope::IsWith)
    .add_property("isDeclaration", &CAstScope::IsDeclaration)
    .add_property("isStrictOrExtendedEval", &CAstScope::IsStrictOrExtendedEval)
    .add_property("isClassicMode", &CAstScope::IsClassicMode)
    .add_property("isExtendedMode", &CAstScope::IsExtendedMode)

    .add_property("callsEval", &CAstScope::CallsEval)
    .add_property("outerScopeCallsEval", &CAstScope::OuterScopeCallsEval)

    .add_property("insideWith", &CAstScope::InsideWith)
    .add_property("containsWith", &CAstScope::ContainsWith)

    .add_property("outer", &CAstScope::GetOuter)

    .add_property("declarations", &CAstScope::GetDeclarations)

    .add_property("function", &CAstScope::GetFunction)
    .add_property("num_parameters", &CAstScope::GetParametersNumer)
    .def("parameter", &CAstScope::GetParameter, (py::args("index")))
    ;

  py::enum_<v8i::VariableMode>("AstVariableMode")
    .value("var", v8i::VAR)
    .value("const", v8i::CONST)
    .value("let", v8i::LET)
    .value("dynamic", v8i::DYNAMIC)
    .value("global", v8i::DYNAMIC_GLOBAL)
    .value("local", v8i::DYNAMIC_LOCAL)
    .value("internal", v8i::INTERNAL)
    .value("temporary", v8i::TEMPORARY)
    ;

  py::enum_<v8i::Variable::Location>("AstVariableLocation")
    .value("UNALLOCATED", v8i::Variable::UNALLOCATED)
    .value("PARAMETER", v8i::Variable::PARAMETER)
    .value("LOCAL", v8i::Variable::LOCAL)
    .value("CONTEXT", v8i::Variable::CONTEXT)
    .value("LOOKUP", v8i::Variable::LOOKUP)
    ;

  py::class_<CAstVariable>("AstVariable", py::no_init)
    .add_property("scope", &CAstVariable::scope)
    .add_property("name", &CAstVariable::name)
    .add_property("mode", &CAstVariable::mode)
    .add_property("isValidLeftHandSide", &CAstVariable::IsValidLeftHandSide)
    .add_property("isThis", &CAstVariable::is_this)
    .add_property("isArguments", &CAstVariable::is_arguments)
    .add_property("isPossiblyEval", &CAstVariable::is_possibly_eval)
    .add_property("location", &CAstVariable::location)
    .add_property("index", &CAstVariable::index)
    ;

  py::class_<CAstLabel>("AstLabel", py::no_init)
    .add_property("pos", &CAstLabel::GetPosition)
    .add_property("bound", &CAstLabel::IsBound)
    .add_property("unused", &CAstLabel::IsUnused)
    .add_property("linked", &CAstLabel::IsLinked)
    ;

  #define DECLARE_TYPE_ENUM(type) .value(#type, v8i::AstNode::k##type)

  py::enum_<v8i::AstNode::NodeType>("AstNodeType")
    AST_NODE_LIST(DECLARE_TYPE_ENUM)

    .value("Invalid", v8i::AstNode::kInvalid)
    ;

  py::class_<CAstNode, boost::noncopyable>("AstNode", py::no_init)
    .add_property("type", &CAstNode::GetType)

    .add_property("pos", &CAstStatement::GetPosition)

    .def("visit", &CAstNode::Visit, (py::arg("handler")))

    .def("__str__", &CAstNode::ToString)
    ;

  py::class_<CAstStatement, py::bases<CAstNode> >("AstStatement", py::no_init)
    .def("__nonzero__", &CAstStatement::operator bool)
    ;

  py::class_<CAstExpression, py::bases<CAstNode> >("AstExpression", py::no_init)
    .add_property("isPropertyName", &CAstExpression::IsPropertyName)
    .add_property("isSmi", &CAstExpression::IsSmiLiteral)
    .add_property("isString", &CAstExpression::IsStringLiteral)
    .add_property("isNull", &CAstExpression::IsNullLiteral)
    ;

  py::class_<CAstBreakableStatement, py::bases<CAstStatement> >("AstBreakableStatement", py::no_init)
    .add_property("anonymous", &CAstBreakableStatement::IsTargetForAnonymous)
    .add_property("breakTarget", &CAstBreakableStatement::GetBreakTarget)
    ;

  py::class_<CAstBlock, py::bases<CAstBreakableStatement> >("AstBlock", py::no_init)
    .add_property("statements", &CAstBlock::GetStatements)
    .add_property("initializerBlock", &CAstBlock::IsInitializerBlock)

    .def("addStatement", &CAstBlock::AddStatement)
    ;

  py::class_<CAstDeclaration, py::bases<CAstNode> >("AstDeclaration", py::no_init)
    .add_property("proxy", &CAstDeclaration::GetProxy)
    .add_property("mode", &CAstDeclaration::GetMode)
    .add_property("scope", &CAstDeclaration::GetScope)
    ;

  py::class_<CAstVariableDeclaration, py::bases<CAstDeclaration> >("AstVariableDeclaration", py::no_init)
    ;

  py::class_<CAstFunctionDeclaration, py::bases<CAstDeclaration> >("AstFunctionDeclaration", py::no_init)
	  .add_property("function", &CAstFunctionDeclaration::GetFunction)
	  ;

  py::class_<CAstModule, py::bases<CAstNode> >("AstModule", py::no_init)
    ;

  py::class_<CAstModuleDeclaration, py::bases<CAstDeclaration> >("AstModuleDeclaration", py::no_init)
    .add_property("module", &CAstModuleDeclaration::GetModule)
    ;

  py::class_<CAstImportDeclaration, py::bases<CAstDeclaration> >("AstImportDeclaration", py::no_init)
	  .add_property("module", &CAstImportDeclaration::GetModule)
	  ;

  py::class_<CAstExportDeclaration, py::bases<CAstDeclaration> >("AstExportDeclaration", py::no_init)
	  ;

  py::class_<CAstModuleLiteral, py::bases<CAstModule> >("AstModuleLiteral", py::no_init)
    .add_property("body", &CAstModuleLiteral::GetBody)
    ;

  py::class_<CAstModuleVariable, py::bases<CAstModule> >("AstModuleVariable", py::no_init)
    .add_property("proxy", &CAstModuleVariable::GetProxy)
    ;

  py::class_<CAstModulePath, py::bases<CAstModule> >("AstModulePath", py::no_init)
    .add_property("module", &CAstModulePath::GetModule)
    .add_property("name", &CAstModulePath::GetName)
    ;

  py::class_<CAstIterationStatement, py::bases<CAstBreakableStatement> >("AstIterationStatement", py::no_init)
    .add_property("body", &CAstIterationStatement::GetBody)
    .add_property("continueTarget", &CAstIterationStatement::GetContinueTarget)
    ;

  py::class_<CAstDoWhileStatement, py::bases<CAstIterationStatement> >("AstDoWhileStatement", py::no_init)
    .add_property("condition", &CAstDoWhileStatement::GetCondition)
    ;

  py::class_<CAstWhileStatement, py::bases<CAstIterationStatement> >("AstWhileStatement", py::no_init)
    .add_property("condition", &CAstWhileStatement::GetCondition)
    ;

  py::class_<CAstForStatement, py::bases<CAstIterationStatement> >("AstForStatement", py::no_init)
    .add_property("init", &CAstForStatement::GetInit)
    .add_property("condition", &CAstForStatement::GetCondition)
    .add_property("nextStmt", &CAstForStatement::GetNext)
    .add_property("fastLoop", &CAstForStatement::IsFastLoop)
    ;

  py::class_<CAstForEachStatement, py::bases<CAstIterationStatement> >("AstForEachStatement", py::no_init)
    .add_property("each", &CAstForEachStatement::GetEach)
    .add_property("subject", &CAstForEachStatement::GetSubject)
    ;

  py::class_<CAstForInStatement, py::bases<CAstForEachStatement> >("AstForInStatement", py::no_init)
    .add_property("enumerable", &CAstForInStatement::GetEnumerable)
    ;

  py::class_<CAstForOfStatement, py::bases<CAstForEachStatement> >("AstForOfStatement", py::no_init)
    .add_property("iterable", &CAstForOfStatement::GetIterable)
    .add_property("assignIterator", &CAstForOfStatement::AssignIterator)
    .add_property("next", &CAstForOfStatement::NextResult)
    .add_property("done", &CAstForOfStatement::ResultDone)
    .add_property("assignEach", &CAstForOfStatement::AssignEach)
    ;

  py::class_<CAstExpressionStatement, py::bases<CAstStatement> >("AstExpressionStatement", py::no_init)
    .add_property("expression", &CAstExpressionStatement::GetExpression)
    ;

  py::class_<CAstContinueStatement, py::bases<CAstStatement> >("AstContinueStatement", py::no_init)
    .add_property("target", &CAstContinueStatement::GetTarget)
    ;

  py::class_<CAstBreakStatement, py::bases<CAstStatement> >("AstBreakStatement", py::no_init)
    .add_property("target", &CAstBreakStatement::GetTarget)
    ;

  py::class_<CAstReturnStatement, py::bases<CAstStatement> >("AstReturnStatement", py::no_init)
    .add_property("expression", &CAstReturnStatement::expression)
    ;

  py::class_<CAstWithStatement, py::bases<CAstStatement> >("AstWithStatement", py::no_init)
    .add_property("expression", &CAstWithStatement::expression)
    .add_property("statement", &CAstWithStatement::statement)
    ;

  py::class_<CAstCaseClause, py::bases<CAstNode> >("AstCaseClause", py::no_init)
    .add_property("isDefault", &CAstCaseClause::is_default)
    .add_property("label", &CAstCaseClause::label)
    .add_property("bodyTarget", &CAstCaseClause::body_target)
    .add_property("statements", &CAstCaseClause::statements)
    ;

  py::class_<CAstSwitchStatement, py::bases<CAstBreakableStatement> >("AstSwitchStatement", py::no_init)
    .add_property("tag", &CAstSwitchStatement::tag)
    .add_property("cases", &CAstSwitchStatement::cases)
    ;

  py::class_<CAstIfStatement, py::bases<CAstStatement> >("AstIfStatement", py::no_init)
    .add_property("hasThenStatement", &CAstIfStatement::HasThenStatement)
    .add_property("hasElseStatement", &CAstIfStatement::HasElseStatement)

    .add_property("condition", &CAstIfStatement::GetCondition)

    .add_property("thenStatement", &CAstIfStatement::GetThenStatement)
    .add_property("elseStatement", &CAstIfStatement::GetElseStatement)
    ;

  py::class_<CAstTargetCollector, py::bases<CAstNode> >("AstTargetCollector", py::no_init)
    .add_property("targets", &CAstTargetCollector::GetTargets)
    ;

  py::class_<CAstTryStatement, py::bases<CAstStatement> >("AstTryStatement", py::no_init)
    .add_property("tryBlock", &CAstTryStatement::GetTryBlock)
    .add_property("targets", &CAstTryStatement::GetEscapingTargets)
    ;

  py::class_<CAstTryCatchStatement, py::bases<CAstTryStatement> >("AstTryCatchStatement", py::no_init)
    .add_property("scope", &CAstTryCatchStatement::GetScope)
    .add_property("variable", &CAstTryCatchStatement::GetVariable)
    .add_property("catchBlock", &CAstTryCatchStatement::GetCatchBlock)
    ;

  py::class_<CAstTryFinallyStatement, py::bases<CAstTryStatement> >("AstTryFinallyStatement", py::no_init)
    .add_property("finallyBlock", &CAstTryFinallyStatement::GetFinallyBlock)
    ;

  py::class_<CAstDebuggerStatement, py::bases<CAstStatement> >("AstDebuggerStatement", py::no_init)
    ;

  py::class_<CAstEmptyStatement, py::bases<CAstStatement> >("AstEmptyStatement", py::no_init)
    ;

  py::class_<CAstLiteral, py::bases<CAstExpression> >("AstLiteral", py::no_init)
    .add_property("isNull", &CAstLiteral::IsNull)
    .add_property("isTrue", &CAstLiteral::IsTrue)
    .add_property("isFalse", &CAstLiteral::IsFalse)
    .add_property("asPropertyName", &CAstLiteral::AsPropertyName)
    ;

  py::class_<CAstMaterializedLiteral, py::bases<CAstExpression> >("AstMaterializedLiteral", py::no_init)
    .add_property("index", &CAstMaterializedLiteral::GetIndex)
    ;

  py::enum_<v8i::ObjectLiteral::Property::Kind>("AstPropertyKind")
    .value("constant", v8i::ObjectLiteral::Property::CONSTANT)
    .value("computed", v8i::ObjectLiteral::Property::COMPUTED)
    .value("materialized", v8i::ObjectLiteral::Property::MATERIALIZED_LITERAL)
    .value("getter", v8i::ObjectLiteral::Property::GETTER)
    .value("setter", v8i::ObjectLiteral::Property::SETTER)
    .value("prototype", v8i::ObjectLiteral::Property::PROTOTYPE)
    ;

  py::class_<CAstObjectProperty>("AstObjectProperty", py::no_init)
    .add_property("key", &CAstObjectProperty::GetKey)
    .add_property("value", &CAstObjectProperty::GetValue)
    .add_property("kind", &CAstObjectProperty::GetKind)
    .add_property("isCompileTimeValue", &CAstObjectProperty::IsCompileTimeValue)
    ;

  py::class_<CAstObjectLiteral, py::bases<CAstMaterializedLiteral> >("AstObjectLiteral", py::no_init)
    .add_property("properties", &CAstObjectLiteral::GetProperties)
    ;

  py::class_<CAstRegExpLiteral, py::bases<CAstMaterializedLiteral> >("AstRegExpLiteral", py::no_init)
    .add_property("pattern", &CAstRegExpLiteral::GetPattern)
    .add_property("flags", &CAstRegExpLiteral::GetFlags)
    ;

  py::class_<CAstArrayLiteral, py::bases<CAstMaterializedLiteral> >("AstArrayLiteral", py::no_init)
    .add_property("values", &CAstArrayLiteral::GetValues)
    ;

  py::class_<CAstVariableProxy, py::bases<CAstExpression> >("AstVariableProxy", py::no_init)
    .add_property("isValidLeftHandSide", &CAstVariableProxy::IsValidLeftHandSide)
    .add_property("isArguments", &CAstVariableProxy::IsArguments)
    .add_property("name", &CAstVariableProxy::name)
    .add_property("var", &CAstVariableProxy::var)
    .add_property("isThis", &CAstVariableProxy::is_this)
    ;

  py::class_<CAstProperty, py::bases<CAstExpression> >("AstProperty", py::no_init)
    ;

  py::class_<CAstCall, py::bases<CAstExpression> >("AstCall", py::no_init)
    .add_property("expression", &CAstCall::GetExpression)
    .add_property("args", &CAstCall::GetArguments)
    ;

  py::class_<CAstCallNew, py::bases<CAstExpression> >("AstCallNew", py::no_init)
    .add_property("expression", &CAstCallNew::GetExpression)
    .add_property("args", &CAstCallNew::GetArguments)
    ;

  py::class_<CAstCallRuntime, py::bases<CAstExpression> >("AstCallRuntime", py::no_init)
    .add_property("name", &CAstCallRuntime::GetName)
    .add_property("args", &CAstCallRuntime::GetArguments)
    .add_property("isJsRuntime", &CAstCallRuntime::IsJSRuntime)
    ;

  py::enum_<v8i::Token::Value>("AstOperation")
  #define T(name, string, precedence) .value(#name, v8i::Token::name)
    TOKEN_LIST(T, IGNORE_TOKEN)
  #undef T
    ;

  py::class_<CAstUnaryOperation, py::bases<CAstExpression> >("AstUnaryOperation", py::no_init)
    .add_property("op", &CAstUnaryOperation::op)
    .add_property("expression", &CAstUnaryOperation::expression)
    ;

  py::class_<CAstBinaryOperation, py::bases<CAstExpression> >("AstBinaryOperation", py::no_init)
    .add_property("op", &CAstBinaryOperation::op)
    .add_property("left", &CAstBinaryOperation::left)
    .add_property("right", &CAstBinaryOperation::right)
    ;

  py::class_<CAstCountOperation, py::bases<CAstExpression> >("AstCountOperation", py::no_init)
    .add_property("prefix", &CAstCountOperation::is_prefix)
    .add_property("postfix", &CAstCountOperation::is_postfix)

    .add_property("op", &CAstCountOperation::op)
    .add_property("binop", &CAstCountOperation::binary_op)

    .add_property("expression", &CAstCountOperation::expression)
    ;

  py::class_<CAstCompareOperation, py::bases<CAstExpression> >("AstCompareOperation", py::no_init)
    .add_property("op", &CAstCompareOperation::op)
    .add_property("left", &CAstCompareOperation::left)
    .add_property("right", &CAstCompareOperation::right)
    ;

  py::class_<CAstConditional, py::bases<CAstExpression> >("AstConditional", py::no_init)
    .add_property("condition", &CAstConditional::condition)
    .add_property("thenExpr", &CAstConditional::then_expression)
    .add_property("elseExpr", &CAstConditional::else_expression)
    ;

  py::class_<CAstAssignment, py::bases<CAstExpression> >("AstAssignment", py::no_init)
    .add_property("op", &CAstAssignment::op)
    .add_property("binop", &CAstAssignment::binary_op)

    .add_property("target", &CAstAssignment::target)
    .add_property("value", &CAstAssignment::value)

    .add_property("binOperation", &CAstAssignment::binary_operation)

    .add_property("compound", &CAstAssignment::is_compound)
    ;

  py::enum_<v8i::Yield::Kind>("AstYieldKind")
	  .value("initial", v8i::Yield::INITIAL)
	  .value("suspend", v8i::Yield::SUSPEND)
	  .value("delegating", v8i::Yield::DELEGATING)
	  .value("final", v8i::Yield::FINAL)
	  ;

  py::class_<CAstYield, py::bases<CAstExpression> >("AstYield", py::no_init)
    .add_property("expression", &CAstYield::expression)
    .add_property("kind", &CAstYield::yield_kind)
    ;

  py::class_<CAstThrow, py::bases<CAstExpression> >("AstThrow", py::no_init)
    .add_property("exception", &CAstThrow::GetException)
    ;

  py::class_<CAstFunctionLiteral, py::bases<CAstExpression> >("AstFunctionLiteral", py::no_init)
    .add_property("name", &CAstFunctionLiteral::GetName)
    .add_property("scope", &CAstFunctionLiteral::GetScope)
    .add_property("body", &CAstFunctionLiteral::GetBody)

    .add_property("startPos", &CAstFunctionLiteral::GetStartPosition)
    .add_property("endPos", &CAstFunctionLiteral::GetEndPosition)
    .add_property("isExpression", &CAstFunctionLiteral::IsExpression)

    .def("toAST", &CAstFunctionLiteral::ToAST)
    .def("toJSON", &CAstFunctionLiteral::ToJSON)
    ;

  py::class_<CAstNativeFunctionLiteral, py::bases<CAstExpression> >("AstNativeFunctionLiteral", py::no_init)
    ;

  py::class_<CAstThisFunction, py::bases<CAstExpression> >("AstThisFunction", py::no_init)
    ;
}
