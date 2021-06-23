/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStencilData
 * @brief   efficient description of an image stencil
 *
 * svtkImageStencilData describes an image stencil in a manner which is
 * efficient both in terms of speed and storage space.  The stencil extents
 * are stored for each x-row across the image (multiple extents per row if
 * necessary) and can be retrieved via the GetNextExtent() method.
 * @sa
 * svtkImageStencilSource svtkImageStencil
 */

#ifndef svtkImageStencilData_h
#define svtkImageStencilData_h

#include "svtkDataObject.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageStencilData : public svtkDataObject
{
public:
  static svtkImageStencilData* New();
  svtkTypeMacro(svtkImageStencilData, svtkDataObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize() override;
  void DeepCopy(svtkDataObject* o) override;
  void ShallowCopy(svtkDataObject* f) override;
  void InternalImageStencilDataCopy(svtkImageStencilData* s);

  /**
   * Get the data type as an integer (this will return SVTK_DATA_OBJECT
   * for now, maybe a proper type constant will be reserved later).
   */
  int GetDataObjectType() override { return SVTK_DATA_OBJECT; }

  /**
   * The extent type is 3D, just like svtkImageData.
   */
  int GetExtentType() override { return SVTK_3D_EXTENT; }

  /**
   * Given the total output x extent [xMin,xMax] and the current y, z indices,
   * return each sub-extent [r1,r2] that lies within within the unclipped
   * region in sequence.  A value of '0' is returned if no more sub-extents
   * are available.  The variable 'iter' must be initialized to zero before
   * the first call, unless you want the complementary sub-extents in which
   * case you must initialize 'iter' to -1.  The variable 'iter' is used
   * internally to keep track of which sub-extent should be returned next.
   */
  int GetNextExtent(int& r1, int& r2, int xMin, int xMax, int yIdx, int zIdx, int& iter);

  /**
   * Checks if an image index is inside the stencil.
   * Even though GetNextExtent and the svtkImageStencilIterator are faster
   * if every voxel in the volume has to be checked, IsInside provides an
   * efficient alternative for if just a single voxel has to be checked.
   */
  int IsInside(int xIdx, int yIdx, int zIdx);

  /**
   * This method is used by svtkImageStencilDataSource to add an x
   * sub extent [r1,r2] for the x row (yIdx,zIdx).  The specified sub
   * extent must not intersect any other sub extents along the same x row.
   * As well, r1 and r2 must both be within the total x extent
   * [Extent[0],Extent[1]].
   */
  void InsertNextExtent(int r1, int r2, int yIdx, int zIdx);

  /**
   * Similar to InsertNextExtent, except that the extent (r1,r2) at yIdx,
   * zIdx is merged with other extents, (if any) on that row. So a
   * unique extent may not necessarily be added. For instance, if an extent
   * [5,11] already exists adding an extent, [7,9] will not affect the
   * stencil. Likewise adding [10, 13] will replace the existing extent
   * with [5,13].
   */
  void InsertAndMergeExtent(int r1, int r2, int yIdx, int zIdx);

  /**
   * Remove the extent from (r1,r2) at yIdx, zIdx
   */
  void RemoveExtent(int r1, int r2, int yIdx, int zIdx);

  //@{
  /**
   * Set the desired spacing for the stencil.
   * This must be called before the stencil is Updated, ideally
   * in the ExecuteInformation method of the imaging filter that
   * is using the stencil.
   */
  svtkSetVector3Macro(Spacing, double);
  svtkGetVector3Macro(Spacing, double);
  //@}

  //@{
  /**
   * Set the desired origin for the stencil.
   * This must be called before the stencil is Updated, ideally
   * in the ExecuteInformation method of the imaging filter that
   * is using the stencil.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set the extent of the data.  This is should be called only
   * by svtkImageStencilSource, as it is part of the basic pipeline
   * functionality.
   */
  void SetExtent(const int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  svtkGetVector6Macro(Extent, int);
  //@}

  /**
   * Allocate space for the sub-extents.  This is called by
   * svtkImageStencilSource.
   */
  void AllocateExtents();

  /**
   * Fill the sub-extents.
   */
  void Fill();

  //@{
  /**
   * Override these to handle origin, spacing, scalar type, and scalar
   * number of components.  See svtkDataObject for details.
   */
  void CopyInformationFromPipeline(svtkInformation* info) override;
  void CopyInformationToPipeline(svtkInformation* info) override;
  //@}

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static svtkImageStencilData* GetData(svtkInformation* info);
  static svtkImageStencilData* GetData(svtkInformationVector* v, int i = 0);
  //@}

  /**
   * Add merges the stencil supplied as argument into Self.
   */
  virtual void Add(svtkImageStencilData*);

  /**
   * Subtract removes the portion of the stencil, supplied as argument,
   * that lies within Self from Self.
   */
  virtual void Subtract(svtkImageStencilData*);

  /**
   * Replaces the portion of the stencil, supplied as argument,
   * that lies within Self from Self.
   */
  virtual void Replace(svtkImageStencilData*);

  /**
   * Clip the stencil with the supplied extents. In other words, discard data
   * outside the specified extents. Return 1 if something changed.
   */
  virtual int Clip(int extent[6]);

protected:
  svtkImageStencilData();
  ~svtkImageStencilData() override;

  enum Operation
  {
    Merge,
    Erase
  };

  /**
   * Apply the given operation over the given (r1, r2) extent.
   */
  void LogicalOperationExtent(int r1, int r2, int yIdx, int zIdx, Operation operation);

  /**
   * Combine with the given stencil, using the given operation.
   */
  void LogicalOperationInPlace(svtkImageStencilData* stencil, Operation operation);

  /**
   * Change the extent while preserving the data.
   * This can be used to either expand or clip the extent.  The new extent
   * does not have to overlap the current extent.
   */
  void ChangeExtent(const int extent[6]);

  /**
   * Get important info from pipeline.
   */
  void CopyOriginAndSpacingFromPipeline(svtkInformation* info);

  //@{
  /**
   * The Spacing and Origin of the data.
   */
  double Spacing[3];
  double Origin[3];
  //@}

  int Extent[6];

  //@{
  /**
   * The actual 'data' is stored here.
   */
  int NumberOfExtentEntries;
  int* ExtentListLengths;
  int** ExtentLists;
  //@}

private:
  svtkImageStencilData(const svtkImageStencilData&) = delete;
  void operator=(const svtkImageStencilData&) = delete;

  friend class svtkImageStencilIteratorFriendship;
};

/**
 * This is a helper class for stencil creation.  It is a raster with
 * infinite resolution in the X direction (approximately, since it uses
 * double precision).  Lines that represent polygon edges can be drawn
 * into this raster, and then filled given a tolerance.
 */
class SVTKIMAGINGCORE_EXPORT svtkImageStencilRaster
{
public:
  /**
   * Create a raster with the specified whole y extent.
   */
  svtkImageStencilRaster(const int wholeExtent[2]);

  /**
   * Destructor.
   */
  ~svtkImageStencilRaster();

  /**
   * Reset the raster to its original state, but keep the same whole
   * extent. Pre-allocate the specified 1D allocateExtent, which must be
   * within the whole extent.
   */
  void PrepareForNewData(const int allocateExtent[2] = nullptr);

  //@{
  /**
   * Insert a line into the raster, given the two end points.
   */
  void InsertLine(const double p1[2], const double p2[2]);
  SVTK_LEGACY(void InsertLine(const double[2], const double[2], bool, bool));
  //@}

  /**
   * Fill the specified extent of a svtkImageStencilData with the raster,
   * after permuting the raster according to xj and yj.
   */
  void FillStencilData(svtkImageStencilData* data, const int extent[6], int xj = 0, int yj = 1);

  /**
   * The tolerance for float-to-int conversions.
   */
  void SetTolerance(double tol) { this->Tolerance = tol; }
  double GetTolerance() { return this->Tolerance; }

protected:
  /**
   * Ensure that the raster is initialized for the specified range
   * of y values, which must be within the Extent.
   */
  void PrepareExtent(int ymin, int ymax);

  /**
   * Insert an x point into the raster.  If the y value is larger than
   * the y extent, the extent will grow automatically.  The parameter i
   * indicates which of the two internal rasters is to be used.
   */
  void InsertPoint(int y, double x, int i);

  int Extent[2];
  int UsedExtent[2];
  double** Raster;
  double Tolerance;

private:
  svtkImageStencilRaster(const svtkImageStencilRaster&) = delete;
  void operator=(const svtkImageStencilRaster&) = delete;
};

#endif
