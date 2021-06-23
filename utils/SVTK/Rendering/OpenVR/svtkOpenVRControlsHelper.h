/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRControlsHelper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * svtkOpenVRPanelRepresentation
 */

#ifndef svtkOpenVRControlsHelper_h
#define svtkOpenVRControlsHelper_h

#include "svtkEventData.h" // for enums
#include "svtkNew.h"       // for iVar
#include "svtkProp.h"
#include "svtkRenderingOpenVRModule.h" // For export macro
#include "svtkStdString.h"             // needed for svtkStdString iVar.
#include "svtkWeakPointer.h"           // needed for svtkWeakPointer iVar.

class svtkActor;
class svtkProperty;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkCellArray;
class svtkPoints;
class svtkTextActor3D;
class svtkTransform;

class svtkLineSource;
class svtkPolyDataMapper;
class svtkRenderer;
class svtkCallbackCommand;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRControlsHelper : public svtkProp
{
public:
  /**
   * Instantiate the class.
   */
  static svtkOpenVRControlsHelper* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkOpenVRControlsHelper, svtkProp);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  enum ButtonSides
  {
    Back = -1,
    Front = 1
  };

  enum DrawSides
  {
    Left = -1,
    Right = 1
  };

  //@{
  /**
   * Methods to interface with the svtkOpenVRPanelWidget.
   */
  void BuildRepresentation();
  void UpdateRepresentation();
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set Tooltip text (used by TextActor)
   */
  void SetText(svtkStdString str);
  //@}

  void SetTooltipInfo(const char* s, int buttonSide, int drawSide, const char* txt)
  {
    if (!s || !txt)
    {
      return;
    }
    this->ComponentName = svtkStdString(s);
    this->DrawSide = drawSide;
    this->ButtonSide = buttonSide;
    this->SetText(svtkStdString(txt));
  }

  void SetEnabled(bool enabled);
  svtkGetMacro(Enabled, bool);
  svtkBooleanMacro(Enabled, bool);

  void SetDevice(svtkEventDataDevice val);

  virtual void SetRenderer(svtkRenderer* ren);
  virtual svtkRenderer* GetRenderer();

protected:
  svtkOpenVRControlsHelper();
  ~svtkOpenVRControlsHelper() override;

  double FrameSize[2];

  // The text
  svtkTextActor3D* TextActor;
  svtkStdString Text;

  // The line
  svtkLineSource* LineSource;
  svtkPolyDataMapper* LineMapper;
  svtkActor* LineActor;

  svtkEventDataDevice Device;

  // Tooltip parameters
  svtkStdString ComponentName;
  int DrawSide;   // Left/Right
  int ButtonSide; // Front/Back

  bool Enabled;

  double ControlPositionLC[3];

  // The renderer in which this widget is placed
  svtkWeakPointer<svtkRenderer> Renderer;

  svtkCallbackCommand* MoveCallbackCommand;
  unsigned long ObserverTag;
  static void MoveEvent(svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  void InitControlPosition();

  svtkNew<svtkTransform> TempTransform;
  double LastPhysicalTranslation[3];
  double LastEventPosition[3];
  double LastEventOrientation[4];
  bool NeedUpdate;
  bool LabelVisible;

private:
  svtkOpenVRControlsHelper(const svtkOpenVRControlsHelper&) = delete;
  void operator=(const svtkOpenVRControlsHelper&) = delete;
};

#endif
