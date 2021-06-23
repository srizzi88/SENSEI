/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProp
 * @brief   abstract superclass for all actors, volumes and annotations
 *
 * svtkProp is an abstract superclass for any objects that can exist in a
 * rendered scene (either 2D or 3D). Instances of svtkProp may respond to
 * various render methods (e.g., RenderOpaqueGeometry()). svtkProp also
 * defines the API for picking, LOD manipulation, and common instance
 * variables that control visibility, picking, and dragging.
 * @sa
 * svtkActor2D svtkActor svtkVolume svtkProp3D
 */

#ifndef svtkProp_h
#define svtkProp_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include <vector>                   // for method args

class svtkAssemblyPath;
class svtkAssemblyPaths;
class svtkHardwareSelector;
class svtkMatrix4x4;
class svtkPropCollection;
class svtkViewport;
class svtkWindow;
class svtkInformation;
class svtkInformationIntegerKey;
class svtkInformationDoubleVectorKey;
class svtkShaderProperty;

class SVTKRENDERINGCORE_EXPORT svtkProp : public svtkObject
{
public:
  svtkTypeMacro(svtkProp, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  virtual void GetActors(svtkPropCollection*) {}
  virtual void GetActors2D(svtkPropCollection*) {}
  virtual void GetVolumes(svtkPropCollection*) {}

  //@{
  /**
   * Set/Get visibility of this svtkProp. Initial value is true.
   */
  svtkSetMacro(Visibility, svtkTypeBool);
  svtkGetMacro(Visibility, svtkTypeBool);
  svtkBooleanMacro(Visibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the pickable instance variable.  This determines if the svtkProp
   * can be picked (typically using the mouse). Also see dragable.
   * Initial value is true.
   */
  svtkSetMacro(Pickable, svtkTypeBool);
  svtkGetMacro(Pickable, svtkTypeBool);
  svtkBooleanMacro(Pickable, svtkTypeBool);
  //@}

  /**
   * Method fires PickEvent if the prop is picked.
   */
  virtual void Pick();

  //@{
  /**
   * Set/Get the value of the dragable instance variable. This determines if
   * an Prop, once picked, can be dragged (translated) through space.
   * This is typically done through an interactive mouse interface.
   * This does not affect methods such as SetPosition, which will continue
   * to work.  It is just intended to prevent some svtkProp'ss from being
   * dragged from within a user interface.
   * Initial value is true.
   */
  svtkSetMacro(Dragable, svtkTypeBool);
  svtkGetMacro(Dragable, svtkTypeBool);
  svtkBooleanMacro(Dragable, svtkTypeBool);
  //@}

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, textures
   * etc.
   */
  virtual svtkMTimeType GetRedrawMTime() { return this->GetMTime(); }

  //@{
  /**
   * In case the Visibility flag is true, tell if the bounds of this prop
   * should be taken into account or ignored during the computation of other
   * bounding boxes, like in svtkRenderer::ResetCamera().
   * Initial value is true.
   */
  svtkSetMacro(UseBounds, bool);
  svtkGetMacro(UseBounds, bool);
  svtkBooleanMacro(UseBounds, bool);
  //@}

  /**
   * Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   * in world coordinates. NULL means that the bounds are not defined.
   */
  virtual double* GetBounds() SVTK_SIZEHINT(6) { return nullptr; }

  /**
   * Shallow copy of this svtkProp.
   */
  virtual void ShallowCopy(svtkProp* prop);

  //@{
  /**
   * svtkProp and its subclasses can be picked by subclasses of
   * svtkAbstractPicker (e.g., svtkPropPicker). The following methods interface
   * with the picking classes and return "pick paths". A pick path is a
   * hierarchical, ordered list of props that form an assembly.  Most often,
   * when a svtkProp is picked, its path consists of a single node (i.e., the
   * prop). However, classes like svtkAssembly and svtkPropAssembly can return
   * more than one path, each path being several layers deep. (See
   * svtkAssemblyPath for more information.)  To use these methods - first
   * invoke InitPathTraversal() followed by repeated calls to GetNextPath().
   * GetNextPath() returns a NULL pointer when the list is exhausted.
   */
  virtual void InitPathTraversal();
  virtual svtkAssemblyPath* GetNextPath();
  virtual int GetNumberOfPaths() { return 1; }
  //@}

  /**
   * These methods are used by subclasses to place a matrix (if any) in the
   * prop prior to rendering. Generally used only for picking. See svtkProp3D
   * for more information.
   */
  virtual void PokeMatrix(svtkMatrix4x4* svtkNotUsed(matrix)) {}
  virtual svtkMatrix4x4* GetMatrix() { return nullptr; }

  //@{
  /**
   * Set/Get property keys. Property keys can be digest by some rendering
   * passes.
   * For instance, the user may mark a prop as a shadow caster for a
   * shadow mapping render pass. Keys are documented in render pass classes.
   * Initial value is NULL.
   */
  svtkGetObjectMacro(PropertyKeys, svtkInformation);
  virtual void SetPropertyKeys(svtkInformation* keys);
  //@}

  /**
   * Tells if the prop has all the required keys.
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool HasKeys(svtkInformation* requiredKeys);

  /**
   * Optional Key Indicating the texture unit for general texture mapping
   * Old OpenGL was a state machine where you would push or pop
   * items. The new OpenGL design is more mapper centric. Some
   * classes push a texture and then assume a mapper will use it.
   * The new design wants explicit communication of when a texture
   * is being used.  This key can be used to pass that information
   * down to a mapper.
   */
  static svtkInformationIntegerKey* GeneralTextureUnit();

  /**
   * Optional Key Indicating the texture transform for general texture mapping
   * Old OpenGL was a state machine where you would push or pop
   * items. The new OpenGL design is more mapper centric. Some
   * classes push a texture and then assume a mapper will use it.
   * The new design wants explicit communication of when a texture
   * is being used.  This key can be used to pass that information
   * down to a mapper.
   */
  static svtkInformationDoubleVectorKey* GeneralTextureTransform();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * All concrete subclasses must be able to render themselves.
   * There are four key render methods in svtk and they correspond
   * to four different points in the rendering cycle. Any given
   * prop may implement one or more of these methods.
   * The first method is intended for rendering all opaque geometry. The
   * second method is intended for rendering all translucent polygonal
   * geometry. The third one is intended for rendering all translucent
   * volumetric geometry. Most of the volume rendering mappers draw their
   * results during this third method.
   * The last method is to render any 2D annotation or overlays.
   * Each of these methods return an integer value indicating
   * whether or not this render method was applied to this data.
   */
  virtual int RenderOpaqueGeometry(svtkViewport*) { return 0; }
  virtual int RenderTranslucentPolygonalGeometry(svtkViewport*) { return 0; }
  virtual int RenderVolumetricGeometry(svtkViewport*) { return 0; }
  virtual int RenderOverlay(svtkViewport*) { return 0; }

  /**
   * Render the opaque geometry only if the prop has all the requiredKeys.
   * This is recursive for composite props like svtkAssembly.
   * An implementation is provided in svtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredOpaqueGeometry(svtkViewport* v, svtkInformation* requiredKeys);

  /**
   * Render the translucent polygonal geometry only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like svtkAssembly.
   * An implementation is provided in svtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredTranslucentPolygonalGeometry(
    svtkViewport* v, svtkInformation* requiredKeys);

  /**
   * Render the volumetric geometry only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like svtkAssembly.
   * An implementation is provided in svtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredVolumetricGeometry(svtkViewport* v, svtkInformation* requiredKeys);

  /**
   * Render in the overlay of the viewport only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like svtkAssembly.
   * An implementation is provided in svtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredOverlay(svtkViewport* v, svtkInformation* requiredKeys);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Does this prop have some translucent polygonal geometry?
   * This method is called during the rendering process to know if there is
   * some translucent polygonal geometry. A simple prop that has some
   * translucent polygonal geometry will return true. A composite prop (like
   * svtkAssembly) that has at least one sub-prop that has some translucent
   * polygonal geometry will return true.
   * Default implementation return false.
   */
  virtual svtkTypeBool HasTranslucentPolygonalGeometry() { return 0; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Does this prop have some opaque geometry?
   * This method is called during the rendering process to know if there is
   * some opaque geometry. A simple prop that has some
   * opaque geometry will return true. A composite prop (like
   * svtkAssembly) that has at least one sub-prop that has some opaque
   * polygonal geometry will return true.
   * Default implementation return true.
   */
  virtual svtkTypeBool HasOpaqueGeometry() { return 1; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) {}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * The EstimatedRenderTime may be used to select between different props,
   * for example in LODProp it is used to select the level-of-detail.
   * The value is returned in seconds. For simple geometry the accuracy may
   * not be great due to buffering. For ray casting, which is already
   * multi-resolution, the current resolution of the image is factored into
   * the time. We need the viewport for viewing parameters that affect timing.
   * The no-arguments version simply returns the value of the variable with
   * no estimation.
   */
  virtual double GetEstimatedRenderTime(svtkViewport*) { return this->EstimatedRenderTime; }
  virtual double GetEstimatedRenderTime() { return this->EstimatedRenderTime; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * This method is used by, for example, the svtkLODProp3D in order to
   * initialize the estimated render time at start-up to some user defined
   * value.
   */
  virtual void SetEstimatedRenderTime(double t)
  {
    this->EstimatedRenderTime = t;
    this->SavedEstimatedRenderTime = t;
  }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * When the EstimatedRenderTime is first set to 0.0 (in the
   * SetAllocatedRenderTime method) the old value is saved. This
   * method is used to restore that old value should the render be
   * aborted.
   */
  virtual void RestoreEstimatedRenderTime()
  {
    this->EstimatedRenderTime = this->SavedEstimatedRenderTime;
  }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * This method is intended to allow the renderer to add to the
   * EstimatedRenderTime in props that require information that
   * the renderer has in order to do this. For example, props
   * that are rendered with a ray casting method do not know
   * themselves how long it took for them to render. We don't want to
   * cause a this->Modified() when we set this value since it is not
   * really a modification to the object. (For example, we don't want
   * to rebuild matrices at every render because the estimated render time
   * is changing)
   */
  virtual void AddEstimatedRenderTime(double t, svtkViewport* svtkNotUsed(vp))
  {
    this->EstimatedRenderTime += t;
  }

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * The renderer may use the allocated rendering time to determine
   * how to render this actor. Therefore it might need the information
   * provided in the viewport.
   * A side effect of this method is to reset the EstimatedRenderTime to
   * 0.0. This way, each of the ways that this prop may be rendered can
   * be timed and added together into this value.
   */
  virtual void SetAllocatedRenderTime(double t, svtkViewport* svtkNotUsed(v))
  {
    this->AllocatedRenderTime = t;
    this->SavedEstimatedRenderTime = this->EstimatedRenderTime;
    this->EstimatedRenderTime = 0.0;
  }
  //@}

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  svtkGetMacro(AllocatedRenderTime, double);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Get/Set the multiplier for the render time. This is used
   * for culling and is a number between 0 and 1. It is used
   * to create the allocated render time value.
   */
  void SetRenderTimeMultiplier(double t) { this->RenderTimeMultiplier = t; }
  svtkGetMacro(RenderTimeMultiplier, double);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used to construct assembly paths and perform part traversal.
   */
  virtual void BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by svtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  virtual bool GetSupportsSelection() { return false; }

  /**
   * allows a prop to update a selections color buffers
   *
   */
  virtual void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* /* sel */, std::vector<unsigned int>& /* pixeloffsets */)
  {
  }

  //@{
  /**
   * Get the number of consumers
   */
  svtkGetMacro(NumberOfConsumers, int);
  //@}

  //@{
  /**
   * Add or remove or get or check a consumer,
   */
  void AddConsumer(svtkObject* c);
  void RemoveConsumer(svtkObject* c);
  svtkObject* GetConsumer(int i);
  int IsConsumer(svtkObject* c);
  //@}

  //@{
  /**
   * Set/Get the shader property.
   */
  virtual void SetShaderProperty(svtkShaderProperty* property);
  virtual svtkShaderProperty* GetShaderProperty();
  //@}

  //@{
  // Get if we are in the translucent polygonal geometry pass
  virtual bool IsRenderingTranslucentPolygonalGeometry() { return false; }
  //@}

protected:
  svtkProp();
  ~svtkProp() override;

  svtkTypeBool Visibility;
  svtkTypeBool Pickable;
  svtkTypeBool Dragable;
  bool UseBounds;

  double AllocatedRenderTime;
  double EstimatedRenderTime;
  double SavedEstimatedRenderTime;
  double RenderTimeMultiplier;

  // how many consumers does this object have
  int NumberOfConsumers;
  svtkObject** Consumers;

  // support multi-part props and access to paths of prop
  // stuff that follows is used to build the assembly hierarchy
  svtkAssemblyPaths* Paths;

  svtkInformation* PropertyKeys;

  // User-defined shader replacement and uniform variables
  svtkShaderProperty* ShaderProperty;

private:
  svtkProp(const svtkProp&) = delete;
  void operator=(const svtkProp&) = delete;
};

#endif
