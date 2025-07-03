/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/

#ifndef CHANGEVALUECOMMAND_HPP
#define CHANGEVALUECOMMAND_HPP

// Qt
#include <QUndoCommand>
#include <QModelIndex>
#include <LSARecord.hpp>

#include<GuiModelItem.hpp>

class GuiModel;

class ChangeValueCommand : public QUndoCommand
{
public:
  ChangeValueCommand(const QPersistentModelIndex& changeIndex,
                     const QList<QPersistentModelIndex>& selectedIndices,
                     std::string oldValue,
                     std::string newValue,
                     bool isDerivativeChange,
                     GuiModel *model);

  void redo() Q_DECL_OVERRIDE;
  void undo() Q_DECL_OVERRIDE;

private:
  GuiModel *m_model; // Pointer to the gui model
  LSARecord *m_record; // Poinnter to the LSA record being edited
  std::string m_new, m_old; // LSA-formatted strings from before and after the change
  QList<QPersistentModelIndex> m_selectedIndices; // List of all indicies changed in one action (multi-edit)
  LSAModifier *m_modifier; // LSA modifier string
  bool m_redoAction, m_result, m_derivativeChange; // Flags that prevent doing changes twice

  // Structures that map the changedIndex->parent->root link
  QVector<IndexRowColumnParentChainItem> m_parentChain;
  QVector<QVector<IndexRowColumnParentChainItem>> m_selectedIndicesChain;

};

#endif // CHANGEVALUECOMMAND_HPP
