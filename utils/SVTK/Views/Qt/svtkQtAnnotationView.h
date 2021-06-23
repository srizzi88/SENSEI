/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtAnnotationView.h

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
 * @class   svtkQtAnnotationView
 * @brief   A SVTK view that displays the annotations
 *    on its annotation link.
 *
 *
 * svtkQtAnnotationView is a SVTK view using an underlying QTableView.
 *
 */

#ifndef svtkQtAnnotationView_h
#define svtkQtAnnotationView_h

#include "svtkQtView.h"
#include "svtkViewsQtModule.h" // For export macro
#include <QObject>            // Needed for the Q_OBJECT macro

#include <QPointer> // Needed to hold the view

class svtkQtAnnotationLayersModelAdapter;

class QItemSelection;
class QTableView;

class SVTKVIEWSQT_EXPORT svtkQtAnnotationView : public svtkQtView
{
  Q_OBJECT

public:
  static svtkQtAnnotationView* New();
  svtkTypeMacro(svtkQtAnnotationView, svtkQtView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  QWidget* GetWidget() override;

  /**
   * Updates the view.
   */
  void Update() override;

protected:
  svtkQtAnnotationView();
  ~svtkQtAnnotationView() override;

private slots:
  void slotQtSelectionChanged(const QItemSelection&, const QItemSelection&);

private:
  svtkMTimeType LastInputMTime;

  QPointer<QTableView> View;
  svtkQtAnnotationLayersModelAdapter* Adapter;

  svtkQtAnnotationView(const svtkQtAnnotationView&) = delete;
  void operator=(const svtkQtAnnotationView&) = delete;
};

#endif
