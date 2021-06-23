/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogoWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLogoWidget
 * @brief   2D widget for placing and manipulating a logo
 *
 * This class provides support for interactively displaying and manipulating
 * a logo. Logos are defined by an image; this widget simply allows you to
 * interactively place and resize the image logo. To use this widget, simply
 * create a svtkLogoRepresentation (or subclass) and associate it with the
 * svtkLogoWidget.
 *
 * @sa
 * svtkBorderWidget
 */

#ifndef svtkLogoWidget_h
#define svtkLogoWidget_h

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkLogoRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLogoWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkLogoWidget* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkLogoWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkLogoRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkLogoWidget();
  ~svtkLogoWidget() override;

private:
  svtkLogoWidget(const svtkLogoWidget&) = delete;
  void operator=(const svtkLogoWidget&) = delete;
};

#endif
