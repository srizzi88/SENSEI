/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLODProp3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLODProp3D
 * @brief   level of detail 3D prop
 *
 * svtkLODProp3D is a class to support level of detail rendering for Prop3D.
 * Any number of mapper/property/texture items can be added to this object.
 * Render time will be measured, and will be used to select a LOD based on
 * the AllocatedRenderTime of this Prop3D. Depending on the type of the
 * mapper/property, a svtkActor or a svtkVolume will be created behind the
 * scenes.
 *
 * @sa
 * svtkProp3D svtkActor svtkVolume svtkLODActor
 */

#ifndef svtkLODProp3D_h
#define svtkLODProp3D_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkMapper;
class svtkAbstractVolumeMapper;
class svtkAbstractMapper3D;
class svtkImageMapper3D;
class svtkProperty;
class svtkVolumeProperty;
class svtkImageProperty;
class svtkTexture;
class svtkLODProp3DCallback;

typedef struct
{
  svtkProp3D* Prop3D;
  int Prop3DType;
  int ID;
  double EstimatedTime;
  int State;
  double Level;
} svtkLODProp3DEntry;

class SVTKRENDERINGCORE_EXPORT svtkLODProp3D : public svtkProp3D
{
public:
  /**
   * Create an instance of this class.
   */
  static svtkLODProp3D* New();

  svtkTypeMacro(svtkLODProp3D, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Standard svtkProp method to get 3D bounds of a 3D prop
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) { this->svtkProp3D::GetBounds(bounds); }

  //@{
  /**
   * Add a level of detail with a given mapper, property, backface property,
   * texture, and guess of rendering time.  The property and texture fields
   * can be set to NULL (the other methods are included for script access
   * where null variables are not allowed). The time field can be set to 0.0
   * indicating that no initial guess for rendering time is being supplied.
   * The returned integer value is an ID that can be used later to delete
   * this LOD, or set it as the selected LOD.
   */
  int AddLOD(svtkMapper* m, svtkProperty* p, svtkProperty* back, svtkTexture* t, double time);
  int AddLOD(svtkMapper* m, svtkProperty* p, svtkTexture* t, double time);
  int AddLOD(svtkMapper* m, svtkProperty* p, svtkProperty* back, double time);
  int AddLOD(svtkMapper* m, svtkProperty* p, double time);
  int AddLOD(svtkMapper* m, svtkTexture* t, double time);
  int AddLOD(svtkMapper* m, double time);
  int AddLOD(svtkAbstractVolumeMapper* m, svtkVolumeProperty* p, double time);
  int AddLOD(svtkAbstractVolumeMapper* m, double time);
  int AddLOD(svtkImageMapper3D* m, svtkImageProperty* p, double time);
  int AddLOD(svtkImageMapper3D* m, double time);
  //@}

  //@{
  /**
   * Get the current number of LODs.
   */
  svtkGetMacro(NumberOfLODs, int);
  //@}

  //@{
  /**
   * Get the current index, used to determine the ID of the next LOD that is
   * added.  Useful for guessing what IDs have been used (with NumberOfLODs,
   * without depending on the constructor initialization to 1000.
   */
  svtkGetMacro(CurrentIndex, int);
  //@}

  /**
   * Delete a level of detail given an ID. This is the ID returned by the
   * AddLOD method
   */
  void RemoveLOD(int id);

  //@{
  /**
   * Methods to set / get the property of an LOD. Since the LOD could be
   * a volume or an actor, you have to pass in the pointer to the property
   * to get it. The returned property will be NULL if the id is not valid,
   * or the property is of the wrong type for the corresponding Prop3D.
   */
  void SetLODProperty(int id, svtkProperty* p);
  void GetLODProperty(int id, svtkProperty** p);
  void SetLODProperty(int id, svtkVolumeProperty* p);
  void GetLODProperty(int id, svtkVolumeProperty** p);
  void SetLODProperty(int id, svtkImageProperty* p);
  void GetLODProperty(int id, svtkImageProperty** p);
  //@}

  //@{
  /**
   * Methods to set / get the mapper of an LOD. Since the LOD could be
   * a volume or an actor, you have to pass in the pointer to the mapper
   * to get it. The returned mapper will be NULL if the id is not valid,
   * or the mapper is of the wrong type for the corresponding Prop3D.
   */
  void SetLODMapper(int id, svtkMapper* m);
  void GetLODMapper(int id, svtkMapper** m);
  void SetLODMapper(int id, svtkAbstractVolumeMapper* m);
  void GetLODMapper(int id, svtkAbstractVolumeMapper** m);
  void SetLODMapper(int id, svtkImageMapper3D* m);
  void GetLODMapper(int id, svtkImageMapper3D** m);
  //@}

  /**
   * Get the LODMapper as an svtkAbstractMapper3D.  It is the user's
   * respondibility to safe down cast this to a svtkMapper or svtkVolumeMapper
   * as appropriate.
   */
  svtkAbstractMapper3D* GetLODMapper(int id);

  //@{
  /**
   * Methods to set / get the backface property of an LOD. This method is only
   * valid for LOD ids that are Actors (not Volumes)
   */
  void SetLODBackfaceProperty(int id, svtkProperty* t);
  void GetLODBackfaceProperty(int id, svtkProperty** t);
  //@}

  //@{
  /**
   * Methods to set / get the texture of an LOD. This method is only
   * valid for LOD ids that are Actors (not Volumes)
   */
  void SetLODTexture(int id, svtkTexture* t);
  void GetLODTexture(int id, svtkTexture** t);
  //@}

  //@{
  /**
   * Enable / disable a particular LOD. If it is disabled, it will not
   * be used during automatic selection, but can be selected as the
   * LOD if automatic LOD selection is off.
   */
  void EnableLOD(int id);
  void DisableLOD(int id);
  int IsLODEnabled(int id);
  //@}

  //@{
  /**
   * Set the level of a particular LOD. When a LOD is selected for
   * rendering because it has the largest render time that fits within
   * the allocated time, all LOD are then checked to see if any one can
   * render faster but has a lower (more resolution/better) level.
   * This quantity is a double to ensure that a level can be inserted
   * between 2 and 3.
   */
  void SetLODLevel(int id, double level);
  double GetLODLevel(int id);
  double GetLODIndexLevel(int index);
  //@}

  //@{
  /**
   * Access method that can be used to find out the estimated render time
   * (the thing used to select an LOD) for a given LOD ID or index.
   * Value is returned in seconds.
   */
  double GetLODEstimatedRenderTime(int id);
  double GetLODIndexEstimatedRenderTime(int index);
  //@}

  //@{
  /**
   * Turn on / off automatic selection of LOD.
   * This is on by default. If it is off, then the SelectedLODID is
   * rendered regardless of rendering time or desired update rate.
   */
  svtkSetClampMacro(AutomaticLODSelection, svtkTypeBool, 0, 1);
  svtkGetMacro(AutomaticLODSelection, svtkTypeBool);
  svtkBooleanMacro(AutomaticLODSelection, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the id of the LOD that is to be drawn when automatic LOD selection
   * is turned off.
   */
  svtkSetMacro(SelectedLODID, int);
  svtkGetMacro(SelectedLODID, int);
  //@}

  /**
   * Get the ID of the previously (during the last render) selected LOD index
   */
  int GetLastRenderedLODID();

  /**
   * Get the ID of the appropriate pick LOD index
   */
  int GetPickLODID(void);

  //@{
  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(svtkPropCollection*) override;
  void GetVolumes(svtkPropCollection*) override;
  //@}

  //@{
  /**
   * Set the id of the LOD that is to be used for picking when automatic
   * LOD pick selection is turned off.
   */
  void SetSelectedPickLODID(int id);
  svtkGetMacro(SelectedPickLODID, int);
  //@}

  //@{
  /**
   * Turn on / off automatic selection of picking LOD.
   * This is on by default. If it is off, then the SelectedLODID is
   * rendered regardless of rendering time or desired update rate.
   */
  svtkSetClampMacro(AutomaticPickLODSelection, svtkTypeBool, 0, 1);
  svtkGetMacro(AutomaticPickLODSelection, svtkTypeBool);
  svtkBooleanMacro(AutomaticPickLODSelection, svtkTypeBool);
  //@}

  /**
   * Shallow copy of this svtkLODProp3D.
   */
  void ShallowCopy(svtkProp* prop) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* ren) override;
  int RenderVolumetricGeometry(svtkViewport* ren) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Used by the culler / renderer to set the allocated render time for this
   * prop. This is based on the desired update rate, and possibly some other
   * properties such as potential screen coverage of this prop.
   */
  void SetAllocatedRenderTime(double t, svtkViewport* vp) override;

  /**
   * Used when the render process is aborted to restore the previous
   * estimated render time. Overridden here to allow previous time for a
   * particular LOD to be restored - otherwise the time for the last rendered
   * LOD will be copied into the currently selected LOD.
   */
  void RestoreEstimatedRenderTime() override;

  /**
   * Override method from svtkProp in order to push this call down to the
   * selected LOD as well.
   */
  void AddEstimatedRenderTime(double t, svtkViewport* vp) override;

protected:
  svtkLODProp3D();
  ~svtkLODProp3D() override;

  int GetAutomaticPickPropIndex(void);

  // Assumes that SelectedLODIndex has already been validated:
  void UpdateKeysForSelectedProp();

  svtkLODProp3DEntry* LODs;
  int NumberOfEntries;
  int NumberOfLODs;
  int CurrentIndex;

  int GetNextEntryIndex();
  int ConvertIDToIndex(int id);
  int SelectedLODIndex;

  svtkTypeBool AutomaticLODSelection;
  int SelectedLODID;
  int SelectedPickLODID;
  svtkTypeBool AutomaticPickLODSelection;
  svtkLODProp3DCallback* PickCallback;

private:
  svtkLODProp3D(const svtkLODProp3D&) = delete;
  void operator=(const svtkLODProp3D&) = delete;
};

#endif
