/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtListView.h

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
 * @class   svtkQtListView
 * @brief   A SVTK view based on a Qt List view.
 *
 *
 * svtkQtListView is a SVTK view using an underlying QListView.
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for implementing
 * this class
 */

#ifndef svtkQtListView_h
#define svtkQtListView_h

#include "svtkQtView.h"
#include "svtkViewsQtModule.h" // For export macro

#include "svtkSmartPointer.h" // Needed for member variables
#include <QImage>            // Needed for the icon methods
#include <QPointer>          // Needed for the internal list view

class svtkApplyColors;
class svtkDataObjectToTable;
class QItemSelection;
class QSortFilterProxyModel;
class QListView;
class svtkQtTableModelAdapter;

class SVTKVIEWSQT_EXPORT svtkQtListView : public svtkQtView
{
  Q_OBJECT

public:
  static svtkQtListView* New();
  svtkTypeMacro(svtkQtListView, svtkQtView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  QWidget* GetWidget() override;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5,
  };

  //@{
  /**
   * The field type to copy into the output table.
   * Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
   */
  svtkGetMacro(FieldType, int);
  void SetFieldType(int);
  //@}

  /**
   * Enable drag and drop on this widget
   */
  void SetEnableDragDrop(bool);

  /**
   * Have the view alternate its row colors
   */
  void SetAlternatingRowColors(bool);

  /**
   * The strategy for how to decorate rows.
   * Should be one of svtkQtTableModelAdapter::COLORS,
   * svtkQtTableModelAdapter::ICONS, or
   * svtkQtTableModelAdapter::NONE. Default is NONE.
   */
  void SetDecorationStrategy(int);

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

  /**
   * The column to display
   */
  void SetVisibleColumn(int col);

  /**
   * The column used to filter on
   */
  void SetFilterRegExp(const QRegExp& pattern);

  //@{
  /**
   * Set the icon ivars. Only used if the decoration strategy is set to ICONS.
   */
  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);
  void SetIconArrayName(const char* name);
  //@}

  void ApplyViewTheme(svtkViewTheme* theme) override;

  /**
   * Updates the view.
   */
  void Update() override;

protected:
  svtkQtListView();
  ~svtkQtListView() override;

  void AddRepresentationInternal(svtkDataRepresentation* rep) override;
  void RemoveRepresentationInternal(svtkDataRepresentation* rep) override;

private slots:
  void slotQtSelectionChanged(const QItemSelection&, const QItemSelection&);

private:
  void SetSVTKSelection();

  svtkMTimeType LastSelectionMTime;
  svtkMTimeType LastInputMTime;
  svtkMTimeType LastMTime;

  svtkSetStringMacro(ColorArrayNameInternal);
  svtkGetStringMacro(ColorArrayNameInternal);
  svtkSetStringMacro(IconIndexArrayNameInternal);
  svtkGetStringMacro(IconIndexArrayNameInternal);

  QPointer<QListView> ListView;
  svtkQtTableModelAdapter* TableAdapter;
  QSortFilterProxyModel* TableSorter;
  char* ColorArrayNameInternal;
  char* IconIndexArrayNameInternal;
  char* VisibleColumnName;
  bool SortSelectionToTop;
  bool ApplyRowColors;
  int FieldType;
  int VisibleColumn;

  svtkSmartPointer<svtkDataObjectToTable> DataObjectToTable;
  svtkSmartPointer<svtkApplyColors> ApplyColors;

  svtkQtListView(const svtkQtListView&) = delete;
  void operator=(const svtkQtListView&) = delete;
};

#endif
