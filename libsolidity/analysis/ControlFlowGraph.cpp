/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libsolidity/analysis/ControlFlowGraph.h>

#include <algorithm>

using namespace std;
using namespace dev::solidity;

bool CFG::constructFlow(ASTNode const& _astRoot)
{
	_astRoot.accept(*this);
	return Error::containsOnlyWarnings(m_errorReporter.errors());
}

bool CFG::visit(BinaryOperation const& _operation)
{
	// binary operations can occur outside functions and modifiers,
	// e.g. in constant declarations
	if (m_currentNode)
	{
		switch(_operation.getOperator())
		{
			case Token::Or:
			case Token::And:
			{
				_operation.leftExpression().accept(*this);

				auto opRight = newNode();
				auto afterOp = newNode();

				addEdge(m_currentNode, afterOp);
				addEdge(m_currentNode, opRight);

				m_currentNode = opRight;
				_operation.rightExpression().accept(*this);
				addEdge(m_currentNode, afterOp);

				m_currentNode = afterOp;

				return false;
			}
			default:
				break;
		}
	}
	return ASTConstVisitor::visit(_operation);
}

bool CFG::visit(Conditional const& _conditional)
{
	// conditional expressions can occur outside functions and modifiers,
	// e.g. in constant declarations
	if (m_currentNode)
	{
		_conditional.condition().accept(*this);

		auto trueNode = newNode();
		auto falseNode = newNode();
		auto afterCond = newNode();

		addEdge(m_currentNode, trueNode);
		addEdge(m_currentNode, falseNode);

		m_currentNode = trueNode;
		_conditional.trueExpression().accept(*this);
		addEdge(m_currentNode, afterCond);

		m_currentNode = falseNode;
		_conditional.falseExpression().accept(*this);
		addEdge(m_currentNode, afterCond);

		m_currentNode = afterCond;
		return false;
	}
	else
		return ASTConstVisitor::visit(_conditional);
}

bool CFG::visit(ModifierDefinition const& _modifier)
{
	solAssert(!m_currentNode, "");
	solAssert(!m_currentFunctionFlow, "");
	m_currentModifierFlow = make_shared<ModifierFlow>(newNode(), newNode(), newNode());
	m_modifierControlFlow[&_modifier] = m_currentModifierFlow;
	m_currentNode = m_currentModifierFlow->entry;
	m_returnJump = m_currentModifierFlow->exit;
	m_exceptionJump = m_currentModifierFlow->exception;
	return true;
}

void CFG::endVisit(ModifierDefinition const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_currentModifierFlow, "");
	solAssert(m_currentModifierFlow->exit, "");
	addEdge(m_currentNode, m_currentModifierFlow->exit);
	m_currentNode = nullptr;
	m_returnJump = nullptr;
	m_exceptionJump = nullptr;
	m_currentModifierFlow.reset();
}

bool CFG::visit(FunctionDefinition const& _function)
{
	// TODO: insert modifiers into function control flow later

	solAssert(!m_currentNode, "");
	solAssert(!m_currentFunctionFlow, "");

	m_currentFunctionFlow = make_shared<FunctionFlow>(newNode(), newNode(), newNode());
	m_functionControlFlow[&_function] = m_currentFunctionFlow;

	m_currentNode = m_currentFunctionFlow->entry;
	m_returnJump = m_currentFunctionFlow->exit;
	m_exceptionJump = m_currentFunctionFlow->exception;
	return true;
}

void CFG::endVisit(FunctionDefinition const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_currentFunctionFlow, "");
	solAssert(m_currentFunctionFlow->entry, "");
	solAssert(m_currentFunctionFlow->exit, "");
	addEdge(m_currentNode, m_currentFunctionFlow->exit);

	m_currentNode = nullptr;
	m_returnJump = nullptr;
	m_exceptionJump = nullptr;
	m_currentFunctionFlow.reset();
}

bool CFG::visit(IfStatement const& _ifStatement)
{
	solAssert(!!m_currentNode, "");

	_ifStatement.condition().accept(*this);

	auto beforeBranch = m_currentNode;
	auto afterBranch = newNode();

	m_currentNode = newNode();
	addEdge(beforeBranch, m_currentNode);
	_ifStatement.trueStatement().accept(*this);
	addEdge(m_currentNode, afterBranch);

	if (_ifStatement.falseStatement())
	{
		m_currentNode = newNode();
		addEdge(beforeBranch, m_currentNode);
		_ifStatement.falseStatement()->accept(*this);
		addEdge(m_currentNode, afterBranch);
	}
	else
		addEdge(beforeBranch, afterBranch);


	m_currentNode = afterBranch;

	return false;
}

bool CFG::visit(ForStatement const& _forStatement)
{
	solAssert(!!m_currentNode, "");

	if (auto initializationExpression = _forStatement.initializationExpression())
		initializationExpression->accept(*this);

	auto condition = newNode();
	auto body = newNode();
	auto loopExpression = newNode();
	auto afterFor = newNode();

	addEdge(m_currentNode, condition);
	m_currentNode = condition;

	if (auto conditionExpression = _forStatement.condition())
		conditionExpression->accept(*this);

	addEdge(m_currentNode, body);
	addEdge(m_currentNode, afterFor);

	m_currentNode = body;
	m_breakJumps.push(afterFor);
	m_continueJumps.push(loopExpression);
	_forStatement.body().accept(*this);
	m_breakJumps.pop();
	m_continueJumps.pop();

	addEdge(m_currentNode, loopExpression);
	m_currentNode = loopExpression;

	if (auto expression = _forStatement.loopExpression())
		expression->accept(*this);

	addEdge(m_currentNode, condition);

	m_currentNode = afterFor;

	return false;
}

bool CFG::visit(WhileStatement const& _whileStatement)
{
	solAssert(!!m_currentNode, "");

	auto afterWhile = newNode();
	if (_whileStatement.isDoWhile())
	{
		auto whileBody = newNode();

		addEdge(m_currentNode, whileBody);
		m_currentNode = whileBody;

		m_continueJumps.push(whileBody);
		m_breakJumps.push(afterWhile);
		_whileStatement.body().accept(*this);
		m_breakJumps.pop();
		m_continueJumps.pop();
		_whileStatement.condition().accept(*this);

		addEdge(m_currentNode, afterWhile);
		addEdge(m_currentNode, whileBody);
	}
	else
	{
		auto whileCondition = newNode();
		addEdge(m_currentNode, whileCondition);
		_whileStatement.condition().accept(*this);
		auto whileBody = newNode();
		addEdge(m_currentNode, whileBody);
		addEdge(m_currentNode, afterWhile);

		m_currentNode = whileBody;
		m_breakJumps.push(afterWhile);
		m_continueJumps.push(whileCondition);
		_whileStatement.body().accept(*this);
		m_breakJumps.pop();
		m_continueJumps.pop();

		addEdge(m_currentNode, whileCondition);

	}

	m_currentNode = afterWhile;

	return false;
}

bool CFG::visit(Break const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!m_breakJumps.empty(), "");
	addEdge(m_currentNode, m_breakJumps.top());
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Continue const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!m_continueJumps.empty(), "");
	addEdge(m_currentNode, m_continueJumps.top());
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Throw const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(m_exceptionJump, "");
	addEdge(m_currentNode, m_exceptionJump);
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Block const&)
{
	solAssert(!!m_currentNode, "");
	auto beforeBlock = m_currentNode;
	m_currentNode = newNode();
	addEdge(beforeBlock, m_currentNode);
	return true;
}

void CFG::endVisit(Block const&)
{
	solAssert(!!m_currentNode, "");
	auto blockEnd = m_currentNode;
	m_currentNode = newNode();
	addEdge(blockEnd, m_currentNode);
}

bool CFG::visit(Return const& _return)
{
	solAssert(m_currentNode, "");
	solAssert(m_returnJump, "");
	addEdge(m_currentNode, m_returnJump);
	// only the first return statement is interesting
	if (!m_currentNode->block.returnStatement)
		m_currentNode->block.returnStatement = &_return;
	m_currentNode = newNode();
	return true;
}


bool CFG::visit(PlaceholderStatement const&)
{
	solAssert(m_currentModifierFlow, "");
	auto placeholderEntry = newNode();
	auto placeholderExit = newNode();

	addEdge(m_currentNode, placeholderEntry);

	m_currentModifierFlow->placeholders.emplace_back(placeholderEntry, placeholderExit);

	m_currentNode = placeholderExit;
	return false;
}

FunctionFlow const &CFG::functionFlow(FunctionDefinition const &_function) const
{
	solAssert(m_functionControlFlow.count(&_function), "");
	return *m_functionControlFlow.find(&_function)->second;
}

bool CFG::visitNode(ASTNode const& node)
{
	if (m_currentNode)
	{
		if (auto const* expression = dynamic_cast<Expression const*>(&node))
			m_currentNode->block.expressions.emplace_back(expression);
		if (auto const* variableDeclaration = dynamic_cast<VariableDeclaration const*>(&node))
			m_currentNode->block.variableDeclarations.emplace_back(variableDeclaration);
		if (auto const* assembly = dynamic_cast<InlineAssembly const*>(&node))
			m_currentNode->block.inlineAssemblyStatements.emplace_back(assembly);

	}
	return true;
}

CFGNode* CFG::newNode()
{
	m_nodes.emplace_back(new CFGNode());
	return m_nodes.back().get();
}

void CFG::addEdge(CFGNode* _from, CFGNode* _to)
{
	solAssert(_from, "");
	solAssert(_to, "");
	_from->exits.push_back(_to);
	_to->entries.push_back(_from);
}

bool CFG::visit(FunctionCall const& _functionCall)
{
	if (m_currentNode)
	{
		if (auto functionType = dynamic_pointer_cast<FunctionType const>(_functionCall.expression().annotation().type))
			switch (functionType->kind())
			{
				case FunctionType::Kind::Revert:
					solAssert(m_exceptionJump, "");
					ASTNode::listAccept(_functionCall.arguments(), *this);
					addEdge(m_currentNode, m_exceptionJump);
					m_currentNode = newNode();
					return false;
				case FunctionType::Kind::Require:
				case FunctionType::Kind::Assert:
				{
					solAssert(m_exceptionJump, "");
					ASTNode::listAccept(_functionCall.arguments(), *this);
					addEdge(m_currentNode, m_exceptionJump);
					auto nextNode = newNode();
					addEdge(m_currentNode, nextNode);
					m_currentNode = nextNode;
					return false;
				}
				default:
					break;
			}
	}
	return ASTConstVisitor::visit(_functionCall);
}
