//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   svtkmProbe
 * @brief   Sample data at specified point locations
 *
 * svtkmProbe is a filter that computes point attributes(e.g., scalars, vectors,
 * etc.) at specific point positions using the probe filter in SVTK-m. The
 * filter has two inputs: the Input and Source.
 * The Input geometric structure is passed through the filter. The point
 * attributes are computed at the Input point positions by interpolating into
 * the source data. For example, we can compute data values on a plane(plane
 * specified as Input from a volume(Source). The source geometry must have cellSet
 * defined otherwise the svtkm filter won't work. The cell data of the source data
 * is copied to the output based on in which source cell each input point is. If
 * an array of the same name exists both in source's point and cell data, only
 * the one from the point data is probed. The valid point result is stored as
 * a field array whose default name is "svtkValidPointMask" in the point data and
 * the valid cell result(Invalid cells are the cells with at least one invalid
 * point) is stored as a field array whose default name is "svtkValidCellMask" in
 * the cell data.
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, an unstructured grid (svtkUnstructuredGrid) can be
 * probed with a volume (three-dimensional svtkImageData), and then volume
 * rendering techniques can be used to visualize the results. Another example:
 * a line or curve can be used to probe data to produce x-y plots along
 * that line or curve.
 */

#ifndef svtkmProbe_h
#define svtkmProbe_h

#include <string> // for std::string

#include "svtkAcceleratorsSVTKmModule.h" //required for export
#include "svtkDataSetAlgorithm.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmProbe : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkmProbe, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmProbe* New();

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

  //@}
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);
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
   * Shallow copy the input point data arrays to the output.
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
   * Returns the name of the valid point array added to the output with values 2 for
   * hidden points and 0 for valid points.
   * Set to "svtkValidPointMask" by default.
   */
  svtkSetMacro(ValidPointMaskArrayName, std::string);
  svtkGetMacro(ValidPointMaskArrayName, std::string);
  //@}

  //@{
  /**
   * Returns the name of the valid cell array added to the output with values 2 for
   * hidden points and 0 for valid points.
   * Set to "svtkValidCellMask" by default.
   */
  svtkSetMacro(ValidCellMaskArrayName, std::string);
  svtkGetMacro(ValidCellMaskArrayName, std::string);
  //@}

protected:
  svtkmProbe();
  ~svtkmProbe() override = default;

  svtkTypeBool PassCellArrays;
  svtkTypeBool PassPointArrays;
  svtkTypeBool PassFieldArrays;
  std::string ValidPointMaskArrayName;
  std::string ValidCellMaskArrayName;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Call at the end of RequestData() to pass attribute dat a respecting the
   * PassCellArrays, PassPointArrays and PassFieldArrays flag
   */
  void PassAttributeData(svtkDataSet* input, svtkDataObject* source, svtkDataSet* output);

private:
  svtkmProbe(const svtkmProbe&) = delete;
  void operator=(const svtkmProbe&) = delete;
};

#endif // svtkmProbe_h
// SVTK-HeaderTest-Exclude: svtkmProbe.h
