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

#include "ChangeValueCommand.hpp"

// Project includes
#include "GuiModel.hpp"

ChangeValueCommand::ChangeValueCommand(const QPersistentModelIndex& changeIndex,
                                       const QList<QPersistentModelIndex>& selectedIndices,
                                       std::string oldValue,
                                       std::string newValue,
                                       bool isDerivativeChange,
                                       GuiModel *model) : m_model(model)
{
    m_result = false;
    m_redoAction = false;
    m_derivativeChange = isDerivativeChange;
    m_old = oldValue;
    m_new = newValue;
    m_selectedIndices = selectedIndices;

    // Create the structures that will map an index and it's parents all the way to the root index
    m_parentChain = m_model->buildSingleIndexChain(changeIndex);
    m_selectedIndicesChain = m_model->buildMultipleIndicesChain(m_selectedIndices);

    setText("ChangeValueCommand");
}

void ChangeValueCommand::redo()
{
    // The index can sometimes change due to insert/remove. This ensures the "same" record will be edited
    if (!m_parentChain[0].index.isValid())
    {
        // Refresh the index/parent->root chain
        m_model->refreshSingleIndexChain(m_parentChain);

        // Don't attempt to remove if index is stil invalid
        if (!m_parentChain[0].index.isValid())
        {
            this->setObsolete(true);
            return;
        }
    }

    // redoAction check is needed because when an undo/redo object is initialized, `redo()` is automatically called.
    // Since the actual change is accomplished in the code before the object is created, this would lead to the action
    // being performed twice and ruining the undo/redo ability.
    if(m_redoAction)
    {
        m_result = m_model->updateRecord(m_parentChain[0].index, m_new);
    }

    if (m_result)
    {
        // Sync the model and push appropriate signals
        m_model->refreshAfterUndoRedo(m_parentChain[0].index);

        // Keep the focus on the actual changed record (needed because of updating points/modifiers in tree)
        if(!m_derivativeChange)
        {
            if(m_selectedIndices.size() > 1)
            {
                QModelIndexList indicesToSelect;
                m_model->refreshMultipleIndicesChain(m_selectedIndicesChain);
                for(int i = 0; i < m_selectedIndices.size(); i++)
                {
                    if(!m_selectedIndices[i].isValid())
                    {
                        m_selectedIndices[i] = m_selectedIndicesChain[i][0].index;
                    }
                    indicesToSelect.append(m_selectedIndices[i]);
                }
                m_model->select(indicesToSelect);
            }
            else
            {
                m_model->select(m_parentChain[0].index);
            }
        }
        return;
    }

    m_redoAction = true;
    return;
}

void ChangeValueCommand::undo()
{
    // The index can sometimes change due to insert/remove. This ensures the "same" record will be edited
    if (!m_parentChain[0].index.isValid())
    {
        // Refresh the index/parent->root chain
        m_model->refreshSingleIndexChain(m_parentChain);

        // Don't attempt to remove if index is stil invalid
        if (!m_parentChain[0].index.isValid())
        {
            this->setObsolete(true);
            return;
        }

    }

    //bool result = item->setData(m_col, m_old) (if we ever refactor this will be the change)
    m_result = m_model->updateRecord(m_parentChain[0].index, m_old);

    if (m_result)
    {
        // Sync the model and push appropriate signals
        m_model->refreshAfterUndoRedo(m_parentChain[0].index);

        // Keep the focus on the actual changed record (needed because of updating points/modifiers in tree)
        if(!m_derivativeChange)
        {
            if(m_selectedIndices.size() > 1)
            {
                QModelIndexList indicesToSelect;
                m_model->refreshMultipleIndicesChain(m_selectedIndicesChain);
                for(int i = 0; i < m_selectedIndices.size(); i++)
                {
                    if(!m_selectedIndices[i].isValid())
                    {
                        m_selectedIndices[i] = m_selectedIndicesChain[i][0].index;
                    }
                    indicesToSelect.append(m_selectedIndices[i]);
                }
                m_model->select(indicesToSelect);
            }
            else
            {
                m_model->select(m_parentChain[0].index);
            }
        }
    }
}
