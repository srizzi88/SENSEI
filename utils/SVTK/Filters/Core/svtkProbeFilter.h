/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProbeFilter
 * @brief   sample data values at specified point locations
 *
 * svtkProbeFilter is a filter that computes point attributes (e.g., scalars,
 * vectors, etc.) at specified point positions. The filter has two inputs:
 * the Input and Source. The Input geometric structure is passed through the
 * filter. The point attributes are computed at the Input point positions
 * by interpolating into the source data. For example, we can compute data
 * values on a plane (plane specified as Input) from a volume (Source).
 * The cell data of the source data is copied to the output based on in
 * which source cell each input point is. If an array of the same name exists
 * both in source's point and cell data, only the one from the point data is
 * probed.
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, an unstructured grid (svtkUnstructuredGrid) can be
 * probed with a volume (three-dimensional svtkImageData), and then volume
 * rendering techniques can be used to visualize the results. Another example:
 * a line or curve can be used to probe data to produce x-y plots along
 * that line or curve.
 *
 * @warning
 * A critical algorithmic component of svtkProbeFilter is the manner in which
 * it finds the cell containing a probe point. By default, the
 * svtkDataSet::FindCell() method is used, which in turn uses a
 * svtkPointLocator to perform an accelerated search. However, using a
 * svtkPointLocator may fail to identify an enclosing cell in some cases. A
 * more robust but slower approach is to use a svtkCellLocator to perform the
 * the FindCell() operation (via specification of the
 * CellLocatorPrototype). Finally, more advanced searches can be configured
 * by specifying an instance of svtkFindCellStrategy. (Note: image data
 * probing never uses a locator since finding a containing cell is a simple,
 * fast operation. This specifying a svtkFindCellStrategy or cell locator
 * prototype has no effect.)
 *
 * @warning
 * The svtkProbeFilter, once it finds the cell containing a query point, uses
 * the cell's interpolation functions to perform the interpolate / compute
 * the point attributes. Note that other interpolation processes with
 * different kernels are available: svtkPointInterpolator and
 * svtkSPHInterpolator. svtkPointInterpolator supports a variety of generalized
 * kernels, while svtkSPHInterpolator supports a variety of SPH interpolation
 * kernels.
 *
 * @sa
 * svtkFindCellStrategy svtkPointLocator svtkCellLocator svtkStaticPointLocator
 * svtkStaticCellLocator svtkPointInterpolator svtkSPHInterpolator
 */

#ifndef svtkProbeFilter_h
#define svtkProbeFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkDataSetAttributes.h" // needed for svtkDataSetAttributes::FieldList
#include "svtkFiltersCoreModule.h" // For export macro

class svtkAbstractCellLocator;
class svtkCell;
class svtkCharArray;
class svtkIdTypeArray;
class svtkImageData;
class svtkPointData;
class svtkFindCellStrategy;

class SVTKFILTERSCORE_EXPORT svtkProbeFilter : public svtkDataSetAlgorithm
{
public:
  static svtkProbeFilter* New();
  svtkTypeMacro(svtkProbeFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(svtkDataObject* source);
  svtkDataObject* GetSource();
  //@}

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Control whether the source point data is to be treated as categorical. If
   * the data is categorical, then the resultant data will be determined by
   * a nearest neighbor interpolation scheme.
   */
  svtkSetMacro(CategoricalData, svtkTypeBool);
  svtkGetMacro(CategoricalData, svtkTypeBool);
  svtkBooleanMacro(CategoricalData, svtkTypeBool);
  //@}

  //@{
  /**
   * This flag is used only when a piece is requested to update.  By default
   * the flag is off.  Because no spatial correspondence between input pieces
   * and source pieces is known, all of the source has to be requested no
   * matter what piece of the output is requested.  When there is a spatial
   * correspondence, the user/application can set this flag.  This hint allows
   * the breakup of the probe operation to be much more efficient.  When piece
   * m of n is requested for update by the user, then only n of m needs to
   * be requested of the source.
   */
  svtkSetMacro(SpatialMatch, svtkTypeBool);
  svtkGetMacro(SpatialMatch, svtkTypeBool);
  svtkBooleanMacro(SpatialMatch, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the list of point ids in the output that contain attribute data
   * interpolated from the source.
   */
  svtkIdTypeArray* GetValidPoints();
  //@}

  //@{
  /**
   * Returns the name of the char array added to the output with values 1 for
   * valid points and 0 for invalid points.
   * Set to "svtkValidPointMask" by default.
   */
  svtkSetStringMacro(ValidPointMaskArrayName);
  svtkGetStringMacro(ValidPointMaskArrayName);
  //@}

  //@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  svtkSetMacro(PassCellArrays, svtkTypeBool);
  svtkBooleanMacro(PassCellArrays, svtkTypeBool);
  svtkGetMacro(PassCellArrays, svtkTypeBool);
  //@}
  //@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  svtkSetMacro(PassPointArrays, svtkTypeBool);
  svtkBooleanMacro(PassPointArrays, svtkTypeBool);
  svtkGetMacro(PassPointArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  svtkSetMacro(PassFieldArrays, svtkTypeBool);
  svtkBooleanMacro(PassFieldArrays, svtkTypeBool);
  svtkGetMacro(PassFieldArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. On by default.
   */
  svtkSetMacro(ComputeTolerance, bool);
  svtkBooleanMacro(ComputeTolerance, bool);
  svtkGetMacro(ComputeTolerance, bool);
  //@}

  //@{
  /**
   * Set / get the strategy used to perform the FindCell() operation. When
   * specified, the strategy is used in preference to a cell locator
   * prototype. When neither a strategy or cell locator prototype is defined,
   * then the svtkDataSet::FindCell() method is used.
   */
  virtual void SetFindCellStrategy(svtkFindCellStrategy*);
  svtkGetObjectMacro(FindCellStrategy, svtkFindCellStrategy);
  //@}

  //@{
  /**
   * Set/Get the prototype cell locator to perform the FindCell() operation.
   * (A prototype is used as an object factory to instantiate an instance of
   * the prototype to perform the FindCell() operation). If a prototype, and
   * a svtkFindCellStrategy are not defined, the svtkDataSet::FindCell() is
   * used. If a svtkFindCellStrategy is not defined, then the prototype is
   * used.
   */
  virtual void SetCellLocatorPrototype(svtkAbstractCellLocator*);
  svtkGetObjectMacro(CellLocatorPrototype, svtkAbstractCellLocator);
  //@}

protected:
  svtkProbeFilter();
  ~svtkProbeFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Call at end of RequestData() to pass attribute data respecting the
   * PassCellArrays, PassPointArrays, PassFieldArrays flags.
   */
  void PassAttributeData(svtkDataSet* input, svtkDataObject* source, svtkDataSet* output);

  /**
   * Equivalent to calling BuildFieldList(); InitializeForProbing(); DoProbing().
   */
  void Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output);

  /**
   * Build the field lists. This is required before calling
   * InitializeForProbing().
   */
  void BuildFieldList(svtkDataSet* source);

  /**
   * Initializes output and various arrays which keep track for probing status.
   */
  virtual void InitializeForProbing(svtkDataSet* input, svtkDataSet* output);
  virtual void InitializeOutputArrays(svtkPointData* outPD, svtkIdType numPts);

  /**
   * Probe appropriate points
   * srcIdx is the index in the PointList for the given source.
   */
  void DoProbing(svtkDataSet* input, int srcIdx, svtkDataSet* source, svtkDataSet* output);

  svtkTypeBool CategoricalData;

  svtkTypeBool PassCellArrays;
  svtkTypeBool PassPointArrays;
  svtkTypeBool PassFieldArrays;

  svtkTypeBool SpatialMatch;

  double Tolerance;
  bool ComputeTolerance;

  char* ValidPointMaskArrayName;
  svtkIdTypeArray* ValidPoints;
  svtkCharArray* MaskPoints;

  // Support various methods to support the FindCell() operation
  svtkAbstractCellLocator* CellLocatorPrototype;
  svtkFindCellStrategy* FindCellStrategy;

  svtkDataSetAttributes::FieldList* CellList;
  svtkDataSetAttributes::FieldList* PointList;

private:
  svtkProbeFilter(const svtkProbeFilter&) = delete;
  void operator=(const svtkProbeFilter&) = delete;

  // Probe only those points that are marked as not-probed by the MaskPoints
  // array.
  void ProbeEmptyPoints(svtkDataSet* input, int srcIdx, svtkDataSet* source, svtkDataSet* output);

  // A faster implementation for svtkImageData input.
  void ProbePointsImageData(
    svtkImageData* input, int srcIdx, svtkDataSet* source, svtkImageData* output);
  void ProbeImagePointsInCell(svtkCell* cell, svtkIdType cellId, svtkDataSet* source, int srcBlockId,
    const double start[3], const double spacing[3], const int dim[3], svtkPointData* outPD,
    char* maskArray, double* wtsBuff);

  class ProbeImageDataWorklet;

  class svtkVectorOfArrays;
  svtkVectorOfArrays* CellArrays;
};

#endif
