/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextWidget
 * @brief   widget for placing text on overlay plane
 *
 * This class provides support for interactively placing text on the 2D
 * overlay plane. The text is defined by an instance of svtkTextActor. It uses
 * the event bindings of its superclass (svtkBorderWidget). In addition, when
 * the text is selected, the widget emits a WidgetActivateEvent that
 * observers can watch for. This is useful for opening GUI dialogues to
 * adjust font characteristics, etc. (Please see the superclass for a
 * description of event bindings.)
 *
 * @sa
 * svtkBorderWidget svtkCaptionWidget
 */

#ifndef svtkTextWidget_h
#define svtkTextWidget_h

class svtkTextRepresentation;
class svtkTextActor;

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkTextWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate class.
   */
  static svtkTextWidget* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkTextWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkTextRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  //@{
  /**
   * Specify a svtkTextActor to manage. This is a convenient, alternative
   * method to specify the representation for the widget (i.e., used instead
   * of SetRepresentation()). It internally creates a svtkTextRepresentation
   * and then invokes svtkTextRepresentation::SetTextActor().
   */
  void SetTextActor(svtkTextActor* textActor);
  svtkTextActor* GetTextActor();
  //@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkTextWidget();
  ~svtkTextWidget() override;

private:
  svtkTextWidget(const svtkTextWidget&) = delete;
  void operator=(const svtkTextWidget&) = delete;
};

#endif
