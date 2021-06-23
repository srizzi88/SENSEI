/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkActor
 * @brief   represents an object (geometry & properties) in a rendered scene
 *
 *
 * svtkActor is used to represent an entity in a rendering scene.  It inherits
 * functions related to the actors position, and orientation from
 * svtkProp. The actor also has scaling and maintains a reference to the
 * defining geometry (i.e., the mapper), rendering properties, and possibly a
 * texture map. svtkActor combines these instance variables into one 4x4
 * transformation matrix as follows: [x y z 1] = [x y z 1] Translate(-origin)
 * Scale(scale) Rot(y) Rot(x) Rot (z) Trans(origin) Trans(position)
 *
 * @sa
 * svtkProperty svtkTexture svtkMapper svtkAssembly svtkFollower svtkLODActor
 */

#ifndef svtkActor_h
#define svtkActor_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkPropCollection;
class svtkActorCollection;
class svtkTexture;
class svtkMapper;
class svtkProperty;

class SVTKRENDERINGCORE_EXPORT svtkActor : public svtkProp3D
{
public:
  svtkTypeMacro(svtkActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates an actor with the following defaults: origin(0,0,0)
   * position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
   * orientation=(0,0,0). No user defined matrix and no texture map.
   */
  static svtkActor* New();

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(svtkPropCollection*) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  //@{
  /**
   * Does this prop have some opaque/translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  svtkTypeBool HasOpaqueGeometry() override;
  //@}

  /**
   * This causes the actor to be rendered. It in turn will render the actor's
   * property, texture map and then mapper. If a property hasn't been
   * assigned, then the actor will create one automatically. Note that a side
   * effect of this method is that the pipeline will be updated.
   */
  virtual void Render(svtkRenderer*, svtkMapper*) {}

  /**
   * Shallow copy of an actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Set/Get the property object that controls this actors surface
   * properties.  This should be an instance of a svtkProperty object.  Every
   * actor must have a property associated with it.  If one isn't specified,
   * then one will be generated automatically. Multiple actors can share one
   * property object.
   */
  void SetProperty(svtkProperty* lut);
  svtkProperty* GetProperty();
  //@}

  /**
   * Create a new property suitable for use with this type of Actor.
   * For example, a svtkMesaActor should create a svtkMesaProperty
   * in this function.   The default is to just call svtkProperty::New.
   */
  virtual svtkProperty* MakeProperty();

  //@{
  /**
   * Set/Get the property object that controls this actors backface surface
   * properties.  This should be an instance of a svtkProperty object. If one
   * isn't specified, then the front face properties will be used.  Multiple
   * actors can share one property object.
   */
  void SetBackfaceProperty(svtkProperty* lut);
  svtkGetObjectMacro(BackfaceProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps.  This will
   * be a svtkTexture object. An actor does not need to have an associated
   * texture map and multiple actors can share one texture.
   */
  virtual void SetTexture(svtkTexture*);
  svtkGetObjectMacro(Texture, svtkTexture);
  //@}

  /**
   * This is the method that is used to connect an actor to the end of a
   * visualization pipeline, i.e. the mapper. This should be a subclass
   * of svtkMapper. Typically svtkPolyDataMapper and svtkDataSetMapper will
   * be used.
   */
  virtual void SetMapper(svtkMapper*);

  //@{
  /**
   * Returns the Mapper that this actor is getting its data from.
   */
  svtkGetObjectMacro(Mapper, svtkMapper);
  //@}

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  using Superclass::GetBounds;
  double* GetBounds() SVTK_SIZEHINT(6) override;

  /**
   * Apply the current properties to all parts that compose this actor.
   * This method is overloaded in svtkAssembly to apply the assemblies'
   * properties to all its parts in a recursive manner. Typically the
   * use of this method is to set the desired properties in the assembly,
   * and then push the properties down to the assemblies parts with
   * ApplyProperties().
   */
  virtual void ApplyProperties() {}

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, textures,
   * etc.
   */
  svtkMTimeType GetRedrawMTime() override;

  //@{
  /**
   * Force the actor to be treated as opaque or translucent
   */
  svtkGetMacro(ForceOpaque, bool);
  svtkSetMacro(ForceOpaque, bool);
  svtkBooleanMacro(ForceOpaque, bool);
  svtkGetMacro(ForceTranslucent, bool);
  svtkSetMacro(ForceTranslucent, bool);
  svtkBooleanMacro(ForceTranslucent, bool);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by svtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override;

  /**
   * allows a prop to update a selections color buffers
   * Default just forwards to the Mapper
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets) override;

  //@{
  // Get if we are in the translucent polygonal geometry pass
  bool IsRenderingTranslucentPolygonalGeometry() override { return this->InTranslucentPass; }
  void SetIsRenderingTranslucentPolygonalGeometry(bool val) { this->InTranslucentPass = val; }
  //@}

protected:
  svtkActor();
  ~svtkActor() override;

  // is this actor opaque
  int GetIsOpaque();
  bool ForceOpaque;
  bool ForceTranslucent;
  bool InTranslucentPass;

  svtkProperty* Property;
  svtkProperty* BackfaceProperty;
  svtkTexture* Texture;
  svtkMapper* Mapper;

  // Bounds are cached in an actor - the MapperBounds are also cache to
  // help know when the Bounds need to be recomputed.
  double MapperBounds[6];
  svtkTimeStamp BoundsMTime;

private:
  svtkActor(const svtkActor&) = delete;
  void operator=(const svtkActor&) = delete;
};

#endif
