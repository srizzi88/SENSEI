/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPointCloudPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractPointCloudPiece
 * @brief   Return a piece of a point cloud
 *
 * This filter takes the output of a svtkHierarchicalBinningFilter and allows
 * the pipeline to stream it. Pieces are determined from an offset integral
 * array associated with the field data of the input.
 */

#ifndef svtkExtractPointCloudPiece_h
#define svtkExtractPointCloudPiece_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIdList;
class svtkIntArray;

class SVTKFILTERSPOINTS_EXPORT svtkExtractPointCloudPiece : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static svtkExtractPointCloudPiece* New();
  svtkTypeMacro(svtkExtractPointCloudPiece, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Turn on or off modulo sampling of the points. By default this is on and the
   * points in a given piece will be reordered in an attempt to reduce spatial
   * coherency.
   */
  svtkSetMacro(ModuloOrdering, bool);
  svtkGetMacro(ModuloOrdering, bool);
  svtkBooleanMacro(ModuloOrdering, bool);
  //@}

protected:
  svtkExtractPointCloudPiece();
  ~svtkExtractPointCloudPiece() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  bool ModuloOrdering;

private:
  svtkExtractPointCloudPiece(const svtkExtractPointCloudPiece&) = delete;
  void operator=(const svtkExtractPointCloudPiece&) = delete;
};

#endif
