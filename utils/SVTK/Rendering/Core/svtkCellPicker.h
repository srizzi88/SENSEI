/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellPicker
 * @brief   ray-cast cell picker for all kinds of Prop3Ds
 *
 * svtkCellPicker will shoot a ray into a 3D scene and return information
 * about the first object that the ray hits.  It works for all Prop3Ds.
 * For svtkVolume objects, it shoots a ray into the volume and returns
 * the point where the ray intersects an isosurface of a chosen opacity.
 * For svtkImage objects, it intersects the ray with the displayed
 * slice. For svtkActor objects, it intersects the actor's polygons.
 * If the object's mapper has ClippingPlanes, then it takes the clipping
 * into account, and will return the Id of the clipping plane that was
 * intersected.
 * For all prop types, it returns point and cell information, plus the
 * normal of the surface that was intersected at the pick position.  For
 * volumes and images, it also returns (i,j,k) coordinates for the point
 * and the cell that were picked.
 *
 * @sa
 * svtkPicker svtkPointPicker svtkVolumePicker
 *
 * @par Thanks:
 * This class was contributed to SVTK by David Gobbi on behalf of Atamai Inc.,
 * as an enhancement to the original svtkCellPicker.
 */

#ifndef svtkCellPicker_h
#define svtkCellPicker_h

#include "svtkPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkMapper;
class svtkTexture;
class svtkAbstractHyperTreeGridMapper;
class svtkAbstractVolumeMapper;
class svtkImageMapper3D;
class svtkPlaneCollection;
class svtkPiecewiseFunction;
class svtkDataArray;
class svtkDoubleArray;
class svtkIdList;
class svtkCell;
class svtkGenericCell;
class svtkImageData;
class svtkAbstractCellLocator;
class svtkCollection;
class svtkMatrix4x4;
class svtkBitArray;
class svtkHyperTreeGridNonOrientedGeometryCursor;

class SVTKRENDERINGCORE_EXPORT svtkCellPicker : public svtkPicker
{
public:
  static svtkCellPicker* New();
  svtkTypeMacro(svtkCellPicker, svtkPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values are the (x,y) pixel coordinates for the pick, and
   * the third value is z=0. The return value will be non-zero if
   * something was successfully picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer) override;

  /**
   * Perform pick operation with selection point provided. The
   * selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DRay(double selectionPt[3], double orient[4], svtkRenderer* ren) override;

  /**
   * Add a locator for one of the data sets that will be included in the
   * scene.  You must set up the locator with exactly the same data set
   * that was input to the mapper of one or more of the actors in the
   * scene.  As well, you must either build the locator before doing the
   * pick, or you must turn on LazyEvaluation in the locator to make it
   * build itself on the first pick.  Note that if you try to add the
   * same locator to the picker twice, the second addition will be ignored.
   */
  void AddLocator(svtkAbstractCellLocator* locator);

  /**
   * Remove a locator that was previously added.  If you try to remove a
   * nonexistent locator, then nothing will happen and no errors will be
   * raised.
   */
  void RemoveLocator(svtkAbstractCellLocator* locator);

  /**
   * Remove all locators associated with this picker.
   */
  void RemoveAllLocators();

  //@{
  /**
   * Set the opacity isovalue to use for defining volume surfaces.  The
   * pick will occur at the location along the pick ray where the
   * opacity of the volume is equal to this isovalue.  If you want to do
   * the pick based on an actual data isovalue rather than the opacity,
   * then pass the data value through the scalar opacity function before
   * using this method.
   */
  svtkSetMacro(VolumeOpacityIsovalue, double);
  svtkGetMacro(VolumeOpacityIsovalue, double);
  //@}

  //@{
  /**
   * Use the product of the scalar and gradient opacity functions when
   * computing the opacity isovalue, instead of just using the scalar
   * opacity. This parameter is only relevant to volume picking and
   * is off by default.
   */
  svtkSetMacro(UseVolumeGradientOpacity, svtkTypeBool);
  svtkBooleanMacro(UseVolumeGradientOpacity, svtkTypeBool);
  svtkGetMacro(UseVolumeGradientOpacity, svtkTypeBool);
  //@}

  //@{
  /**
   * The PickClippingPlanes setting controls how clipping planes are
   * handled by the pick.  If it is On, then the clipping planes become
   * pickable objects, even though they are usually invisible.  This
   * means that if the pick ray intersects a clipping plane before it
   * hits anything else, the pick will stop at that clipping plane.
   * The GetProp3D() and GetMapper() methods will return the Prop3D
   * and Mapper that the clipping plane belongs to.  The
   * GetClippingPlaneId() method will return the index of the clipping
   * plane so that you can retrieve it from the mapper, or -1 if no
   * clipping plane was picked.
   */
  svtkSetMacro(PickClippingPlanes, svtkTypeBool);
  svtkBooleanMacro(PickClippingPlanes, svtkTypeBool);
  svtkGetMacro(PickClippingPlanes, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the index of the clipping plane that was intersected during
   * the pick.  This will be set regardless of whether PickClippingPlanes
   * is On, all that is required is that the pick intersected a clipping
   * plane of the Prop3D that was picked.  The result will be -1 if the
   * Prop3D that was picked has no clipping planes, or if the ray didn't
   * intersect the planes.
   */
  svtkGetMacro(ClippingPlaneId, int);
  //@}

  //@{
  /**
   * Return the normal of the picked surface at the PickPosition.  If no
   * surface was picked, then a vector pointing back at the camera is
   * returned.
   */
  svtkGetVectorMacro(PickNormal, double, 3);
  //@}

  //@{
  /**
   * Return the normal of the surface at the PickPosition in mapper
   * coordinates.  The result is undefined if no prop was picked.
   */
  svtkGetVector3Macro(MapperNormal, double);
  //@}

  //@{
  /**
   * Get the structured coordinates of the point at the PickPosition.
   * Only valid for image actors and volumes with svtkImageData.
   */
  svtkGetVector3Macro(PointIJK, int);
  //@}

  //@{
  /**
   * Get the structured coordinates of the cell at the PickPosition.
   * Only valid for image actors and volumes with svtkImageData.
   * Combine this with the PCoords to get the position within the cell.
   */
  svtkGetVector3Macro(CellIJK, int);
  //@}

  //@{
  /**
   * Get the id of the picked point. If PointId = -1, nothing was picked.
   * This point will be a member of any cell that is picked.
   */
  svtkGetMacro(PointId, svtkIdType);
  //@}

  //@{
  /**
   * Get the id of the picked cell. If CellId = -1, nothing was picked.
   */
  svtkGetMacro(CellId, svtkIdType);
  //@}

  //@{
  /**
   * Get the subId of the picked cell. This is useful, for example, if
   * the data is made of triangle strips. If SubId = -1, nothing was picked.
   */
  svtkGetMacro(SubId, int);
  //@}

  //@{
  /**
   * Get the parametric coordinates of the picked cell. Only valid if
   * a prop was picked.  The PCoords can be used to compute the weights
   * that are needed to interpolate data values within the cell.
   */
  svtkGetVector3Macro(PCoords, double);
  //@}

  /**
   * Get the texture that was picked.  This will always be set if the
   * picked prop has a texture, and will always be null otherwise.
   */
  svtkTexture* GetTexture() { return this->Texture; }

  //@{
  /**
   * If this is "On" and if the picked prop has a texture, then the data
   * returned by GetDataSet() will be the texture's data instead of the
   * mapper's data.  The GetPointId(), GetCellId(), GetPCoords() etc. will
   * all return information for use with the texture's data.  If the picked
   * prop does not have any texture, then GetDataSet() will return the
   * mapper's data instead and GetPointId() etc. will return information
   * related to the mapper's data.  The default value of PickTextureData
   * is "Off".
   */
  svtkSetMacro(PickTextureData, svtkTypeBool);
  svtkBooleanMacro(PickTextureData, svtkTypeBool);
  svtkGetMacro(PickTextureData, svtkTypeBool);
  //@}

protected:
  svtkCellPicker();
  ~svtkCellPicker() override;

  void Initialize() override;

  virtual void ResetPickInfo();

  double IntersectWithLine(const double p1[3], const double p2[3], double tol,
    svtkAssemblyPath* path, svtkProp3D* p, svtkAbstractMapper3D* m) override;

  virtual double IntersectActorWithLine(const double p1[3], const double p2[3], double t1,
    double t2, double tol, svtkProp3D* prop, svtkMapper* mapper);

  virtual bool IntersectDataSetWithLine(svtkDataSet* dataSet, const double p1[3], const double p2[3],
    double t1, double t2, double tol, svtkAbstractCellLocator*& locator, svtkIdType& cellId,
    int& subId, double& tMin, double& pDistMin, double xyz[3], double minPCoords[3]);

  //@{
  /**
   * Intersect a svtkAbstractHyperTreeGridMapper with a line by ray casting.
   */
  virtual double IntersectHyperTreeGridWithLine(
    const double[3], const double[3], double, double, svtkAbstractHyperTreeGridMapper*);
  virtual bool RecursivelyProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor*, int);
  //@}

  virtual double IntersectVolumeWithLine(const double p1[3], const double p2[3], double t1,
    double t2, svtkProp3D* prop, svtkAbstractVolumeMapper* mapper);

  virtual double IntersectImageWithLine(const double p1[3], const double p2[3], double t1,
    double t2, svtkProp3D* prop, svtkImageMapper3D* mapper);

  virtual double IntersectProp3DWithLine(const double p1[3], const double p2[3], double t1,
    double t2, double tol, svtkProp3D* prop, svtkAbstractMapper3D* mapper);

  static int ClipLineWithPlanes(svtkAbstractMapper3D* mapper, svtkMatrix4x4* propMatrix,
    const double p1[3], const double p2[3], double& t1, double& t2, int& planeId);

  static int ClipLineWithExtent(const int extent[6], const double x1[3], const double x2[3],
    double& t1, double& t2, int& planeId);

  static int ComputeSurfaceNormal(
    svtkDataSet* data, svtkCell* cell, const double* weights, double normal[3]);

  static int ComputeSurfaceTCoord(
    svtkDataSet* data, svtkCell* cell, const double* weights, double tcoord[3]);

  static int HasSubCells(int cellType);

  static int GetNumberOfSubCells(svtkIdList* pointIds, int cellType);

  static void GetSubCell(
    svtkDataSet* data, svtkIdList* pointIds, int subId, int cellType, svtkGenericCell* cell);

  static void SubCellFromCell(svtkGenericCell* cell, int subId);

  void SetImageDataPickInfo(const double x[3], const int extent[6]);

  double ComputeVolumeOpacity(const int xi[3], const double pcoords[3], svtkImageData* data,
    svtkDataArray* scalars, svtkPiecewiseFunction* scalarOpacity,
    svtkPiecewiseFunction* gradientOpacity);

  svtkCollection* Locators;

  double VolumeOpacityIsovalue;
  svtkTypeBool UseVolumeGradientOpacity;
  svtkTypeBool PickClippingPlanes;
  int ClippingPlaneId;

  svtkIdType PointId;
  svtkIdType CellId;
  int SubId;
  double PCoords[3];

  int PointIJK[3];
  int CellIJK[3];

  double PickNormal[3];
  double MapperNormal[3];

  svtkTexture* Texture;
  svtkTypeBool PickTextureData;

  svtkBitArray* InMask;
  double WordlPoint[3];

private:
  void ResetCellPickerInfo();

  svtkGenericCell* Cell;      // used to accelerate picking
  svtkIdList* PointIds;       // used to accelerate picking
  svtkDoubleArray* Gradients; // used in volume picking

private:
  svtkCellPicker(const svtkCellPicker&) = delete;
  void operator=(const svtkCellPicker&) = delete;
};

#endif
