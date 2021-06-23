/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextActor3D
 * @brief   An actor that displays text.
 *
 * The input text is rendered into a buffer, which in turn is used as a
 * texture applied onto a quad (a svtkImageActor is used under the hood).
 * @warning
 * This class is experimental at the moment.
 * - The orientation is not optimized, the quad should be oriented, not
 *   the text itself when it is rendered in the buffer (we end up with
 *   excessively big textures for 45 degrees angles).
 *   This will be fixed first.
 * - No checking is done at the moment regarding hardware texture size limits.
 *
 * @sa
 * svtkProp3D
 */

#ifndef svtkTextActor3D_h
#define svtkTextActor3D_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include <string>                   // for ivar

class svtkImageActor;
class svtkImageData;
class svtkTextProperty;

class SVTKRENDERINGCORE_EXPORT svtkTextActor3D : public svtkProp3D
{
public:
  static svtkTextActor3D* New();
  svtkTypeMacro(svtkTextActor3D, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the text string to be displayed.
   */
  svtkSetStringMacro(Input);
  svtkGetStringMacro(Input);
  //@}

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  /**
   * Since a 3D text actor is not pixel-aligned and positioned in 3D space,
   * the text is rendered at a constant DPI, rather than using the current
   * window DPI. This static method returns the DPI value used to produce the
   * text images.
   */
  static int GetRenderedDPI() { return 72; }

  /**
   * Shallow copy of this text actor. Overloads the virtual
   * svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) { this->svtkProp3D::GetBounds(bounds); }

  /**
   * Get the svtkTextRenderer-derived bounding box for the given svtkTextProperty
   * and text string str.  Results are returned in the four element bbox int
   * array.  This call can be used for sizing other elements.
   */
  int GetBoundingBox(int bbox[4]);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

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

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Draw the text actor to the screen.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

protected:
  svtkTextActor3D();
  ~svtkTextActor3D() override;

  char* Input;

  svtkImageActor* ImageActor;
  svtkImageData* ImageData;
  svtkTextProperty* TextProperty;

  svtkTimeStamp BuildTime;
  std::string LastInputString;

  virtual int UpdateImageActor();

private:
  svtkTextActor3D(const svtkTextActor3D&) = delete;
  void operator=(const svtkTextActor3D&) = delete;
};

#endif
