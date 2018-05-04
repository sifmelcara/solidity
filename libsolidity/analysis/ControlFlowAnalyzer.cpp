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

#include <libsolidity/analysis/ControlFlowAnalyzer.h>

using namespace std;
using namespace dev::solidity;

bool ControlFlowAnalyzer::analyze(ASTNode const& _astRoot)
{
	_astRoot.accept(*this);
	return Error::containsOnlyWarnings(m_errorReporter.errors());
}

bool ControlFlowAnalyzer::visit(FunctionDefinition const& _function)
{
	auto const& functionFlow = m_cfg.functionFlow(_function);
	checkUnassignedStorageReturnValues(_function, functionFlow.entry, functionFlow.exit);
	return ASTConstVisitor::visit(_function);
}

void ControlFlowAnalyzer::checkUnassignedStorageReturnValues(
	FunctionDefinition const& _function,
	CFGNode const* _functionEntry,
	CFGNode const* _functionExit
) const
{
	if (_function.returnParameterList()->parameters().empty())
		return;

	map<CFGNode const*, set<VariableDeclaration const*>> unassigned;

	{
		auto& unassignedAtFunctionEntry = unassigned[_functionEntry];
		for (auto const& returnParameter: _function.returnParameterList()->parameters())
			if (returnParameter->type()->dataStoredIn(DataLocation::Storage))
				unassignedAtFunctionEntry.insert(returnParameter.get());
	}

	stack<CFGNode const*> nodesToTraverse;
	nodesToTraverse.push(_functionEntry);

	// walk all paths from entry with maximal set of unassigned return values
	while (!nodesToTraverse.empty())
	{
		auto node = nodesToTraverse.top();
		nodesToTraverse.pop();

		auto& unassignedAtNode = unassigned[node];

		if (node->block.returnStatement != nullptr)
			if (node->block.returnStatement->expression())
				unassignedAtNode.clear();
		if (!unassignedAtNode.empty())
		{
			// kill all return values to which a value is assigned
			for (auto expression: node->block.expressions)
			{
				if (auto const* assignment = dynamic_cast<Assignment const*>(expression))
				{
					stack<Expression const*> expressions;
					expressions.push(&assignment->leftHandSide());
					while (!expressions.empty())
					{
						Expression const* expression = expressions.top();
						expressions.pop();

						if (auto const *tuple = dynamic_cast<TupleExpression const*>(expression))
							for (auto const& component: tuple->components())
								expressions.push(component.get());
						else if (auto const* identifier = dynamic_cast<Identifier const*>(expression))
							if (auto const* variableDeclaration = dynamic_cast<VariableDeclaration const*>(
								identifier->annotation().referencedDeclaration
							))
								unassignedAtNode.erase(variableDeclaration);
					}
				}
			}
			// kill all return values referenced in inline assembly
			// a reference is enough, checking whether there actually was an assignment might be overkill
			for (auto assembly: node->block.inlineAssemblyStatements)
				for (auto const& ref: assembly->annotation().externalReferences)
					if(auto variableDeclaration = dynamic_cast<VariableDeclaration const*>(ref.second.declaration))
						unassignedAtNode.erase(variableDeclaration);
		}

		for (auto const& exit: node->exits)
		{
			auto& unassignedAtExit = unassigned[exit];
			auto oldSize = unassignedAtExit.size();
			unassignedAtExit.insert(unassignedAtNode.begin(), unassignedAtNode.end());
			// traverse an exit, if we are on a path with new unassigned return values to consider
			// this will terminate, since there is only a finite number of unassigned return values
			if (unassignedAtExit.size() > oldSize)
				nodesToTraverse.push(exit);
		}
	}

	if (unassigned[_functionExit].size() > 0)
	{
		vector<VariableDeclaration const*> unassignedOrdered(
			unassigned[_functionExit].begin(),
			unassigned[_functionExit].end()
		);
		sort(
			unassignedOrdered.begin(),
			unassignedOrdered.end(),
			[](VariableDeclaration const* lhs, VariableDeclaration const* rhs) -> bool {
				return lhs->id() < rhs->id();
			}
		);
		for (auto returnVal: unassignedOrdered)
			m_errorReporter.warning(returnVal->location(), "uninitialized storage pointer may be returned");
	}
}
