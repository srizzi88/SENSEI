/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTableModelAdapter.h

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
 * @class   svtkQtTableModelAdapter
 * @brief   Adapts a table to a Qt item model.
 *
 *
 * svtkQtTableModelAdapter is a QAbstractItemModel with a svtkTable as its
 * underlying data model.
 *
 * @sa
 * svtkQtAbstractModelAdapter svtkQtTreeModelAdapter
 */

#ifndef svtkQtTableModelAdapter_h
#define svtkQtTableModelAdapter_h

#include "svtkConfigure.h"
#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkQtAbstractModelAdapter.h"
#include <QImage> // Needed for icon support

class svtkSelection;
class svtkTable;
class svtkVariant;

class QMimeData;

class SVTKGUISUPPORTQT_EXPORT svtkQtTableModelAdapter : public svtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  svtkQtTableModelAdapter(QObject* parent = nullptr);
  svtkQtTableModelAdapter(svtkTable* table, QObject* parent = nullptr);
  ~svtkQtTableModelAdapter() override;

  //@{
  /**
   * Set/Get the SVTK data object as input to this adapter
   */
  void SetSVTKDataObject(svtkDataObject* data) override;
  svtkDataObject* GetSVTKDataObject() const override;
  //@}

  //@{
  /**
   * Selection conversion from SVTK land to Qt land
   */
  svtkSelection* QModelIndexListToSVTKIndexSelection(const QModelIndexList qmil) const override;
  QItemSelection SVTKIndexSelectionToQItemSelection(svtkSelection* svtksel) const override;
  //@}

  void SetKeyColumnName(const char* name) override;
  void SetColorColumnName(const char* name) override;
  void SetIconIndexColumnName(const char* name);

  enum
  {
    HEADER = 0,
    ITEM = 1
  };

  enum
  {
    COLORS = 0,
    ICONS = 1,
    NONE = 2
  };

  /**
   * Specify how to color rows if colors are provided by SetColorColumnName().
   * Default is the vertical header.
   */
  void SetDecorationLocation(int s);

  /**
   * Specify how to color rows if colors are provided by SetColorColumnName().
   * Default is the vertical header.
   */
  void SetDecorationStrategy(int s);

  bool GetSplitMultiComponentColumns() const;
  void SetSplitMultiComponentColumns(bool value);

  //@{
  /**
   * Set up the model based on the current table.
   */
  void setTable(svtkTable* table);
  svtkTable* table() const { return this->Table; }
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  //@}

  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
    const QModelIndex& parent) override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  QStringList mimeTypes() const override;
  Qt::DropActions supportedDropActions() const override;

  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);

signals:
  void selectionDropped(svtkSelection*);

private:
  void getValue(int row, int column, svtkVariant& retVal) const;
  bool noTableCheck() const;
  void updateModelColumnHashTables();
  QVariant getColorIcon(int row) const;
  QVariant getIcon(int row) const;

  bool SplitMultiComponentColumns;
  svtkTable* Table;
  int DecorationLocation;
  int DecorationStrategy;
  QImage IconSheet;
  int IconSize[2];
  int IconSheetSize[2];
  int IconIndexColumn;

  class svtkInternal;
  svtkInternal* Internal;

  svtkQtTableModelAdapter(const svtkQtTableModelAdapter&) = delete;
  void operator=(const svtkQtTableModelAdapter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtTableModelAdapter.h
