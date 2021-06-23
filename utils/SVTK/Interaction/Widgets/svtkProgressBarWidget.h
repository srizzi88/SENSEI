/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgressBarWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgressBarWidget
 * @brief   2D widget for placing and manipulating a progress bar
 *
 * This class provides support for interactively displaying and manipulating
 * a progress bar.A Progress bar is defined by a progress rate and the color of the bar and
 * its background.
 * This widget allows you to interactively place and resize the progress bar.
 * To use this widget, simply create a svtkProgressBarRepresentation (or subclass)
 * and associate it with a svtkProgressBarWidget instance.
 *
 * @sa
 * svtkBorderWidget
 */

#ifndef svtkProgressBarWidget_h
#define svtkProgressBarWidget_h

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkProgressBarRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkProgressBarWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkProgressBarWidget* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkProgressBarWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkProgressBarRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkProgressBarWidget();
  ~svtkProgressBarWidget() override;

private:
  svtkProgressBarWidget(const svtkProgressBarWidget&) = delete;
  void operator=(const svtkProgressBarWidget&) = delete;
};

#endif
