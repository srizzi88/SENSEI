/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveHiddenData.h

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
 * @class   svtkRemoveHiddenData
 * @brief   Removes the rows/edges/vertices of
 * input data flagged by ann.
 *
 *
 * Output only those rows/vertices/edges of the input svtkDataObject that
 * are visible, as defined by the svtkAnnotation::HIDE() flag of the input
 * svtkAnnotationLayers.
 * Inputs:
 *    Port 0 - svtkDataObject
 *    Port 1 - svtkAnnotationLayers (optional)
 *
 */

#ifndef svtkRemoveHiddenData_h
#define svtkRemoveHiddenData_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"
#include "svtkSmartPointer.h" // For Smartpointer

class svtkExtractSelectedGraph;
class svtkExtractSelectedRows;

class SVTKINFOVISCORE_EXPORT svtkRemoveHiddenData : public svtkPassInputTypeAlgorithm
{
public:
  static svtkRemoveHiddenData* New();
  svtkTypeMacro(svtkRemoveHiddenData, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkRemoveHiddenData();
  ~svtkRemoveHiddenData() override;

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRemoveHiddenData(const svtkRemoveHiddenData&) = delete;
  void operator=(const svtkRemoveHiddenData&) = delete;

  svtkSmartPointer<svtkExtractSelectedGraph> ExtractGraph;
  svtkSmartPointer<svtkExtractSelectedRows> ExtractTable;
};

#endif
