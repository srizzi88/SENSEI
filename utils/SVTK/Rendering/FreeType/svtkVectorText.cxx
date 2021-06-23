/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorText.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVectorText.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkTransformPolyDataFilter.h"

#include <clocale>

svtkStandardNewMacro(svtkVectorText);

#include "svtkVectorTextData.cxx"

// Construct object with no string set and backing enabled.
svtkVectorText::svtkVectorText()
{
  this->Text = nullptr;

  this->SetNumberOfInputPorts(0);
}

int svtkVectorText::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* newPoints;
  svtkCellArray* newPolys;
  int ptOffset = 0;
  int aPoint, i;
  int pos = 0;
  float xpos = 0;
  float ypos = 0;
  int ptCount, triCount;
  SVTK_VECTOR_TEXT_GLYPH aLetter;
  float width;
  float ftmp[3];

  if (this->Text == nullptr)
  {
    svtkErrorMacro(<< "Text is not set!");
    return 0;
  }

  // Set things up; allocate memory
  newPoints = svtkPoints::New();
  newPolys = svtkCellArray::New();
  ftmp[2] = 0.0;

  // Create Text
  while (this->Text[pos])
  {
    switch (this->Text[pos])
    {
      case 32:
        xpos += 0.4;
        break;

      case 10:
        ypos -= 1.4;
        xpos = 0;
        break;

      default:
        // if we have a valid character
        if ((this->Text[pos] > 32) && (this->Text[pos] < 127))
        {
          // add the result to our output
          aLetter = Letters[static_cast<int>(this->Text[pos]) - 33];
          ptCount = aLetter.ptCount;
          width = aLetter.width;
          for (i = 0; i < ptCount; i++)
          {
            ftmp[0] = aLetter.points[i].x;
            ftmp[1] = aLetter.points[i].y;
            ftmp[0] += xpos;
            ftmp[1] += ypos;
            newPoints->InsertNextPoint(ftmp);
          }
          triCount = aLetter.triCount;
          for (i = 0; i < triCount; i++)
          {
            newPolys->InsertNextCell(3);
            aPoint = aLetter.triangles[i].p1;
            newPolys->InsertCellPoint(aPoint + ptOffset);
            aPoint = aLetter.triangles[i].p2;
            newPolys->InsertCellPoint(aPoint + ptOffset);
            aPoint = aLetter.triangles[i].p3;
            newPolys->InsertCellPoint(aPoint + ptOffset);
          }
          ptOffset += ptCount;
          xpos += width;
        }
        break;
    }
    pos++;
  }

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

void svtkVectorText::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Text: " << (this->Text ? this->Text : "(none)") << "\n";
}

svtkVectorText::~svtkVectorText()
{
  delete[] this->Text;
}
