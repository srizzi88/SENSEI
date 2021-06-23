/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractCTHPart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractCTHPart
 * @brief   Generates surface of a CTH volume fraction.
 *
 * svtkExtractCTHPart is a filter that is specialized for creating
 * visualizations for a CTH simulation. CTH datasets comprise of either
 * svtkNonOverlappingAMR or a multiblock of non-overlapping rectilinear grids
 * with cell-data. Certain cell-arrays in the dataset identify the fraction of
 * a particular material present in a given cell. This goal with this filter is
 * to extract a surface contour demarcating the surface where the volume
 * fraction for a particular material is equal to the user chosen value.
 *
 * To achieve that, this filter first converts the cell-data to point-data and
 * then simply apply svtkContourFilter filter to extract the contour.
 *
 * svtkExtractCTHPart also provides the user with an option to clip the resultant
 * contour using a svtkPlane. Internally, it uses svtkClipClosedSurface to clip
 * the contour using the svtkPlane provided.
 *
 * The output of this filter is a svtkMultiBlockDataSet with one block
 * corresponding to each volume-fraction array requested. Each block itself is a
 * svtkPolyData for the contour generated on the current process (which may be
 * null, for processes where no contour is generated).
 */

#ifndef svtkExtractCTHPart_h
#define svtkExtractCTHPart_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // for using smartpointer

class svtkAppendPolyData;
class svtkContourFilter;
class svtkDataArray;
class svtkDataSet;
class svtkDataSetSurfaceFilter;
class svtkDoubleArray;
class svtkExtractCTHPartInternal;
class svtkImageData;
class svtkCompositeDataSet;
class svtkMultiProcessController;
class svtkPlane;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkUniformGrid;
class svtkUnsignedCharArray;
class svtkUnstructuredGrid;
class svtkExtractCTHPartFragments;

//#define EXTRACT_USE_IMAGE_DATA 1

class SVTKFILTERSPARALLEL_EXPORT svtkExtractCTHPart : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkExtractCTHPart* New();
  svtkTypeMacro(svtkExtractCTHPart, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Select cell-data arrays (volume-fraction arrays) to contour with.
   */
  void AddVolumeArrayName(const char*);
  void RemoveVolumeArrayNames();
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);
  //@}

  //@{
  /**
   * Get/Set the parallel controller. By default, the value returned by
   * svtkMultiBlockDataSetAlgorithm::GetGlobalController() when the object is
   * instantiated is used.
   */
  void SetController(svtkMultiProcessController* controller);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * On by default, enables logic to cap the material volume.
   */
  svtkSetMacro(Capping, bool);
  svtkGetMacro(Capping, bool);
  svtkBooleanMacro(Capping, bool);
  //@}

  //@{
  /**
   * Triangulate results. When set to false, the internal cut and contour filters
   * are told not to triangulate results if possible. true by default.
   */
  svtkSetMacro(GenerateTriangles, bool);
  svtkGetMacro(GenerateTriangles, bool);
  svtkBooleanMacro(GenerateTriangles, bool);
  //@}

  //@{
  /**
   * Generate solid geometry as results instead of 2D contours.
   * When set to true, GenerateTriangles flag will be ignored.
   * False by default.
   */
  svtkSetMacro(GenerateSolidGeometry, bool);
  svtkGetMacro(GenerateSolidGeometry, bool);
  svtkBooleanMacro(GenerateSolidGeometry, bool);
  //@}

  //@{
  /**
   * When set to false, the output surfaces will not hide contours extracted from
   * ghost cells. This results in overlapping contours but overcomes holes.
   * Default is set to true.
   */
  svtkSetMacro(RemoveGhostCells, bool);
  svtkGetMacro(RemoveGhostCells, bool);
  svtkBooleanMacro(RemoveGhostCells, bool);
  //@}

  //@{
  /**
   * Set, get or manipulate the implicit clipping plane.
   */
  void SetClipPlane(svtkPlane* clipPlane);
  svtkGetObjectMacro(ClipPlane, svtkPlane);
  //@}

  /**
   * Look at clip plane to compute MTime.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set and get the volume fraction surface value. This value should be
   * between 0 and 1
   */
  svtkSetClampMacro(VolumeFractionSurfaceValue, double, 0.0, 1.0);
  svtkGetMacro(VolumeFractionSurfaceValue, double);
  //@}

protected:
  svtkExtractCTHPart();
  ~svtkExtractCTHPart() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Compute the bounds over the composite dataset, some sub-dataset
   * can be on other processors. Returns false of communication failure.
   */
  bool ComputeGlobalBounds(svtkCompositeDataSet* input);

  /**
   * Extract contour for a particular array over the entire input dataset.
   * Returns false on error.
   */
  svtkSmartPointer<svtkDataSet> ExtractContour(svtkCompositeDataSet* input, const char* arrayName);

  /**
   * Extract solids (unstructuredGrids) for a particular array
   * over the entire input dataset. Returns false on error.
   */
  svtkSmartPointer<svtkDataSet> ExtractSolid(svtkCompositeDataSet* input, const char* arrayName);

  void ExecuteFaceQuads(svtkDataSet* input, svtkPolyData* output, int maxFlag, int originExtents[3],
    int ext[6], int aAxis, int bAxis, int cAxis);

  /**
   * Is block face on axis0 (either min or max depending on the maxFlag)
   * composed of only ghost cells?
   * \pre valid_axis0: axis0>=0 && axis0<=2
   */
  int IsGhostFace(int axis0, int maxFlag, int dims[3], svtkUnsignedCharArray* ghostArray);

  void TriggerProgressEvent(double val);

  int VolumeFractionType;
  double VolumeFractionSurfaceValue;
  double VolumeFractionSurfaceValueInternal;
  bool GenerateTriangles;
  bool GenerateSolidGeometry;
  bool Capping;
  bool RemoveGhostCells;
  svtkPlane* ClipPlane;
  svtkMultiProcessController* Controller;

private:
  svtkExtractCTHPart(const svtkExtractCTHPart&) = delete;
  void operator=(const svtkExtractCTHPart&) = delete;

  class VectorOfFragments;
  class VectorOfSolids;

  /**
   * Determine the true value to use for clipping based on the data-type.
   */
  inline void DetermineSurfaceValue(int dataType);

  /**
   * Extract contour for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractClippedContourOnBlock(
    svtkExtractCTHPart::VectorOfFragments& fragments, T* input, const char* arrayName);

  /**
   * Extract contour for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractContourOnBlock(
    svtkExtractCTHPart::VectorOfFragments& fragments, T* input, const char* arrayName);

  /**
   * Append quads for faces of the block that actually on the bounds
   * of the hierarchical dataset. Deals with ghost cells.
   */
  template <class T>
  void ExtractExteriorSurface(svtkExtractCTHPart::VectorOfFragments& fragments, T* input);

  /**
   * Extract clipped volume for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractClippedVolumeOnBlock(VectorOfSolids& solids, T* input, const char* arrayName);

  /**
   * Fast cell-data-2-point-data implementation.
   */
  void ExecuteCellDataToPointData(
    svtkDataArray* cellVolumeFraction, svtkDoubleArray* pointVolumeFraction, const int* dims);

  double ProgressShift;
  double ProgressScale;

  class ScaledProgress;
  friend class ScaledProgress;
  svtkExtractCTHPartInternal* Internals;
};
#endif
