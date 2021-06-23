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
 * @class   svtkmExternalFaces
 * @brief   generate External Faces of a DataSet
 *
 * svtkmExternalFaces is a filter that extracts all external faces from a
 * data set. An external face is defined is defined as a face/side of a cell
 * that belongs only to one cell in the entire mesh.
 * @warning
 * This filter is currently only supports propagation of point properties
 *
 */

#ifndef svtkmExternalFaces_h
#define svtkmExternalFaces_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkAlgorithm.h"

class svtkUnstructuredGrid;

class SVTKACCELERATORSSVTKM_EXPORT svtkmExternalFaces : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkmExternalFaces, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmExternalFaces* New();

  /**
   * Set the input DataSet
   */
  void SetInputData(svtkUnstructuredGrid* ds);

  /**
   * Get the resulr DataSet
   */
  svtkUnstructuredGrid* GetOutput();

  //@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  svtkSetMacro(CompactPoints, bool);
  svtkGetMacro(CompactPoints, bool);
  svtkBooleanMacro(CompactPoints, bool);
  //@}

protected:
  svtkmExternalFaces();
  ~svtkmExternalFaces() override;

  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  bool CompactPoints;

private:
  svtkmExternalFaces(const svtkmExternalFaces&) = delete;
  void operator=(const svtkmExternalFaces&) = delete;
};

#endif // svtkmExternalFaces_h
// SVTK-HeaderTest-Exclude: svtkmExternalFaces.h
