/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractPiece.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkExtentTranslator.h"
#include "svtkExtractGrid.h"
#include "svtkExtractPolyDataPiece.h"
#include "svtkExtractRectilinearGrid.h"
#include "svtkExtractUnstructuredGridPiece.h"
#include "svtkImageClip.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractPiece);

//=============================================================================
int svtkExtractPiece::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//=============================================================================
int svtkExtractPiece::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    if (!output || !output->IsA(input->GetClassName()))
    {
      svtkDataObject* outData = input->NewInstance();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outData);
      outData->Delete();
    }
    return 1;
  }
  return 0;
}

//=============================================================================
int svtkExtractPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkCompositeDataSet* input =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }
  svtkCompositeDataSet* output =
    svtkCompositeDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  // Copy structure and meta-data.
  output->CopyStructure(input);

  int updateNumPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatePiece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateGhostLevel =
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  svtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* tmpDS = iter->GetCurrentDataObject();
    switch (tmpDS->GetDataObjectType())
    {
      case SVTK_IMAGE_DATA:
        this->ExtractImageData(
          (svtkImageData*)(tmpDS), output, updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case SVTK_POLY_DATA:
        this->ExtractPolyData(
          (svtkPolyData*)(tmpDS), output, updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case SVTK_RECTILINEAR_GRID:
        this->ExtractRectilinearGrid((svtkRectilinearGrid*)(tmpDS), output, updatePiece,
          updateNumPieces, updateGhostLevel, iter);
        break;
      case SVTK_STRUCTURED_GRID:
        this->ExtractStructuredGrid((svtkStructuredGrid*)(tmpDS), output, updatePiece,
          updateNumPieces, updateGhostLevel, iter);
        break;
      case SVTK_UNSTRUCTURED_GRID:
        this->ExtractUnstructuredGrid((svtkUnstructuredGrid*)(tmpDS), output, updatePiece,
          updateNumPieces, updateGhostLevel, iter);
        break;
      default:
        svtkErrorMacro("Cannot extract data of type " << tmpDS->GetClassName());
        break;
    }
  }
  iter->Delete();

  return 1;
}

//=============================================================================
void svtkExtractPiece::ExtractImageData(svtkImageData* imageData, svtkCompositeDataSet* output,
  int piece, int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter)
{
  int ext[6];

  svtkImageClip* extractID = svtkImageClip::New();
  extractID->ClipDataOn();
  imageData->GetExtent(ext);

  svtkExtentTranslator* translate = svtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractID->SetInputData(imageData);
  extractID->SetOutputWholeExtent(ext);
  extractID->UpdateExtent(ext);
  svtkImageData* extractOutput = svtkImageData::New();
  extractOutput->ShallowCopy(extractID->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractID->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void svtkExtractPiece::ExtractPolyData(svtkPolyData* polyData, svtkCompositeDataSet* output, int piece,
  int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter)
{
  svtkExtractPolyDataPiece* extractPD = svtkExtractPolyDataPiece::New();
  extractPD->SetInputData(polyData);
  extractPD->UpdatePiece(piece, numberOfPieces, ghostLevel);
  svtkPolyData* extractOutput = svtkPolyData::New();
  extractOutput->ShallowCopy(extractPD->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractPD->Delete();
  extractOutput->Delete();
}

void svtkExtractPiece::ExtractRectilinearGrid(svtkRectilinearGrid* rGrid, svtkCompositeDataSet* output,
  int piece, int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter)
{
  int ext[6];

  svtkExtractRectilinearGrid* extractRG = svtkExtractRectilinearGrid::New();
  rGrid->GetExtent(ext);

  svtkExtentTranslator* translate = svtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractRG->SetInputData(rGrid);
  extractRG->UpdateExtent(ext);
  svtkRectilinearGrid* extractOutput = svtkRectilinearGrid::New();
  extractOutput->ShallowCopy(extractRG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractRG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void svtkExtractPiece::ExtractStructuredGrid(svtkStructuredGrid* sGrid, svtkCompositeDataSet* output,
  int piece, int numberOfPieces, int ghostLevel, svtkCompositeDataIterator* iter)
{
  svtkInformation* extractInfo;
  int ext[6];

  svtkExtractGrid* extractSG = svtkExtractGrid::New();
  sGrid->GetExtent(ext);

  svtkExtentTranslator* translate = svtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractSG->SetInputData(sGrid);
  extractInfo = extractSG->GetOutputInformation(0);
  extractSG->UpdateInformation();
  extractInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  extractSG->UpdateExtent(ext);
  svtkStructuredGrid* extractOutput = svtkStructuredGrid::New();
  extractOutput->ShallowCopy(extractSG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractSG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void svtkExtractPiece::ExtractUnstructuredGrid(svtkUnstructuredGrid* uGrid,
  svtkCompositeDataSet* output, int piece, int numberOfPieces, int ghostLevel,
  svtkCompositeDataIterator* iter)
{
  svtkExtractUnstructuredGridPiece* extractUG = svtkExtractUnstructuredGridPiece::New();
  extractUG->SetInputData(uGrid);
  extractUG->UpdatePiece(piece, numberOfPieces, ghostLevel);
  svtkUnstructuredGrid* extractOutput = svtkUnstructuredGrid::New();
  extractOutput->ShallowCopy(extractUG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractUG->Delete();
  extractOutput->Delete();
}

//=============================================================================
void svtkExtractPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
