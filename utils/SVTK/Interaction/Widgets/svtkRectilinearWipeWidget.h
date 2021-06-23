/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearWipeWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearWipeWidget
 * @brief   interactively control an instance of svtkImageRectilinearWipe filter
 *
 * The svtkRectilinearWipeWidget is used to interactively control an instance
 * of svtkImageRectilinearWipe (and an associated svtkImageActor used to
 * display the rectilinear wipe). A rectilinear wipe is a 2x2 checkerboard
 * pattern created by combining two separate images, where various
 * combinations of the checker squares are possible. Using this widget, the
 * user can adjust the layout of the checker pattern, such as moving the
 * center point, moving the horizontal separator, or moving the vertical
 * separator. These capabilities are particularly useful for comparing two
 * images.
 *
 * To use this widget, specify its representation (by default the
 * representation is an instance of svtkRectilinearWipeProp). The
 * representation generally requires that you specify an instance of
 * svtkImageRectilinearWipe and an instance of svtkImageActor. Other instance
 * variables may also be required to be set -- see the documentation for
 * svtkRectilinearWipeProp (or appropriate subclass).
 *
 * By default, the widget responds to the following events:
 * <pre>
 * Selecting the center point, horizontal separator, and verticel separator:
 *   LeftButtonPressEvent - move the separators
 *   LeftButtonReleaseEvent - release the separators
 *   MouseMoveEvent - move the separators
 * </pre>
 * Selecting the center point allows you to move the horizontal and vertical
 * separators simultaneously. Otherwise only horizontal or vertical motion
 * is possible/
 *
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events into
 * the svtkRectilinearWipeWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for motion has been invoked
 * </pre>
 *
 * In turn, when these widget events are processed, the
 * svtkRectilinearWipeWidget invokes the following SVTK events (which
 * observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 * @warning
 * The appearance of this widget is defined by its representation, including
 * any properties associated with the representation.  The widget
 * representation is a type of svtkProp that defines a particular API that
 * works with this widget. If desired, the svtkProp may be subclassed to
 * create new looks for the widget.
 *
 * @sa
 * svtkRectilinearWipeProp svtkImageRectilinearWipe svtkImageActor
 * svtkCheckerboardWidget
 */

#ifndef svtkRectilinearWipeWidget_h
#define svtkRectilinearWipeWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkRectilinearWipeRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkRectilinearWipeWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static svtkRectilinearWipeWidget* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkRectilinearWipeWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkRectilinearWipeRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkRectilinearWipeRepresentation.
   */
  svtkRectilinearWipeRepresentation* GetRectilinearWipeRepresentation()
  {
    return reinterpret_cast<svtkRectilinearWipeRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkRectilinearWipeWidget();
  ~svtkRectilinearWipeWidget() override;

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Selected
  };

private:
  svtkRectilinearWipeWidget(const svtkRectilinearWipeWidget&) = delete;
  void operator=(const svtkRectilinearWipeWidget&) = delete;
};

#endif
