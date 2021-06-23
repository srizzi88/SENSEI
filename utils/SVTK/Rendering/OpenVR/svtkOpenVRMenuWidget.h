/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRMenuWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRMenuWidget
 * @brief   3D widget to display a menu in VR
 *
 * @sa
 * svtkOpenVRMenuRepresentation
 */

#ifndef svtkOpenVRMenuWidget_h
#define svtkOpenVRMenuWidget_h

#include "svtkAbstractWidget.h"
#include "svtkRenderingOpenVRModule.h" // For export macro
#include <deque>                      // for ivar

class svtkEventData;
class svtkOpenVRMenuRepresentation;
class svtkPropMap;
class svtkProp;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRMenuWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkOpenVRMenuWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkOpenVRMenuWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkOpenVRMenuRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  //@{
  /**
   * Get the widget state.
   */
  svtkGetMacro(WidgetState, int);
  //@}

  // Manage the state of the widget
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  //@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, svtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  //@}

  void Show(svtkEventData* ed);
  void ShowSubMenu(svtkOpenVRMenuWidget*);

protected:
  svtkOpenVRMenuWidget();
  ~svtkOpenVRMenuWidget() override;

  int WidgetState;

  class InternalElement;
  std::deque<InternalElement*> Menus;

  // These are the callbacks for this widget
  static void StartMenuAction(svtkAbstractWidget*);
  static void SelectMenuAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  svtkCallbackCommand* EventCommand;
  static void EventCallback(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  /**
   * Update callback to check for the hovered prop
   */
  static void Update(svtkAbstractWidget*);

private:
  svtkOpenVRMenuWidget(const svtkOpenVRMenuWidget&) = delete;
  void operator=(const svtkOpenVRMenuWidget&) = delete;
};
#endif
