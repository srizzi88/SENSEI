/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtDebugLeaksView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQtDebugLeaksView
 * @brief   view class to display contents of svtkQtDebugLeaksModel
 *
 *
 * A widget the displays all svtkObjectBase derived objects that are alive in
 * memory.  The widget is designed to be a debugging tool that is instantiated
 * at program startup and displayed as a top level widget.  Simply create the
 * widget and call show().
 */

#ifndef svtkQtDebugLeaksView_h
#define svtkQtDebugLeaksView_h

#include "svtkGUISupportQtModule.h" // For export macro
#include <QWidget>

class QModelIndex;
class svtkObjectBase;
class svtkQtDebugLeaksModel;

class SVTKGUISUPPORTQT_EXPORT svtkQtDebugLeaksView : public QWidget
{
  Q_OBJECT

public:
  svtkQtDebugLeaksView(QWidget* p = nullptr);
  ~svtkQtDebugLeaksView() override;

  svtkQtDebugLeaksModel* model();

  /**
   * Returns whether or not the regexp filter is enabled
   */
  bool filterEnabled() const;

  /**
   * Enabled or disables the regexp filter
   */
  void setFilterEnabled(bool value);

  /**
   * Returns the regexp filter line edit's current text
   */
  QString filterText() const;

  /**
   * Sets the current text in the regexp filter line edit
   */
  void setFilterText(const QString& text);

protected:
  virtual void onObjectDoubleClicked(svtkObjectBase* object);
  virtual void onClassNameDoubleClicked(const QString& className);

protected slots:

  void onCurrentRowChanged(const QModelIndex& current);
  void onRowDoubleClicked(const QModelIndex&);
  void onFilterTextChanged(const QString& filterText);
  void onFilterToggled();
  void onFilterHelp();

private:
  class qInternal;
  qInternal* Internal;

  Q_DISABLE_COPY(svtkQtDebugLeaksView);
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtDebugLeaksView.h
