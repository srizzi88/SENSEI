/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSlice
 * @brief   represents an image in a 3D scene
 *
 * svtkImageSlice is used to represent an image in a 3D scene.  It displays
 * the image either as a slice or as a projection from the camera's
 * perspective. Adjusting the position and orientation of the slice
 * is done by adjusting the focal point and direction of the camera,
 * or alternatively the slice can be set manually in svtkImageMapper3D.
 * The lookup table and window/leve are set in svtkImageProperty.
 * Prop3D methods such as SetPosition() and RotateWXYZ() change the
 * position and orientation of the data with respect to SVTK world
 * coordinates.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImageMapper3D svtkImageProperty svtkProp3D
 */

#ifndef svtkImageSlice_h
#define svtkImageSlice_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkPropCollection;
class svtkImageProperty;
class svtkImageMapper3D;

class SVTKRENDERINGCORE_EXPORT svtkImageSlice : public svtkProp3D
{
public:
  svtkTypeMacro(svtkImageSlice, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates an Image with the following defaults: origin(0,0,0)
   * position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
   * orientation=(0,0,0).
   */
  static svtkImageSlice* New();

  //@{
  /**
   * Set/Get the mapper.
   */
  void SetMapper(svtkImageMapper3D* mapper);
  svtkGetObjectMacro(Mapper, svtkImageMapper3D);
  //@}

  //@{
  /**
   * Set/Get the image display properties.
   */
  void SetProperty(svtkImageProperty* property);
  virtual svtkImageProperty* GetProperty();
  //@}

  /**
   * Update the rendering pipeline by updating the ImageMapper
   */
  void Update();

  //@{
  /**
   * Get the bounds - either all six at once
   * (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) { this->svtkProp3D::GetBounds(bounds); }
  double GetMinXBound();
  double GetMaxXBound();
  double GetMinYBound();
  double GetMaxYBound();
  double GetMinZBound();
  double GetMaxZBound();
  //@}

  /**
   * Return the MTime also considering the property etc.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, mappers,
   * etc.
   */
  svtkMTimeType GetRedrawMTime() override;

  //@{
  /**
   * Force the actor to be treated as translucent.
   */
  svtkGetMacro(ForceTranslucent, bool);
  svtkSetMacro(ForceTranslucent, bool);
  svtkBooleanMacro(ForceTranslucent, bool);
  //@}

  /**
   * Shallow copy of this svtkImageSlice. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors, volumes, and images. These
   * methods are used in that process.
   */
  void GetImages(svtkPropCollection*);

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Internal method, should only be used by rendering.
   * This method will always return 0 unless ForceTranslucent is On.
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * This causes the image and its mapper to be rendered. Note that a side
   * effect of this method is that the pipeline will be updated.
   */
  virtual void Render(svtkRenderer*);

  /**
   * Release any resources held by this prop.
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

  /**
   * For stacked image rendering, set the pass.  The first pass
   * renders just the backing polygon, the second pass renders
   * the image, and the third pass renders the depth buffer.
   * Set to -1 to render all of these in the same pass.
   */
  void SetStackedImagePass(int pass);

protected:
  svtkImageSlice();
  ~svtkImageSlice() override;

  svtkImageMapper3D* Mapper;
  svtkImageProperty* Property;

  bool ForceTranslucent;

private:
  svtkImageSlice(const svtkImageSlice&) = delete;
  void operator=(const svtkImageSlice&) = delete;
};

#endif
