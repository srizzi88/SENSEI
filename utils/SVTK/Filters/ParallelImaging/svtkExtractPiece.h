/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractPiece
 *
 * svtkExtractPiece returns the appropriate piece of each
 * sub-dataset in the svtkCompositeDataSet.
 * This filter can handle sub-datasets of type svtkImageData, svtkPolyData,
 * svtkRectilinearGrid, svtkStructuredGrid, and svtkUnstructuredGrid; it does
 * not handle sub-grids of type svtkCompositeDataSet.
 */

#ifndef svtkExtractPiece_h
#define svtkExtractPiece_h

#include "svtkCompositeDataSetAlgorithm.h"
#include "svtkFiltersParallelImagingModule.h" // For export macro

class svtkImageData;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkUnstructuredGrid;
class svtkCompositeDataIterator;

class SVTKFILTERSPARALLELIMAGING_EXPORT svtkExtractPiece : public svtkCompositeDataSetAlgorithm
{
public:
  static svtkExtractPiece* New();
  svtkTypeMacro(svtkExtractPiece, svtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkExtractPiece() {}
  ~svtkExtractPiece() override {}

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ExtractImageData(svtkImageData* imageData, svtkCompositeDataSet* output, int piece,
    int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter);
  void ExtractPolyData(svtkPolyData* polyData, svtkCompositeDataSet* output, int piece,
    int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter);
  void ExtractRectilinearGrid(svtkRectilinearGrid* rGrid, svtkCompositeDataSet* output, int piece,
    int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter);
  void ExtractStructuredGrid(svtkStructuredGrid* sGrid, svtkCompositeDataSet* output, int piece,
    int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter);
  void ExtractUnstructuredGrid(svtkUnstructuredGrid* uGrid, svtkCompositeDataSet* output, int piece,
    int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter);

private:
  svtkExtractPiece(const svtkExtractPiece&) = delete;
  void operator=(const svtkExtractPiece&) = delete;
};

#endif
