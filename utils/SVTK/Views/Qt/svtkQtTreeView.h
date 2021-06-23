/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTreeView.h

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
 * @class   svtkQtTreeView
 * @brief   A SVTK view based on a Qt tree view.
 *
 *
 * svtkQtTreeView is a SVTK view using an underlying QTreeView.
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for implementing
 * this class
 */

#ifndef svtkQtTreeView_h
#define svtkQtTreeView_h

#include "svtkQtView.h"
#include "svtkViewsQtModule.h" // For export macro

#include "svtkSmartPointer.h" // Needed for member variables
#include <QList>             // Needed for member variables
#include <QPointer>          // Needed for member variables

class QAbstractItemDelegate;
class QAbstractItemView;
class QFilterTreeProxyModel;
class QColumnView;
class QItemSelection;
class QModelIndex;
class QTreeView;
class svtkApplyColors;
class QVBoxLayout;
class svtkQtTreeModelAdapter;
class QItemSelectionModel;

class SVTKVIEWSQT_EXPORT svtkQtTreeView : public svtkQtView
{
  Q_OBJECT

signals:
  void expanded(const QModelIndex&);
  void collapsed(const QModelIndex&);
  void updatePreviewWidget(const QModelIndex&);

public:
  static svtkQtTreeView* New();
  svtkTypeMacro(svtkQtTreeView, svtkQtView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  QWidget* GetWidget() override;

  /**
   * Have the view show/hide its column headers (default is ON)
   */
  void SetShowHeaders(bool);

  /**
   * Have the view alternate its row colors (default is OFF)
   */
  void SetAlternatingRowColors(bool);

  /**
   * Have the view alternate its row colors (default is OFF)
   */
  void SetEnableDragDrop(bool);

  /**
   * Show the root node of the tree (default is OFF)
   */
  void SetShowRootNode(bool);

  /**
   * Hide the column of the given index from being shown in the view
   */
  void HideColumn(int i);

  /**
   * Show the column of the given index in the view
   */
  void ShowColumn(int i);

  /**
   * Hide all but the first column in the view
   */
  void HideAllButFirstColumn();

  /**
   * The column used to filter on
   */
  void SetFilterColumn(int i);

  /**
   * The column used to filter on
   */
  void SetFilterRegExp(const QRegExp& pattern);

  /**
   * The column used to filter on
   */
  void SetFilterTreeLevel(int level);

  /**
   * Collapses the model item specified by the index.
   */
  void Collapse(const QModelIndex& index);

  /**
   * Collapses all expanded items.
   */
  void CollapseAll();

  /**
   * Expands the model item specified by the index.
   */
  void Expand(const QModelIndex& index);

  /**
   * Expands all expandable items.
   * Warning: if the model contains a large number of items,
   * this function will take some time to execute.
   */
  void ExpandAll();

  /**
   * Expands all expandable items to the given depth.
   */
  void ExpandToDepth(int depth);

  /**
   * Resizes the column given to the size of its contents.
   */
  void ResizeColumnToContents(int column);

  /**
   * Set whether to use a QColumnView (QTreeView is the default)
   */
  void SetUseColumnView(int state);

  /**
   * Updates the view.
   */
  void Update() override;

  /**
   * Set item delegate to something custom
   */
  void SetItemDelegate(QAbstractItemDelegate* delegate);

  //@{
  /**
   * The array to use for coloring items in view.  Default is "color".
   */
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  svtkBooleanMacro(ColorByArray, bool);
  //@}

  void ApplyViewTheme(svtkViewTheme* theme) override;

protected:
  svtkQtTreeView();
  ~svtkQtTreeView() override;

  void AddRepresentationInternal(svtkDataRepresentation* rep) override;
  void RemoveRepresentationInternal(svtkDataRepresentation* rep) override;

private slots:
  void slotQtSelectionChanged(const QItemSelection&, const QItemSelection&);

private:
  void SetSVTKSelection();
  svtkMTimeType CurrentSelectionMTime;
  svtkMTimeType LastInputMTime;

  svtkSetStringMacro(ColorArrayNameInternal);
  svtkGetStringMacro(ColorArrayNameInternal);

  QPointer<QTreeView> TreeView;
  QPointer<QColumnView> ColumnView;
  QPointer<QWidget> Widget;
  QPointer<QVBoxLayout> Layout;
  QPointer<QItemSelectionModel> SelectionModel;
  QList<int> HiddenColumns;
  svtkQtTreeModelAdapter* TreeAdapter;
  QAbstractItemView* View;
  char* ColorArrayNameInternal;
  QFilterTreeProxyModel* TreeFilter;

  svtkSmartPointer<svtkApplyColors> ApplyColors;

  svtkQtTreeView(const svtkQtTreeView&) = delete;
  void operator=(const svtkQtTreeView&) = delete;
};

#endif
