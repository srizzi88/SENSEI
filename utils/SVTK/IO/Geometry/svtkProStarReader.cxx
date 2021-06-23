/*=========================================================================

  Program: Visualization Toolkit
  Module: svtkProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notice for more information.

=========================================================================*/

#include "svtkProStarReader.h"

#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkUnstructuredGrid.h"
#include <svtksys/SystemTools.hxx>

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkIntArray.h"
#include "svtkPoints.h"

#include <cctype>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkProStarReader);

// Internal Classes/Structures
struct svtkProStarReader::idMapping : public std::map<svtkIdType, svtkIdType>
{
};

//----------------------------------------------------------------------------
svtkProStarReader::svtkProStarReader()
{
  this->FileName = nullptr;
  this->ScaleFactor = 1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkProStarReader::~svtkProStarReader()
{
  delete[] this->FileName;
}

//----------------------------------------------------------------------------
int svtkProStarReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    svtkErrorMacro("FileName has to be specified!");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return 0;
  }

  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (this->FileName)
  {
    idMapping mapPointId; // inverse mapping (STAR-CD pointId -> index)

    if (this->ReadVrtFile(output, mapPointId))
    {
      this->ReadCelFile(output, mapPointId);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkProStarReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
}

//----------------------------------------------------------------------------
int svtkProStarReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    svtkErrorMacro("FileName has to be specified!");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
FILE* svtkProStarReader::OpenFile(const char* ext)
{
  std::string fullName = this->FileName;
  const char* dot = strrchr(this->FileName, '.');

  if (dot != nullptr &&
    (strcmp(dot, ".cel") == 0 || strcmp(dot, ".vrt") == 0 || strcmp(dot, ".inp") == 0))
  {
    fullName.resize(dot - this->FileName);
  }

  fullName += ext;
  FILE* in = svtksys::SystemTools::Fopen(fullName.c_str(), "r");
  if (in == nullptr)
  {
    svtkErrorMacro(<< "Error opening file: " << fullName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
  }

  return in;
}

//
// read in the points from the .vrt file
/*---------------------------------------------------------------------------*\
Line 1:
PROSTAR_VERTEX [newline]

Line 2:
<version> 0 0 0 0 0 0 0 [newline]

Body:
<vertexId> <x> <y> <z> [newline]

\*---------------------------------------------------------------------------*/
bool svtkProStarReader::ReadVrtFile(svtkUnstructuredGrid* output, idMapping& mapPointId)
{
  mapPointId.clear();
  FILE* in = this->OpenFile(".vrt");
  if (in == nullptr)
  {
    return false;
  }

  const int MAX_LINE = 1024;
  char rawLine[MAX_LINE];

  int lineLabel, errorCount = 0;
  if (fgets(rawLine, MAX_LINE, in) != nullptr && strncmp(rawLine, "PROSTAR_VERTEX", 14) == 0 &&
    fgets(rawLine, MAX_LINE, in) != nullptr && sscanf(rawLine, "%d", &lineLabel) == 1 &&
    lineLabel >= 4000)
  {
    svtkDebugMacro(<< "Got PROSTAR_VERTEX header");
  }
  else
  {
    svtkErrorMacro(<< "Error reading header for PROSTAR_VERTEX file");
    ++errorCount;
  }

  svtkPoints* points = svtkPoints::New();
  // don't know the number of points a priori -- just pick some number
  points->Allocate(10000, 20000);

  int lineNr = 2;
  float xyz[3];
  svtkIdType nodeCount = 0;

  while (!errorCount && fgets(rawLine, MAX_LINE, in) != nullptr)
  {
    ++lineNr;
    if (sscanf(rawLine, "%d %f %f %f", &lineLabel, xyz, xyz + 1, xyz + 2) == 4)
    {
      xyz[0] *= this->ScaleFactor;
      xyz[1] *= this->ScaleFactor;
      xyz[2] *= this->ScaleFactor;

      points->InsertNextPoint(xyz);
      svtkIdType nodeId = lineLabel;
      mapPointId.insert(std::make_pair(nodeId, nodeCount));
      ++nodeCount;
    }
    else
    {
      svtkErrorMacro(<< "Error reading point at line " << lineNr);
      ++errorCount;
    }
  }

  points->Squeeze();
  output->SetPoints(points);
  points->Delete();

  svtkDebugMacro(<< "Read points: " << lineNr << " errors: " << errorCount);

  fclose(in);
  return errorCount == 0;
}

//
// read in the cells from the .cel file
/*---------------------------------------------------------------------------*\
Line 1:
PROSTAR_CELL [newline]

Line 2:
<version> 0 0 0 0 0 0 0 [newline]

Body:
<cellId> <shapeId> <nLabels> <cellTableId> <typeId> [newline]
<cellId> <int1> .. <int8>
<cellId> <int9> .. <int16>

with shapeId:
* 1 = point
* 2 = line
* 3 = shell
* 11 = hexa
* 12 = prism
* 13 = tetra
* 14 = pyramid
* 255 = polyhedron

with typeId
* 1 = fluid
* 2 = solid
* 3 = baffle
* 4 = shell
* 5 = line
* 6 = point

For primitive cell shapes, the number of vertices will never exceed 8 (hexa)
and corresponds to <nLabels>.
For polyhedral, <nLabels> includes an index table comprising beg/end pairs
for each cell face.

\*---------------------------------------------------------------------------*/
bool svtkProStarReader::ReadCelFile(svtkUnstructuredGrid* output, const idMapping& mapPointId)
{
  FILE* in = this->OpenFile(".cel");
  if (in == nullptr)
  {
    return false;
  }

  const int MAX_LINE = 1024;
  char rawLine[MAX_LINE];

  int lineLabel, errorCount = 0;
  if (fgets(rawLine, MAX_LINE, in) != nullptr && strncmp(rawLine, "PROSTAR_CELL", 12) == 0 &&
    fgets(rawLine, MAX_LINE, in) != nullptr && sscanf(rawLine, "%d", &lineLabel) == 1 &&
    lineLabel >= 4000)
  {
    svtkDebugMacro(<< "Got PROSTAR_CELL header");
  }
  else
  {
    svtkErrorMacro(<< "Error reading header for PROSTAR_CELL file");
    ++errorCount;
  }

  // don't know the number of cells a priori -- just pick some number
  output->Allocate(10000, 20000);

  // add a cellTableId array
  svtkIntArray* cellTableId = svtkIntArray::New();
  cellTableId->Allocate(10000, 20000);
  cellTableId->SetName("cellTableId");

  int shapeId, nLabels, tableId, typeId;
  std::vector<svtkIdType> starLabels;
  starLabels.reserve(256);

  // face-stream for a polyhedral cell
  // [numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...]
  std::vector<svtkIdType> faceStream;
  faceStream.reserve(256);

  // use string buffer for easier parsing
  std::istringstream strbuf;

  int lineNr = 2;
  while (!errorCount && fgets(rawLine, MAX_LINE, in) != nullptr)
  {
    ++lineNr;
    if (sscanf(rawLine, "%d %d %d %d %d", &lineLabel, &shapeId, &nLabels, &tableId, &typeId) == 5)
    {
      starLabels.clear();
      starLabels.reserve(nLabels);

      // read indices - max 8 per line
      for (int index = 0; !errorCount && index < nLabels; ++index)
      {
        int vrtId;
        if ((index % 8) == 0)
        {
          if (fgets(rawLine, MAX_LINE, in) != nullptr)
          {
            ++lineNr;
            strbuf.clear();
            strbuf.str(rawLine);
            strbuf >> lineLabel;
          }
          else
          {
            svtkErrorMacro(<< "Error reading PROSTAR_CELL file at line " << lineNr);
            ++errorCount;
          }
        }

        strbuf >> vrtId;
        starLabels.push_back(vrtId);
      }

      // special treatment for polyhedra
      if (shapeId == starcdPoly)
      {
        const svtkIdType nFaces = starLabels[0] - 1;

        // build face-stream
        // [numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...]
        // point Ids are global
        faceStream.clear();
        faceStream.reserve(nLabels);

        for (svtkIdType faceI = 0; faceI < nFaces; ++faceI)
        {
          // traverse beg/end indices
          const int beg = starLabels[faceI];
          const int end = starLabels[faceI + 1];

          // number of points for this face
          faceStream.push_back(end - beg);

          for (int idxI = beg; idxI < end; ++idxI)
          {
            // map orig vertex id -> point label
            faceStream.push_back(mapPointId.find(starLabels[idxI])->second);
          }
        }

        output->InsertNextCell(SVTK_POLYHEDRON, nFaces, &(faceStream[0]));
        cellTableId->InsertNextValue(tableId);
      }
      else
      {
        // map orig vertex id -> point label
        for (int i = 0; i < nLabels; ++i)
        {
          starLabels[i] = mapPointId.find(starLabels[i])->second;
        }

        switch (shapeId)
        {
          // 0-D
          case starcdPoint:
            output->InsertNextCell(SVTK_VERTEX, 1, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          // 1-D
          case starcdLine:
            output->InsertNextCell(SVTK_LINE, 2, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          // 2-D
          case starcdShell:
            switch (nLabels)
            {
              case 3:
                output->InsertNextCell(SVTK_TRIANGLE, 3, &(starLabels[0]));
                break;
              case 4:
                output->InsertNextCell(SVTK_QUAD, 4, &(starLabels[0]));
                break;
              default:
                output->InsertNextCell(SVTK_POLYGON, nLabels, &(starLabels[0]));
                break;
            }
            cellTableId->InsertNextValue(tableId);
            break;

          // 3-D
          case starcdHex:
            output->InsertNextCell(SVTK_HEXAHEDRON, 8, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          case starcdPrism:
            // the SVTK definition has outwards normals for the triangles!!
            std::swap(starLabels[1], starLabels[2]);
            std::swap(starLabels[4], starLabels[5]);
            output->InsertNextCell(SVTK_WEDGE, 6, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          case starcdTet:
            output->InsertNextCell(SVTK_TETRA, 4, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          case starcdPyr:
            output->InsertNextCell(SVTK_PYRAMID, 5, &(starLabels[0]));
            cellTableId->InsertNextValue(tableId);
            break;

          default:
            break;
        }
      }
    }
    else
    {
      svtkErrorMacro(<< "Error reading cell at line " << lineNr);
      ++errorCount;
    }
  }

  output->Squeeze();
  cellTableId->Squeeze();

  // now add the cellTableId array
  output->GetCellData()->AddArray(cellTableId);
  if (!output->GetCellData()->GetScalars())
  {
    output->GetCellData()->SetScalars(cellTableId);
  }
  cellTableId->Delete();

  svtkDebugMacro(<< "Read cells: " << lineNr << " errors: " << errorCount);

  fclose(in);
  return errorCount == 0;
}
