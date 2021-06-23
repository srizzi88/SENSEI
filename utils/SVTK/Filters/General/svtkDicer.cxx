/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDicer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDicer.h"

#include "svtkDataSet.h"
#include "svtkMath.h"

// Instantiate object.
svtkDicer::svtkDicer()
{
  this->NumberOfPointsPerPiece = 5000;
  this->NumberOfPieces = 10;
  this->MemoryLimit = 50 * 1024; // 50 mebibytes
  this->NumberOfActualPieces = 0;
  this->FieldData = 0;
  this->DiceMode = SVTK_DICE_MODE_NUMBER_OF_POINTS;
}

// This method unifies the measures used to define piece size. Call this
// in the subclass Execute() method.
void svtkDicer::UpdatePieceMeasures(svtkDataSet* input)
{
  int numPts = input->GetNumberOfPoints();
  unsigned long memSize = input->GetActualMemorySize();

  if (this->DiceMode == SVTK_DICE_MODE_NUMBER_OF_POINTS)
  {
    this->NumberOfPieces =
      static_cast<int>(ceil(static_cast<double>(numPts) / this->NumberOfPointsPerPiece));
    this->MemoryLimit =
      static_cast<unsigned long>(ceil(static_cast<double>(memSize) / this->NumberOfPieces));
  }

  else if (this->DiceMode == SVTK_DICE_MODE_SPECIFIED_NUMBER)
  {
    this->NumberOfPointsPerPiece =
      static_cast<int>(ceil(static_cast<double>(numPts) / this->NumberOfPieces));
    this->MemoryLimit =
      static_cast<unsigned long>(ceil(static_cast<double>(memSize) / this->NumberOfPieces));
  }

  else // this->DiceMode == SVTK_DICE_MODE_MEMORY_LIMIT
  {
    this->NumberOfPieces = static_cast<int>(ceil(static_cast<double>(memSize) / this->MemoryLimit));
    this->NumberOfPointsPerPiece =
      static_cast<int>(ceil(static_cast<double>(numPts) / this->NumberOfPieces));
  }
}

void svtkDicer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Points per Piece: " << this->NumberOfPointsPerPiece << "\n";

  os << indent << "Number of Pieces: " << this->NumberOfPieces << "\n";

  os << indent << "Memory Limit (in kibibytes): " << this->MemoryLimit << "\n";

  os << indent << "Number of Actual Pieces: " << this->NumberOfActualPieces << "\n";

  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");

  if (this->DiceMode == SVTK_DICE_MODE_NUMBER_OF_POINTS)
  {
    os << indent << "Dice Mode: Number Of Points\n";
  }
  else if (this->DiceMode == SVTK_DICE_MODE_SPECIFIED_NUMBER)
  {
    os << indent << "Dice Mode: Specified Number\n";
  }
  else
  {
    os << indent << "Dice Mode: Memory Limit\n";
  }
}
