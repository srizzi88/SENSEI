/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStack.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStack
 * @brief   manages a stack of composited images
 *
 * svtkImageStack manages the compositing of a set of images. Each image
 * is assigned a layer number through its property object, and it is
 * this layer number that determines the compositing order: images with
 * a higher layer number are drawn over top of images with a lower layer
 * number.  The image stack has a SetActiveLayer method for controlling
 * which layer to use for interaction and picking.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImageMapper3D svtkImageProperty svtkProp3D
 */

#ifndef svtkImageStack_h
#define svtkImageStack_h

#include "svtkImageSlice.h"
#include "svtkRenderingImageModule.h" // For export macro

class svtkImageSliceCollection;
class svtkImageProperty;
class svtkImageMapper3D;
class svtkCollection;

class SVTKRENDERINGIMAGE_EXPORT svtkImageStack : public svtkImageSlice
{
public:
  svtkTypeMacro(svtkImageStack, svtkImageSlice);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkImageStack* New();

  /**
   * Add an image to the stack.  If the image is already present, then
   * this method will do nothing.
   */
  void AddImage(svtkImageSlice* prop);

  /**
   * Remove an image from the stack.  If the image is not present, then
   * this method will do nothing.
   */
  void RemoveImage(svtkImageSlice* prop);

  /**
   * Check if an image is present.  The returned value is one or zero.
   */
  int HasImage(svtkImageSlice* prop);

  /**
   * Get the list of images as a svtkImageSliceCollection.
   */
  svtkImageSliceCollection* GetImages() { return this->Images; }

  //@{
  /**
   * Set the active layer number.  This is the layer that will be
   * used for picking and interaction.
   */
  svtkSetMacro(ActiveLayer, int);
  int GetActiveLayer() { return this->ActiveLayer; }
  //@}

  /**
   * Get the active image.  This will be the topmost image whose
   * LayerNumber is the ActiveLayer.  If no image matches, then NULL
   * will be returned.
   */
  svtkImageSlice* GetActiveImage();

  /**
   * Get the mapper for the currently active image.
   */
  svtkImageMapper3D* GetMapper() override;

  /**
   * Get the property for the currently active image.
   */
  svtkImageProperty* GetProperty() override;

  //@{
  /**
   * Get the combined bounds of all of the images.
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) { this->svtkProp3D::GetBounds(bounds); }
  //@}

  /**
   * Return the max MTime of all the images.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, mappers,
   * etc.
   */
  svtkMTimeType GetRedrawMTime() override;

  /**
   * Shallow copy of this prop. Overloads the virtual svtkProp method.
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
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any resources held by this prop.
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

  //@{
  /**
   * Methods for traversing the stack as if it was an assembly.
   * The traversal only gives the view prop for the active layer.
   */
  void InitPathTraversal() override;
  svtkAssemblyPath* GetNextPath() override;
  int GetNumberOfPaths() override;
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used to construct assembly paths and perform part traversal.
   */
  void BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path) override;

protected:
  svtkImageStack();
  ~svtkImageStack() override;

  void SetMapper(svtkImageMapper3D* mapper);
  void SetProperty(svtkImageProperty* property);

  void PokeMatrices(svtkMatrix4x4* matrix);
  void UpdatePaths();

  svtkTimeStamp PathTime;
  svtkCollection* ImageMatrices;
  svtkImageSliceCollection* Images;
  int ActiveLayer;

private:
  svtkImageStack(const svtkImageStack&) = delete;
  void operator=(const svtkImageStack&) = delete;
};

#endif
