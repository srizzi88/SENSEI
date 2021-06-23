/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTreeModelAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkQtTreeModelAdapter
 * @brief   Adapts a tree to a Qt item model.
 *
 *
 * svtkQtTreeModelAdapter is a QAbstractItemModel with a svtkTree as its
 * underlying data model.
 *
 * @sa
 * svtkQtAbstractModelAdapter svtkQtTableModelAdapter
 */

#ifndef svtkQtTreeModelAdapter_h
#define svtkQtTreeModelAdapter_h

#include "svtkGUISupportQtModule.h" // For export macro

#include "svtkQtAbstractModelAdapter.h"
#include "svtkType.h" // Needed for svtkIdType
#include <QHash>     // Needed for the decoration map
#include <QVector>   // Needed for the index map

class svtkSelection;
class svtkTree;
class svtkAdjacentVertexIterator;

class QMimeData;

class SVTKGUISUPPORTQT_EXPORT svtkQtTreeModelAdapter : public svtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  svtkQtTreeModelAdapter(QObject* parent = nullptr, svtkTree* tree = nullptr);
  ~svtkQtTreeModelAdapter() override;

  //@{
  /**
   * Set/Get the SVTK data object as input to this adapter
   */
  void SetSVTKDataObject(svtkDataObject* data) override;
  svtkDataObject* GetSVTKDataObject() const override;
  //@}

  /**
   * Get the stored SVTK data object modification time of when the
   * adaption to a Qt model was done. This is in general not the
   * same this as the data objects modification time. It is the mod
   * time of the object when it was placed into the Qt model adapter.
   * You can use this mtime as part of the checking to see whether
   * you need to update the adapter by call SetSVTKDataObject again. :)
   */
  svtkMTimeType GetSVTKDataObjectMTime() const;

  //@{
  /**
   * Selection conversion from SVTK land to Qt land
   */
  svtkSelection* QModelIndexListToSVTKIndexSelection(const QModelIndexList qmil) const override;
  QItemSelection SVTKIndexSelectionToQItemSelection(svtkSelection* svtksel) const override;
  //@}

  void SetKeyColumnName(const char* name) override;

  void SetColorColumnName(const char* name) override;

  /**
   * Set up the model based on the current tree.
   */
  void setTree(svtkTree* t);
  svtkTree* tree() const { return this->Tree; }

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  //@{
  /**
   * If drag/drop is enabled in the view, the model will package up the current
   * pedigreeid svtkSelection into a QMimeData when items are dragged.
   * Currently only leaves of the tree can be dragged.
   */
  Qt::DropActions supportedDragActions() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  QStringList mimeTypes() const override;
  //@}

protected:
  void treeModified();
  void GenerateSVTKIndexToQtModelIndex(svtkIdType svtk_index, QModelIndex qmodel_index);

  svtkTree* Tree;
  svtkAdjacentVertexIterator* ChildIterator;
  svtkMTimeType TreeMTime;
  QVector<QModelIndex> SVTKIndexToQtModelIndex;
  QHash<QModelIndex, QVariant> IndexToDecoration;

private:
  svtkQtTreeModelAdapter(const svtkQtTreeModelAdapter&) = delete;
  void operator=(const svtkQtTreeModelAdapter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtTreeModelAdapter.h
