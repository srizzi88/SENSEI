/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBillboardTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkBillboardTextActor3D
 * @brief Renders pixel-aligned text, facing the camera, anchored at a 3D point.
 */

#ifndef svtkBillboardTextActor3D_h
#define svtkBillboardTextActor3D_h

#include "svtkNew.h" // For.... svtkNew!
#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // For.... svtkSmartPointer!

class svtkActor;
class svtkImageData;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkRenderer;
class svtkTextProperty;
class svtkTextRenderer;
class svtkTexture;

class SVTKRENDERINGCORE_EXPORT svtkBillboardTextActor3D : public svtkProp3D
{
public:
  static svtkBillboardTextActor3D* New();
  svtkTypeMacro(svtkBillboardTextActor3D, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The UTF-8 encoded string to display.
   * @{
   */
  void SetInput(const char* in);
  svtkGetStringMacro(Input);
  /** @} */

  /**
   * Can be used to set a fixed offset from the anchor point.
   * Use display coordinates.
   * @{
   */
  svtkGetVector2Macro(DisplayOffset, int);
  svtkSetVector2Macro(DisplayOffset, int);
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
  virtual void SetForceOpaque(bool opaque);
  virtual bool GetForceOpaque();
  virtual void ForceOpaqueOn();
  virtual void ForceOpaqueOff();
  virtual void SetForceTranslucent(bool trans);
  virtual bool GetForceTranslucent();
  virtual void ForceTranslucentOn();
  virtual void ForceTranslucentOff();
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
   * Returns the anchor position in display coordinates, with depth in NDC.
   * Valid after calling RenderOpaqueGeometry.
   */
  svtkGetVector3Macro(AnchorDC, double);

protected:
  svtkBillboardTextActor3D();
  ~svtkBillboardTextActor3D() override;

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

  // Offset in display coordinates.
  int DisplayOffset[2];

  // Cached metadata to determine if things need rebuildin'
  int RenderedDPI;
  svtkTimeStamp InputMTime;

  // We cache this so we can recompute the bounds between renders, if needed.
  svtkSmartPointer<svtkRenderer> RenderedRenderer;

  // Rendering stuffies
  svtkNew<svtkTextRenderer> TextRenderer;
  svtkNew<svtkImageData> Image;
  svtkNew<svtkTexture> Texture;
  svtkNew<svtkPolyData> Quad;
  svtkNew<svtkPolyDataMapper> QuadMapper;
  svtkNew<svtkActor> QuadActor;

  // Display coordinate for anchor position. Z value is in NDC.
  // Cached for GL2PS export on OpenGL2:
  double AnchorDC[3];

private:
  svtkBillboardTextActor3D(const svtkBillboardTextActor3D&) = delete;
  void operator=(const svtkBillboardTextActor3D&) = delete;
};

#endif // svtkBillboardTextActor3D_h
