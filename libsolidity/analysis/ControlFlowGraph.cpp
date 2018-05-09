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

#include <boost/range/adaptor/reversed.hpp>

#include <algorithm>

using namespace std;
using namespace dev::solidity;

namespace dev
{
namespace solidity
{

/** Helper class that parses the control flow of a non-top-level ASTNode. */
class ControlFlowParser: private ASTConstVisitor
{
public:
	/// Connects @a _entry and @a _exit by inserting the control flow of @a _node in between.
	static void createFlowFromTo(CFG& _cfg, CFGNode* _entry, CFGNode* _exit, ASTNode const& _node)
	{
		ControlFlowParser parser(_cfg, _entry);
		parser.appendControlFlow(_node);
		connect(parser.m_currentNode, _exit);
	}

private:
	explicit ControlFlowParser(CFG& _cfg, CFGNode* _entry):
		m_cfg(_cfg), m_currentNode(_entry) {}

	virtual bool visit(BinaryOperation const& _operation) override;
	virtual bool visit(Conditional const& _conditional) override;
	virtual bool visit(IfStatement const& _ifStatement) override;
	virtual bool visit(ForStatement const& _forStatement) override;
	virtual bool visit(WhileStatement const& _whileStatement) override;
	virtual bool visit(Break const&) override;
	virtual bool visit(Continue const&) override;
	virtual bool visit(Throw const&) override;
	virtual bool visit(Block const&) override;
	virtual void endVisit(Block const&) override;
	virtual bool visit(Return const& _return) override;
	virtual bool visit(PlaceholderStatement const&) override;
	virtual bool visit(FunctionCall const& _functionCall) override;


	/// Appends the control flow of @a _node to the current control flow.
	void appendControlFlow(ASTNode const& _node)
	{
		_node.accept(*this);
	}

	/// Starts at @a _entry and parses the control flow of @a _node.
	/// @returns The node at which the parsed control flow ends.
	/// m_currentNode is not affected (it is saved and restored).
	CFGNode* createFlow(CFGNode* _entry, ASTNode const& _node)
	{
		auto oldCurrentNode = m_currentNode;
		m_currentNode = _entry;
		appendControlFlow(_node);
		auto endNode = m_currentNode;
		m_currentNode = oldCurrentNode;
		return endNode;
	}

	/// Creates an arc from @a _from to @a _to.
	static void connect(CFGNode* _from, CFGNode* _to)
	{
		solAssert(_from, "");
		solAssert(_to, "");
		_from->exits.push_back(_to);
		_to->entries.push_back(_from);
	}


protected:
	virtual bool visitNode(ASTNode const& node) override;

private:

	/// Splits the control flow starting at the current node into n paths.
	/// m_currentNode is set to nullptr and has to be set manually or
	/// using mergeFlow later.
	template<size_t n>
	std::array<CFGNode*, n> splitFlow()
	{
		std::array<CFGNode*, n> result;
		for (auto& node: result)
		{
			node = m_cfg.newNode();
			connect(m_currentNode, node);
		}
		m_currentNode = nullptr;
		return result;
	}

	/// Merges the control flow of @a _nodes to @a _endNode.
	/// If @a _endNode is nullptr, a new node is creates and used as end node.
	/// Sets the merge destination as current node.
	/// Note: @a _endNode may be one of the nodes in @a _nodes.
	template<size_t n>
	void mergeFlow(std::array<CFGNode*, n> const& _nodes, CFGNode* _endNode = nullptr)
	{
		CFGNode* mergeDestination = (_endNode == nullptr) ? m_cfg.newNode() : _endNode;
		for (auto& node: _nodes)
			if (node != mergeDestination)
				connect(node, mergeDestination);
		m_currentNode = mergeDestination;
	}

	CFGNode* newLabel()
	{
		return m_cfg.newNode();
	}
	CFGNode* createLabelHere()
	{
		auto label = m_cfg.newNode();
		connect(m_currentNode, label);
		m_currentNode = label;
		return label;
	}
	void placeAndConnectLabel(CFGNode *_node)
	{
		connect(m_currentNode, _node);
		m_currentNode = _node;
	}

	CFG& m_cfg;
	CFGNode* m_currentNode = nullptr;

	/// The current jump destination of break Statements.
	CFGNode* m_breakJump = nullptr;
	/// The current jump destination of continue Statements.
	CFGNode* m_continueJump = nullptr;

	/// Helper class that replaces the break and continue jump destinations for the
	/// current scope and restores the originals at the end of the scope.
	class BreakContinueScope
	{
	public:
		BreakContinueScope(ControlFlowParser& _parser, CFGNode* _breakJump, CFGNode* _continueJump):
			m_parser(_parser), m_origBreakJump(_parser.m_breakJump), m_origContinueJump(_parser.m_continueJump)
		{
			m_parser.m_breakJump = _breakJump;
			m_parser.m_continueJump = _continueJump;
		}
		~BreakContinueScope()
		{
			m_parser.m_breakJump = m_origBreakJump;
			m_parser.m_continueJump = m_origContinueJump;
		}
	private:
		ControlFlowParser& m_parser;
		CFGNode* m_origBreakJump;
		CFGNode* m_origContinueJump;
	};

	friend class BreakContinueScope;
};

}
}

bool ControlFlowParser::visit(BinaryOperation const& _operation)
{
	solAssert(!!m_currentNode, "");

	switch(_operation.getOperator())
	{
		case Token::Or:
		case Token::And:
		{
			appendControlFlow(_operation.leftExpression());

			auto nodes = splitFlow<2>();
			nodes[0] = createFlow(nodes[0], _operation.rightExpression());
			mergeFlow(nodes, nodes[1]);

			return false;
		}
		default:
			break;
	}
	return ASTConstVisitor::visit(_operation);
}

bool ControlFlowParser::visit(Conditional const& _conditional)
{
	solAssert(!!m_currentNode, "");

	_conditional.condition().accept(*this);

	auto nodes = splitFlow<2>();

	nodes[0] = createFlow(nodes[0], _conditional.trueExpression());
	nodes[1] = createFlow(nodes[1], _conditional.falseExpression());

	mergeFlow(nodes);

	return false;
}

bool ControlFlowParser::visit(IfStatement const& _ifStatement)
{
	solAssert(!!m_currentNode, "");

	_ifStatement.condition().accept(*this);

	if (_ifStatement.falseStatement())
	{
		auto nodes = splitFlow<2>();
		nodes[0] = createFlow(nodes[0], _ifStatement.trueStatement());
		nodes[1] = createFlow(nodes[1], *_ifStatement.falseStatement());
		mergeFlow(nodes);
	}
	else
	{
		auto nodes = splitFlow<2>();
		nodes[0] = createFlow(nodes[0], _ifStatement.trueStatement());
		mergeFlow(nodes, nodes[1]);
	}

	return false;
}

bool ControlFlowParser::visit(ForStatement const& _forStatement)
{
	solAssert(!!m_currentNode, "");

	if (auto initializationExpression = _forStatement.initializationExpression())
		initializationExpression->accept(*this);

	auto condition = createLabelHere();

	if (auto conditionExpression = _forStatement.condition())
		appendControlFlow(*conditionExpression);

	auto loopExpression = newLabel();
	auto nodes = splitFlow<2>();
	auto& afterFor = nodes[1];
	m_currentNode = nodes[0];

	{
		BreakContinueScope scope(*this, afterFor, loopExpression);
		appendControlFlow(_forStatement.body());
	}

	placeAndConnectLabel(loopExpression);

	if (auto expression = _forStatement.loopExpression())
		appendControlFlow(*expression);

	connect(m_currentNode, condition);
	m_currentNode = afterFor;

	return false;
}

bool ControlFlowParser::visit(WhileStatement const& _whileStatement)
{
	solAssert(!!m_currentNode, "");

	if (_whileStatement.isDoWhile())
	{
		auto afterWhile = newLabel();
		auto whileBody = createLabelHere();

		{
			BreakContinueScope scope(*this, afterWhile, whileBody);
			appendControlFlow(_whileStatement.body());
		}
		appendControlFlow(_whileStatement.condition());

		connect(m_currentNode, whileBody);
		placeAndConnectLabel(afterWhile);
	}
	else
	{
		auto whileCondition = createLabelHere();

		appendControlFlow(_whileStatement.condition());

		auto nodes = splitFlow<2>();

		auto& whileBody = nodes[0];
		auto& afterWhile = nodes[1];

		m_currentNode = whileBody;
		{
			BreakContinueScope scope(*this, afterWhile, whileCondition);
			appendControlFlow(_whileStatement.body());
		}

		connect(m_currentNode, whileCondition);

		m_currentNode = afterWhile;
	}


	return false;
}

bool ControlFlowParser::visit(Break const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_breakJump, "");
	connect(m_currentNode, m_breakJump);
	m_currentNode = newLabel();
	return false;
}

bool ControlFlowParser::visit(Continue const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_continueJump, "");
	connect(m_currentNode, m_continueJump);
	m_currentNode = newLabel();
	return false;
}

bool ControlFlowParser::visit(Throw const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_cfg.m_currentFunctionFlow, "");
	solAssert(!!m_cfg.m_currentFunctionFlow->revert, "");
	connect(m_currentNode, m_cfg.m_currentFunctionFlow->revert);
	m_currentNode = newLabel();
	return false;
}

bool ControlFlowParser::visit(Block const&)
{
	solAssert(!!m_currentNode, "");
	createLabelHere();
	return true;
}

void ControlFlowParser::endVisit(Block const&)
{
	solAssert(!!m_currentNode, "");
	createLabelHere();
}

bool ControlFlowParser::visit(Return const& _return)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_cfg.m_currentFunctionFlow, "");
	solAssert(!!m_cfg.m_currentFunctionFlow->exit, "");
	// only the first return statement is interesting
	if (!m_currentNode->block.returnStatement)
		m_currentNode->block.returnStatement = &_return;
	connect(m_currentNode, m_cfg.m_currentFunctionFlow->exit);
	m_currentNode = newLabel();
	return true;
}


bool ControlFlowParser::visit(PlaceholderStatement const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_cfg.m_currentModifierFlow, "");

	connect(m_currentNode, m_cfg.m_currentModifierFlow->placeholderEntry);

	m_currentNode = newLabel();

	connect(m_cfg.m_currentModifierFlow->placeholderExit, m_currentNode);
	return false;
}

bool ControlFlowParser::visitNode(ASTNode const& node)
{
	solAssert(!!m_currentNode, "");
	if (auto const* expression = dynamic_cast<Expression const*>(&node))
		m_currentNode->block.expressions.emplace_back(expression);
	if (auto const* variableDeclaration = dynamic_cast<VariableDeclaration const*>(&node))
		m_currentNode->block.variableDeclarations.emplace_back(variableDeclaration);
	if (auto const* assembly = dynamic_cast<InlineAssembly const*>(&node))
		m_currentNode->block.inlineAssemblyStatements.emplace_back(assembly);

	return true;
}

bool ControlFlowParser::visit(FunctionCall const& _functionCall)
{
	solAssert(!!m_currentNode, "");
	if (auto functionType = dynamic_pointer_cast<FunctionType const>(_functionCall.expression().annotation().type))
		switch (functionType->kind())
		{
			case FunctionType::Kind::Revert:
				solAssert(!!m_cfg.m_currentFunctionFlow, "");
				solAssert(!!m_cfg.m_currentFunctionFlow->revert, "");
				ASTNode::listAccept(_functionCall.arguments(), *this);
				connect(m_currentNode, m_cfg.m_currentFunctionFlow->revert);
				m_currentNode = newLabel();
				return false;
			case FunctionType::Kind::Require:
			case FunctionType::Kind::Assert:
			{
				solAssert(!!m_cfg.m_currentFunctionFlow, "");
				solAssert(!!m_cfg.m_currentFunctionFlow->revert, "");
				ASTNode::listAccept(_functionCall.arguments(), *this);
				connect(m_currentNode, m_cfg.m_currentFunctionFlow->revert);
				auto nextNode = newLabel();
				connect(m_currentNode, nextNode);
				m_currentNode = nextNode;
				return false;
			}
			default:
				break;
		}
	return ASTConstVisitor::visit(_functionCall);
}

bool CFG::constructFlow(ASTNode const& _astRoot)
{
	_astRoot.accept(*this);
	applyModifiers();
	return Error::containsOnlyWarnings(m_errorReporter.errors());
}


bool CFG::visit(ModifierDefinition const& _modifier)
{
	m_currentModifierFlow = make_shared<ModifierFlow>(newNode(), newNode(), newNode());
	m_currentModifierFlow->placeholderEntry = newNode();
	m_currentModifierFlow->placeholderExit = newNode();
	m_currentFunctionFlow = std::static_pointer_cast<FunctionFlow>(m_currentModifierFlow);
	m_modifierControlFlow[&_modifier] = m_currentModifierFlow;

	ControlFlowParser::createFlowFromTo(*this, m_currentFunctionFlow->entry, m_currentFunctionFlow->exit, _modifier);

	m_currentModifierFlow.reset();
	m_currentFunctionFlow.reset();

	return false;
}

bool CFG::visit(FunctionDefinition const& _function)
{
	m_currentFunctionFlow = make_shared<FunctionFlow>(newNode(), newNode(), newNode());
	m_functionControlFlow[&_function] = m_currentFunctionFlow;

	ControlFlowParser::createFlowFromTo(*this, m_currentFunctionFlow->entry, m_currentFunctionFlow->exit, _function);

	m_currentFunctionFlow.reset();

	return false;
}

FunctionFlow const& CFG::functionFlow(FunctionDefinition const& _function) const
{
	solAssert(m_functionControlFlow.count(&_function), "");
	return *m_functionControlFlow.find(&_function)->second;
}

CFGNode* CFG::newNode()
{
	m_nodes.emplace_back(new CFGNode());
	return m_nodes.back().get();
}

void CFG::applyModifiers()
{
	for (auto const& function: m_functionControlFlow)
	{
		for (auto const& modifierInvocation: boost::adaptors::reverse(function.first->modifiers()))
		{
			if (auto modifierDefinition = dynamic_cast<ModifierDefinition const*>(
				modifierInvocation->name()->annotation().referencedDeclaration
			))
			{
				solAssert(m_modifierControlFlow.count(modifierDefinition), "");
				auto modifierFlow = m_modifierControlFlow[modifierDefinition];
				applyModifierFlowToFunctionFlow(*modifierFlow, function.second);
			}
		}
	}
}

void CFG::applyModifierFlowToFunctionFlow(
	ModifierFlow const& _modifierFlow,
	std::shared_ptr<FunctionFlow> _functionFlow
)
{
	map<CFGNode*, CFGNode*> oldToNew;

	// inherit the revert node of the function
	oldToNew[_modifierFlow.revert] = _functionFlow->revert;

	// replace the placeholder nodes by the function entry and exit
	oldToNew[_modifierFlow.placeholderEntry] = _functionFlow->entry;
	oldToNew[_modifierFlow.placeholderExit] = _functionFlow->exit;

	stack<CFGNode*> nodesToClone;
	nodesToClone.push(_modifierFlow.entry);

	// map the modifier entry to a new node that will become the new function entry
	oldToNew[_modifierFlow.entry] = newNode();

	while (!nodesToClone.empty())
	{
		auto srcNode = nodesToClone.top();
		nodesToClone.pop();

		solAssert(oldToNew.count(srcNode), "");

		auto dstNode = oldToNew[srcNode];

		dstNode->block = srcNode->block;
		for (auto& entry: srcNode->entries)
		{
			if (!oldToNew.count(entry))
			{
				oldToNew[entry] = newNode();
				nodesToClone.push(entry);
			}
			dstNode->entries.emplace_back(oldToNew[entry]);
		}
		for (auto& exit: srcNode->exits)
		{
			if (!oldToNew.count(exit))
			{
				oldToNew[exit] = newNode();
				nodesToClone.push(exit);
			}
			dstNode->exits.emplace_back(oldToNew[exit]);
		}
	}

	// if the modifier control flow never reached its exit node,
	// we need to create a new (disconnected) exit node now
	if (!oldToNew.count(_modifierFlow.exit))
		oldToNew[_modifierFlow.exit] = newNode();

	_functionFlow->entry = oldToNew[_modifierFlow.entry];
	_functionFlow->exit = oldToNew[_modifierFlow.exit];
}