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

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/interface/ErrorReporter.h>

#include <map>
#include <memory>
#include <stack>
#include <vector>

namespace dev
{
namespace solidity
{

/// Basic Control Flow Block.
/// Basic block of control flow. Consists of a set of AST nodes
/// for which control flow is always linear.
struct ControlFlowBlock
{
	/// All variable declarations executed in this control flow block.
	std::vector<VariableDeclaration const*> variableDeclarations;
	/// All expressions executed in this control flow block (this includes all subexpressions!).
	std::vector<Expression const*> expressions;
	/// All inline assembly statements executed in this control flow block.
	std::vector<InlineAssembly const*> inlineAssemblyStatements;
	/// If control flow returns in this node, the return statement is stored in returnStatement,
	/// otherwise returnStatement is nullptr.
	Return const* returnStatement = nullptr;
};

/// Node of the Control Flow Graph.
/// The control flow is a directed graph connecting control flow blocks.
/// An arc between two nodes indicates that the control flow can possibly
/// move from its start node to its end node during execution.
struct CFGNode
{
	/// Entry nodes. All CFG nodes from which control flow may move into this node.
	std::vector<CFGNode*> entries;
	/// Exit nodes. All CFG nodes to which control flow may continue after this node.
	std::vector<CFGNode*> exits;

	/// Control flow in the node.
	ControlFlowBlock block;
};

/// Describes the control flow of a function.
struct FunctionFlow
{
	FunctionFlow(CFGNode* _entry, CFGNode* _exit, CFGNode* _exception):
		entry(_entry), exit(_exit), exception(_exception) {}
	/// Entry node. Control flow of the function starts here.
	/// This node does not have any entries.
	CFGNode* entry;
	/// Exit node. Control flow of the function ends here.
	/// This node does not have any exits, but may have multiple entries
	/// (e.g. all return statements of the function).
	CFGNode* exit;
	/// Exception node. Control flow of the function in case of revert.
	/// This node does not have any exists, but may have multiple entries
	/// (e.g. all revert, assert and require statements).
	CFGNode* exception;
};

/// Describes the control flow of a modifier.
/// Inherits all members from FunctionFlow.
struct ModifierFlow : FunctionFlow
{
	template<typename... Args>
	ModifierFlow(Args&&... args): FunctionFlow(std::forward<Args>(args)...) {}
	/// Placeholder cuts. List of pairs of disconnected CFGNode's
	/// indicating the location of a placeholder.
	/// E.g. the control flow of a function with a single modifier
	/// is the control flow of the modifier in which the first node
	/// of each placeholder is connected to the function's entry node
	/// and the second node of each placeholder to the function's exit node.
	std::vector<std::pair<CFGNode*, CFGNode*>> placeholders;
};

class CFG: private ASTConstVisitor
{
public:
	explicit CFG(ErrorReporter& _errorReporter): m_errorReporter(_errorReporter) {}

	bool constructFlow(ASTNode const& _astRoot);

	virtual bool visit(ModifierDefinition const& _modifier) override;
	virtual bool visit(FunctionDefinition const& _function) override;

	FunctionFlow const& functionFlow(FunctionDefinition const& _function) const;
private:
	CFGNode* newNode();

	ErrorReporter& m_errorReporter;

	/// Maps function definitions to their control flow.
	std::map<FunctionDefinition const*, std::shared_ptr<FunctionFlow>> m_functionControlFlow;
	/// Maps modifier definitions to their control flow.
	std::map<ModifierDefinition const*, std::shared_ptr<ModifierFlow>> m_modifierControlFlow;

	/// List of nodes.
	/// All nodes allocated during the construction of the control flow graph
	/// are owned by the CFG class and stored here.
	std::vector<std::unique_ptr<CFGNode>> m_nodes;

	/// The control flow of the function that is currently parsed.
	/// Note: this can also be a ModifierFlow
	std::shared_ptr<FunctionFlow> m_currentFunctionFlow;
	/// The control flow of the modifier that is currently parsed.
	std::shared_ptr<ModifierFlow> m_currentModifierFlow;

	friend class ControlFlowParser;
};

}
}
