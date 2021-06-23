/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCirclePackToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkCirclePackToPolyData
 * @brief   converts a tree to a polygonal data
 * representing a circle packing of the hierarchy.
 *
 *
 * This algorithm requires that the svtkCirclePackLayout filter has already
 * been applied to the data in order to create the triple array
 * (Xcenter, Ycenter, Radius) of circle bounds or each vertex of the tree.
 */

#ifndef svtkCirclePackToPolyData_h
#define svtkCirclePackToPolyData_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKINFOVISLAYOUT_EXPORT svtkCirclePackToPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkCirclePackToPolyData* New();

  svtkTypeMacro(svtkCirclePackToPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The field containing triples of the form (Xcenter, Ycenter, Radius).

   * This field may be added to the tree using svtkCirclePackLayout.
   * This array must be set.
   */
  virtual void SetCirclesArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  //@{
  /**
   * Define the number of sides used in output circles.
   * Default is 100.
   */
  svtkSetMacro(Resolution, unsigned int);
  svtkGetMacro(Resolution, unsigned int);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkCirclePackToPolyData();
  ~svtkCirclePackToPolyData() override;

  unsigned int Resolution;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkCirclePackToPolyData(const svtkCirclePackToPolyData&) = delete;
  void operator=(const svtkCirclePackToPolyData&) = delete;
  void CreateCircle(const double& x, const double& y, const double& z, const double& radius,
    const int& resolution, svtkPolyData* polyData);
};

#endif
