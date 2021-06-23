/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIVWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkIVWriter.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkIVWriter);

//----------------------------------------------------------------------------
void svtkIVWriter::WriteData()
{
  FILE* fp;

  // make sure the user specified a FileName
  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // try opening the files
  fp = svtksys::SystemTools::Fopen(this->FileName, "w");
  if (!fp)
  {
    svtkErrorMacro(<< "unable to open OpenInventor file: " << this->FileName);
    return;
  }

  //
  //  Write header
  //
  svtkDebugMacro("Writing OpenInventor file");
  fprintf(fp, "#Inventor V2.0 ascii\n");
  fprintf(fp, "# OpenInventor file written by the visualization toolkit\n\n");
  this->WritePolyData(this->GetInput(), fp);
  if (fclose(fp))
  {
    svtkErrorMacro(<< this->FileName << " did not close successfully. Check disk space.");
  }
}

//----------------------------------------------------------------------------
void svtkIVWriter::WritePolyData(svtkPolyData* pd, FILE* fp)
{
  svtkPoints* points;
  svtkIdType i;
  svtkCellArray* cells;
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;
  svtkUnsignedCharArray* colors = nullptr;

  points = pd->GetPoints();

  // create colors for vertices
  svtkDataArray* scalars = pd->GetPointData()->GetScalars();

  if (scalars)
  {
    svtkLookupTable* lut;
    if ((lut = scalars->GetLookupTable()) == nullptr)
    {
      lut = svtkLookupTable::New();
      lut->Build();
    }
    colors = lut->MapScalars(scalars, SVTK_COLOR_MODE_DEFAULT, 0);
    if (!scalars->GetLookupTable())
    {
      lut->Delete();
    }
  }

  fprintf(fp, "Separator {\n");

  // Point data (coordinates)
  fprintf(fp, "\tCoordinate3 {\n");
  fprintf(fp, "\t\tpoint [\n");
  fprintf(fp, "\t\t\t");
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double xyz[3];
    points->GetPoint(i, xyz);
    fprintf(fp, "%g %g %g, ", xyz[0], xyz[1], xyz[2]);
    if (!((i + 1) % 2))
    {
      fprintf(fp, "\n\t\t\t");
    }
  }
  fprintf(fp, "\n\t\t]");
  fprintf(fp, "\t}\n");

  // Per vertex coloring
  fprintf(fp, "\tMaterialBinding {\n");
  fprintf(fp, "\t\tvalue PER_VERTEX_INDEXED\n");
  fprintf(fp, "\t}\n");

  // Colors, if any
  if (colors)
  {
    fprintf(fp, "\tMaterial {\n");
    fprintf(fp, "\t\tdiffuseColor [\n");
    fprintf(fp, "\t\t\t");
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      unsigned char* rgba;
      rgba = colors->GetPointer(4 * i);
      fprintf(fp, "%g %g %g, ", rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f);
      if (!((i + 1) % 2))
      {
        fprintf(fp, "\n\t\t\t");
      }
    }
    fprintf(fp, "\n\t\t]\n");
    fprintf(fp, "\t}\n");
    colors->Delete();
  }

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    fprintf(fp, "\tIndexedFaceSet {\n");
    fprintf(fp, "\t\tcoordIndex [\n");
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", (int)indx[i]);
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "\t\t]\n");
    fprintf(fp, "\t}\n");
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    fprintf(fp, "\tIndexedLineSet {\n");
    fprintf(fp, "\t\tcoordIndex  [\n");

    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", (int)indx[i]);
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "\t\t]\n");
    fprintf(fp, "\t}\n");
  }

  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
  {
    fprintf(fp, "\tIndexdedPointSet {\n");
    fprintf(fp, "\t\tcoordIndex [");
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", (int)indx[i]);
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "\t\t]\n");
    fprintf(fp, "\t}\n");
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {

    fprintf(fp, "\tIndexedTriangleStripSet {\n");
    fprintf(fp, "\t\tcoordIndex [\n");
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", (int)indx[i]);
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "\t\t]\n");
    fprintf(fp, "\t}\n");
  }

  fprintf(fp, "}\n"); // close the Shape
}

//----------------------------------------------------------------------------
void svtkIVWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkIVWriter::GetInput()
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkPolyData* svtkIVWriter::GetInput(int port)
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int svtkIVWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
