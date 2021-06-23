/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFlagpoleLabel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkFlagpoleLabel
 * @brief Renders a flagpole (line) with a label at the top that faces the camera
 *
 * This class draws a line from the base to the top of the flagpole. It then
 * places a text annotation at the top, centered horizontally. The text is
 * always oriented with the flagpole but will rotate aroundthe flagpole to
 * face the camera.
 */

#ifndef svtkFlagpoleLabel_h
#define svtkFlagpoleLabel_h

#include "svtkActor.h"
#include "svtkNew.h"                 // For.... svtkNew!
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // For.... svtkSmartPointer!

class svtkActor;
class svtkImageData;
class svtkLineSource;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkRenderer;
class svtkTextProperty;
class svtkTextRenderer;

class SVTKRENDERINGCORE_EXPORT svtkFlagpoleLabel : public svtkActor
{
public:
  static svtkFlagpoleLabel* New();
  svtkTypeMacro(svtkFlagpoleLabel, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The UTF-8 encoded string to display.
   * @{
   */
  void SetInput(const char* in);
  svtkGetStringMacro(Input);
  /** @} */

  /**
   * The svtkTextProperty object that controls the rendered text.
   * @{
   */
  void SetTextProperty(svtkTextProperty* tprop);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  /** @} */

  /**
   * Force the actor to render during the opaque or translucent pass.
   * @{
   */
  void SetForceOpaque(bool opaque) override;
  bool GetForceOpaque() override;
  void ForceOpaqueOn() override;
  void ForceOpaqueOff() override;
  void SetForceTranslucent(bool trans) override;
  bool GetForceTranslucent() override;
  void ForceTranslucentOn() override;
  void ForceTranslucentOff() override;
  /**@}*/

  /**
   * Defers to internal actor.
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Check/update geometry/texture in opaque pass, since it only happens once.
   */
  int RenderOpaqueGeometry(svtkViewport* vp) override;

  /**
   * Just render in translucent pass, since it can execute multiple times
   * (depth peeling, for instance).
   */
  int RenderTranslucentPolygonalGeometry(svtkViewport* vp) override;

  void ReleaseGraphicsResources(svtkWindow* win) override;
  double* GetBounds() override;
  using Superclass::GetBounds;

  /**
   * Set/Get the world coordinate position of the base
   */
  svtkGetVector3Macro(BasePosition, double);
  void SetBasePosition(double x, double y, double z);

  /**
   * Set/Get the world coordinate position of the top
   */
  svtkGetVector3Macro(TopPosition, double);
  void SetTopPosition(double x, double y, double z);

  /**
   * Set/Get the size of the flag. 1.0 is the default size
   * which corresponds to a preset texels/window value. Adjust this
   * to increase or decrease the default size.
   */
  svtkGetMacro(FlagSize, double);
  svtkSetMacro(FlagSize, double);

protected:
  svtkFlagpoleLabel();
  ~svtkFlagpoleLabel() override;

  bool InputIsValid();

  void UpdateInternals(svtkRenderer* ren);

  bool TextureIsStale(svtkRenderer* ren);
  void GenerateTexture(svtkRenderer* ren);

  bool QuadIsStale(svtkRenderer* ren);
  void GenerateQuad(svtkRenderer* ren);

  // Used by the opaque pass to tell the translucent pass not to render.
  void Invalidate();
  bool IsValid();

  // Used to sync the internal actor's state.
  void PreRender();

  // Text specification:
  char* Input;
  svtkTextProperty* TextProperty;

  // Cached metadata to determine if things need rebuildin'
  int RenderedDPI;
  svtkTimeStamp InputMTime;

  // We cache this so we can recompute the bounds between renders, if needed.
  svtkSmartPointer<svtkRenderer> RenderedRenderer;

  // Rendering stuffies
  svtkNew<svtkTextRenderer> TextRenderer;
  svtkNew<svtkImageData> Image;
  svtkNew<svtkPolyData> Quad;
  svtkNew<svtkPolyDataMapper> QuadMapper;
  svtkNew<svtkActor> QuadActor;

  svtkNew<svtkPolyDataMapper> PoleMapper;
  svtkNew<svtkLineSource> LineSource;
  svtkNew<svtkActor> PoleActor;

  double TopPosition[3];
  double BasePosition[3];
  double FlagSize;

private:
  svtkFlagpoleLabel(const svtkFlagpoleLabel&) = delete;
  void operator=(const svtkFlagpoleLabel&) = delete;
};

#endif // svtkFlagpoleLabel_h
