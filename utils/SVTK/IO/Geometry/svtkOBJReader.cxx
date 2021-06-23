/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOBJReader.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <svtksys/SystemTools.hxx>

#include "svtkCellData.h"
#include "svtkStringArray.h"

svtkStandardNewMacro(svtkOBJReader);

//----------------------------------------------------------------------------
svtkOBJReader::svtkOBJReader()
{
  this->Comment = nullptr;
}

//----------------------------------------------------------------------------
svtkOBJReader::~svtkOBJReader()
{
  this->SetComment(nullptr);
}

/*---------------------------------------------------------------------------*\

This is only partial support for the OBJ format, which is quite complicated.
To find a full specification, search the net for "OBJ format", eg.:

    https://en.wikipedia.org/wiki/Wavefront_.obj_file
    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm
    http://paulbourke.net/dataformats/obj/

We support the following types:

g <groupName>  [... <groupNameN]

    group name, primarily for faces

v <x> <y> <z>

    vertex

vn <x> <y> <z>

    vertex normal

vt <x> <y>

    texture coordinate
    note: vt are globally indexed, see "Referencing vertex data" section
    of Paul Bourke format description.

f <v_a> <v_b> <v_c> ...

    polygonal face linking vertices v_a, v_b, v_c, etc. which
    are 1-based indices into the vertex list

f <v_a>/<t_a> <v_b>/<t_b> ...

    polygonal face as above, but with texture coordinates for
    each vertex. t_a etc. are 1-based indices into the texture
    coordinates list (from the vt lines)

f <v_a>/<t_a>/<n_a> <v_b>/<t_b>/<n_b> ...

    polygonal face as above, with a normal at each vertex, as a
    1-based index into the normals list (from the vn lines)

f <v_a>//<n_a> <v_b>//<n_b> ...

    polygonal face as above but without texture coordinates.

    Per-face tcoords and normals are supported by duplicating
    the vertices on each face as necessary.

l <v_a> <v_b> ...

    lines linking vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

p <v_a> <v_b> ...

    points located at the vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

\*---------------------------------------------------------------------------*/

int svtkOBJReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    svtkErrorMacro(<< "A FileName must be specified.");
    return 0;
  }

  FILE* in = svtksys::SystemTools::Fopen(this->FileName, "r");

  if (in == nullptr)
  {
    svtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  svtkDebugMacro(<< "Reading file");

  // initialize some structures to store the file contents in
  svtkPoints* points = svtkPoints::New();
  std::unordered_map<std::string, svtkFloatArray*> tcoords_map;
  std::vector<std::pair<float, float> > verticesTextureList;
  svtkFloatArray* normals = svtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetName("Normals");
  svtkCellArray* polys = svtkCellArray::New();
  svtkCellArray* tcoord_polys = svtkCellArray::New();
  svtkCellArray* pointElems = svtkCellArray::New();
  svtkCellArray* lineElems = svtkCellArray::New();
  svtkCellArray* normal_polys = svtkCellArray::New();

  // Face scalars (corresponding to groups)
  svtkFloatArray* faceScalars = svtkFloatArray::New();
  faceScalars->SetNumberOfComponents(1);
  faceScalars->SetName("GroupIds");

  // Handling of "g" grouping
  int groupId = -1;

  bool hasTCoords = false;
  bool hasNormals = false;
  bool tcoords_same_as_verts = true;
  bool normals_same_as_verts = true;

  svtkSmartPointer<svtkIntArray> matIds = svtkSmartPointer<svtkIntArray>::New();
  matIds->SetNumberOfComponents(1);
  matIds->SetName("MaterialIds");
  svtkSmartPointer<svtkStringArray> matNames = svtkSmartPointer<svtkStringArray>::New();
  matNames->SetName("MaterialNames");
  matNames->SetNumberOfComponents(1);
  std::unordered_map<std::string, int> matNameToId;
  std::unordered_map<svtkIdType, std::string> startCellToMatName;
  int matcnt = 0;
  int matid = 0;

  bool everything_ok = true; // (use of this flag avoids early return and associated memory leak)

  // -- work through the file line by line, assigning into the above 7 structures as appropriate --

  { // (make a local scope section to emphasise that the variables below are only used here)

    const int MAX_LINE = 1024 * 256;
    char rawLine[MAX_LINE];
    char tcoordsName[100];
    float xyz[3];
    int numPoints = 0;
    int numTCoords = 0;
    int numNormals = 0;

    // First loop to initialize the data arrays for the different set of texture coordinates
    bool readingFirstComment = true;
    std::string firstComment;
    int lineNr = 0;
    while (everything_ok && fgets(rawLine, MAX_LINE, in) != nullptr)
    {
      ++lineNr;
      char* pLine = rawLine;
      char* pEnd = rawLine + strlen(rawLine);

      if (*(pEnd - 1) != '\n' && !feof(in))
      {
        svtkErrorMacro(<< "Line longer than " << MAX_LINE << ": " << pLine);
        everything_ok = false;
      }

      // find the first non-whitespace character
      while (isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // this first non-whitespace is the command
      const char* cmd = pLine;

      if (readingFirstComment)
      {
        if (cmd[0] == '#')
        {
          cmd++; // skip #
          while (isspace(*cmd) && cmd < pEnd)
          {
            cmd++;
          } // skip whitespace at comment start
          firstComment += cmd;
        }
        else
        {
          // This is not a comment line, real file content is started.
          // There may be more comments in the file but we ignore those.
          readingFirstComment = false;
        }
      }

      // skip over non-whitespace
      while (!isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // terminate command
      if (pLine < pEnd)
      {
        *pLine = '\0';
        pLine++;
      }

      // if line starts by "usemtl", we're listing a new set of texture coordinates
      if (strcmp(cmd, "usemtl") == 0)
      {
        // Read name of texture coordinate
        if (sscanf(pLine, "%s", tcoordsName) == 1)
        {
          if (tcoords_map.find(tcoordsName) == tcoords_map.end())
          {
            svtkFloatArray* tcoords = svtkFloatArray::New();
            tcoords->SetNumberOfComponents(2);
            tcoords->SetName(tcoordsName);
            tcoords_map.emplace(tcoordsName, tcoords);
          }
        }
        else
        {
          svtkErrorMacro(<< "Error reading 'usemtl' at line " << lineNr);
          everything_ok = false;
        }
      }
      else if (strcmp(cmd, "vt") == 0)
      {
        // this is a tcoord, expect two floats, separated by whitespace:
        std::stringstream dataStream;
        dataStream.imbue(std::locale::classic());
        dataStream << pLine;

        try
        {
          dataStream >> xyz[0] >> xyz[1];
          verticesTextureList.emplace_back(xyz[0], xyz[1]);
        }
        catch (const std::exception&)
        {
          svtkErrorMacro(<< "Error reading 'vt' at line " << lineNr);
        }
      }
    } // (end of first while loop)

    // Comment lines include newline characters.
    // Keep newlines between lines of multi-line comment, but
    // remove the last newline to have a clean string when comment is single-line.
    while (!firstComment.empty() && (firstComment.back() == '\r' || firstComment.back() == '\n'))
    {
      firstComment.pop_back();
    }
    this->SetComment(firstComment.c_str());

    // If no material texture coordinates are found, add default TCoords
    if (tcoords_map.empty())
    {
      svtkFloatArray* tcoords = svtkFloatArray::New();
      tcoords->SetNumberOfComponents(2);
      strcpy(tcoordsName, "TCoords");
      tcoords->SetName(tcoordsName);
      tcoords_map.emplace(tcoordsName, tcoords);
    }

    // Initialize every texture array with (-1, -1)
    {
      const svtkIdType nTuples = static_cast<svtkIdType>(verticesTextureList.size());

      for (const auto& iter : tcoords_map)
      {
        svtkFloatArray* tcoords = iter.second;
        tcoords->SetNumberOfTuples(nTuples);

        for (svtkIdType i = 0; i < nTuples; ++i)
        {
          tcoords->SetTuple2(i, -1.0, -1.0);
        }
      }
    }

    // Second loop to parse points, faces, texture coordinates, normals...
    lineNr = 0;
    fseek(in, 0, SEEK_SET);
    while (everything_ok && fgets(rawLine, MAX_LINE, in) != nullptr)
    {
      ++lineNr;
      char* pLine = rawLine;
      char* pEnd = rawLine + strlen(rawLine);

      // find the first non-whitespace character
      while (isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // this first non-whitespace is the command
      const char* cmd = pLine;

      // skip over non-whitespace
      while (!isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // terminate command
      if (pLine < pEnd)
      {
        *pLine = '\0';
        pLine++;
      }

      if (strcmp(cmd, "g") == 0)
      {
        // group definition, expect 0 or more words separated by whitespace.
        // But here we simply note its existence, without a name
        ++groupId;
      }
      else if (strcmp(cmd, "v") == 0)
      {
        // vertex definition, expect three floats, separated by whitespace:
        std::stringstream dataStream;
        dataStream.imbue(std::locale::classic());
        dataStream << pLine;

        try
        {
          dataStream >> xyz[0] >> xyz[1] >> xyz[2];
          points->InsertNextPoint(xyz);
          numPoints++;
        }
        catch (const std::exception&)
        {
          svtkErrorMacro(<< "Error reading 'v' at line " << lineNr);
          everything_ok = false;
        }
      }
      else if (strcmp(cmd, "usemtl") == 0)
      {
        // material name (for texture coordinates), expect one string:
        if (sscanf(pLine, "%s", tcoordsName) != 1)
        {
          svtkErrorMacro(<< "Error reading 'usemtl' at line " << lineNr);
          everything_ok = false;
        }
        if (matNameToId.find(tcoordsName) == matNameToId.end())
        {
          // haven't seen this material yet, keep a record of it
          matNameToId.emplace(tcoordsName, matcnt);
          matNames->InsertNextValue(tcoordsName);
          matcnt++;
        }
        // remember that starting with current cell, we should draw with it
        startCellToMatName[polys->GetNumberOfCells()] = tcoordsName;
      }
      else if (strcmp(cmd, "vt") == 0)
      {
        numTCoords++;
      }
      else if (strcmp(cmd, "vn") == 0)
      {
        // vertex normal, expect three floats, separated by whitespace:
        std::stringstream dataStream;
        dataStream.imbue(std::locale::classic());
        dataStream << pLine;

        try
        {
          dataStream >> xyz[0] >> xyz[1] >> xyz[2];
          normals->InsertNextTuple(xyz);
          hasNormals = true;
          numNormals++;
        }
        catch (const std::exception&)
        {
          svtkErrorMacro(<< "Error reading 'vn' at line " << lineNr);
          everything_ok = false;
        }
      }
      else if (strcmp(cmd, "p") == 0)
      {
        // point definition, consisting of 1-based indices separated by whitespace and /
        pointElems->InsertNextCell(0); // we don't yet know how many points are to come

        int nVerts = 0; // keep a count of how many there are

        while (everything_ok && pLine < pEnd)
        {
          // find next non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert;
            if (sscanf(pLine, "%d", &iVert) == 1)
            {
              if (iVert < 0)
              {
                pointElems->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                pointElems->InsertCellPoint(iVert - 1);
              }
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'p' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just sscanf'd
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        if (nVerts < 1)
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'p' command");
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        pointElems->UpdateCellCount(nVerts);
      }
      else if (strcmp(cmd, "l") == 0)
      {
        // line definition, consisting of 1-based indices separated by whitespace and /
        lineElems->InsertNextCell(0); // we don't yet know how many points are to come

        int nVerts = 0; // keep a count of how many there are

        while (everything_ok && pLine < pEnd)
        {
          // find next non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert, dummyInt;
            if (sscanf(pLine, "%d/%d", &iVert, &dummyInt) == 2)
            {
              // we simply ignore texture information
              if (iVert < 0)
              {
                lineElems->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                lineElems->InsertCellPoint(iVert - 1);
              }
              nVerts++;
            }
            else if (sscanf(pLine, "%d", &iVert) == 1)
            {
              if (iVert < 0)
              {
                lineElems->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                lineElems->InsertCellPoint(iVert - 1);
              }
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'l' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just sscanf'd
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        if (nVerts < 2)
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'l' command");
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        lineElems->UpdateCellCount(nVerts);
      }
      else if (strcmp(cmd, "f") == 0)
      {
        // face definition, consisting of 1-based indices separated by whitespace and /

        polys->InsertNextCell(0); // we don't yet know how many points are to come
        tcoord_polys->InsertNextCell(0);
        normal_polys->InsertNextCell(0);

        int nVerts = 0, nTCoords = 0, nNormals = 0; // keep a count of how many of each there are

        while (everything_ok && pLine < pEnd)
        {
          // find the first non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert, iTCoord, iNormal;
            if (sscanf(pLine, "%d/%d/%d", &iVert, &iTCoord, &iNormal) == 3)
            {
              if (iVert < 0)
              {
                polys->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                polys->InsertCellPoint(iVert - 1);
              }
              nVerts++;

              // Current index is relative to last texture index
              int iTCoordAbs = (iTCoord < 0) ? numTCoords + iTCoord : iTCoord - 1;
              tcoord_polys->InsertCellPoint(iTCoordAbs);

              // Set the current texture array with the value corresponding to the
              // iTcoords read
              const auto& currentTCoord = verticesTextureList[iTCoordAbs];
              auto iter = tcoords_map.find(tcoordsName);
              svtkFloatArray* tcArray = iter->second;
              tcArray->SetTuple2(iTCoordAbs, currentTCoord.first, currentTCoord.second);

              nTCoords++;

              // Current index is relative to last normal index
              if (iNormal < 0)
              {
                normal_polys->InsertCellPoint(numNormals + iNormal);
              }
              else
              {
                normal_polys->InsertCellPoint(iNormal - 1);
              }
              nNormals++;
              if (iTCoord != iVert)
              {
                tcoords_same_as_verts = false;
              }
              if (iNormal != iVert)
              {
                normals_same_as_verts = false;
              }
            }
            else if (sscanf(pLine, "%d//%d", &iVert, &iNormal) == 2)
            {
              if (iVert < 0)
              {
                polys->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                polys->InsertCellPoint(iVert - 1);
              }
              nVerts++;

              // Current index is relative to last normal index
              if (iNormal < 0)
              {
                normal_polys->InsertCellPoint(numNormals + iNormal);
              }
              else
              {
                normal_polys->InsertCellPoint(iNormal - 1);
              }
              nNormals++;
              if (iNormal != iVert)
                normals_same_as_verts = false;
            }
            else if (sscanf(pLine, "%d/%d", &iVert, &iTCoord) == 2)
            {
              if (iVert < 0)
              {
                polys->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                polys->InsertCellPoint(iVert - 1);
              }
              nVerts++;

              // Current index is relative to last texture index
              int iTCoordAbs = (iTCoord < 0) ? numTCoords + iTCoord : iTCoord - 1;
              tcoord_polys->InsertCellPoint(iTCoordAbs);

              // Set the current texture array with the value corresponding to the
              // iTcoords read
              const auto& currentTCoord = verticesTextureList[iTCoordAbs];
              tcoords_map[tcoordsName]->SetTuple2(
                iTCoordAbs, currentTCoord.first, currentTCoord.second);

              nTCoords++;
              if (iTCoord != iVert)
              {
                tcoords_same_as_verts = false;
              }
            }
            else if (sscanf(pLine, "%d", &iVert) == 1)
            {
              if (iVert < 0)
              {
                polys->InsertCellPoint(numPoints + iVert);
              }
              else
              {
                polys->InsertCellPoint(iVert - 1);
              }
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'f' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just read
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        // count of tcoords and normals must be equal to number of vertices or zero
        if (nVerts < 3 || (nTCoords > 0 && nTCoords != nVerts) ||
          (nNormals > 0 && nNormals != nVerts))
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'f' command");
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        polys->UpdateCellCount(nVerts);
        tcoord_polys->UpdateCellCount(nTCoords);
        normal_polys->UpdateCellCount(nNormals);

        // also make a note of whether any cells have tcoords, and whether any have normals
        if (nTCoords > 0)
        {
          hasTCoords = true;
        }
        if (nNormals > 0)
        {
          hasNormals = true;
        }

        if (faceScalars && nVerts)
        {
          if (groupId < 0)
          {
            groupId = 0;
          }
          faceScalars->InsertNextValue(groupId);
        }
      }
      else
      {
        // svtkDebugMacro(<<"Ignoring line: "<<rawLine);
      }

    } // (end of while loop)

  } // (end of local scope section)

  // we have finished with the file
  fclose(in);

  const bool hasGroups = (groupId >= 0);
  const bool hasMaterials = (matcnt > 0);

  if (everything_ok) // (otherwise just release allocated memory and return)
  {
    // -- now turn this lot into a useable svtkPolyData --

    // if there are no tcoords or normals or they match exactly
    // then we can just copy the data into the output (easy!)
    if ((!hasTCoords || tcoords_same_as_verts) && (!hasNormals || normals_same_as_verts))
    {
      svtkDebugMacro(<< "Copying file data into the output directly");

      output->SetPoints(points);
      if (pointElems->GetNumberOfCells())
      {
        output->SetVerts(pointElems);
      }
      if (lineElems->GetNumberOfCells())
      {
        output->SetLines(lineElems);
      }
      if (polys->GetNumberOfCells())
      {
        output->SetPolys(polys);
      }

      // if there is an exact correspondence between tcoords and vertices then can simply
      // assign the tcoords points as point data
      if (hasTCoords && tcoords_same_as_verts)
      {
        bool setTcoords = true;
        for (const auto& iter : tcoords_map)
        {
          svtkFloatArray* tcoords = iter.second;
          output->GetPointData()->AddArray(tcoords);
          if (setTcoords)
          {
            setTcoords = false;
            output->GetPointData()->SetActiveTCoords(tcoords->GetName());
          }
        }
      }

      // if there is an exact correspondence between normals and vertices then can simply
      // assign the normals as point data
      if (hasNormals && normals_same_as_verts)
      {
        output->GetPointData()->SetNormals(normals);
      }

      if (hasMaterials)
      {
        // keep a record of the material for each cell
        for (svtkIdType celli = 0; celli < polys->GetNumberOfCells(); ++celli)
        {
          const auto citer = startCellToMatName.find(celli);
          if (citer != startCellToMatName.end())
          {
            const std::string& matname = citer->second;
            matid = matNameToId.find(matname)->second;
          }
          matIds->InsertNextValue(matid);
        }
        output->GetCellData()->AddArray(matIds);
        output->GetFieldData()->AddArray(matNames);
      }

      if (hasGroups && faceScalars)
      {
        output->GetCellData()->AddArray(faceScalars);
      }

      output->Squeeze();
    }
    // otherwise we can duplicate the vertices as necessary (a bit slower)
    else
    {
      svtkDebugMacro(<< "Duplicating vertices so that tcoords and normals are correct");

      svtkPoints* new_points = svtkPoints::New();
      std::vector<svtkFloatArray*> new_tcoords_vector;
      for (const auto& iter : tcoords_map)
      {
        svtkFloatArray* tcoords = iter.second;
        svtkFloatArray* new_tcoords = svtkFloatArray::New();
        new_tcoords->SetName(tcoords->GetName());
        new_tcoords->SetNumberOfComponents(2);
        new_tcoords_vector.push_back(new_tcoords);
      }
      svtkFloatArray* new_normals = svtkFloatArray::New();
      new_normals->SetNumberOfComponents(3);
      new_normals->SetName("Normals");
      svtkCellArray* new_polys = svtkCellArray::New();

      // for each poly, copy its vertices into new_points (and point at them)
      // also copy its tcoords into new_tcoords
      // also copy its normals into new_normals
      polys->InitTraversal();
      tcoord_polys->InitTraversal();
      normal_polys->InitTraversal();

      svtkIdType n_pts;
      const svtkIdType* pts;
      svtkIdType n_tcoord_pts;
      const svtkIdType* tcoord_pts;
      svtkIdType n_normal_pts;
      const svtkIdType* normal_pts;

      svtkNew<svtkIdList> tmpCell;

      for (svtkIdType celli = 0; celli < polys->GetNumberOfCells(); ++celli)
      {
        polys->GetNextCell(n_pts, pts);
        tcoord_polys->GetNextCell(n_tcoord_pts, tcoord_pts);
        normal_polys->GetNextCell(n_normal_pts, normal_pts);

        if (hasMaterials)
        {
          // keep a record of the material for each cell
          const auto citer = startCellToMatName.find(celli);
          if (citer != startCellToMatName.end())
          {
            const std::string& matname = citer->second;
            matid = matNameToId.find(matname)->second;
          }
        }
        // If some vertices have tcoords and not others (likewise normals)
        // then we must do something else SVTK will complain. (crash on render attempt)
        // Easiest solution is to delete polys that don't have complete tcoords (if there
        // are any tcoords in the dataset) or normals (if there are any normals in the dataset).

        if ((n_pts != n_tcoord_pts && hasTCoords) || (n_pts != n_normal_pts && hasNormals))
        {
          // skip this poly
          svtkDebugMacro(<< "Skipping poly " << celli + 1 << " (1-based index)");
        }
        else
        {
          tmpCell->SetNumberOfIds(n_pts);
          // copy the corresponding points, tcoords and normals across
          for (svtkIdType pointi = 0; pointi < n_pts; ++pointi)
          {
            // copy the tcoord for this point across (if there is one)
            if (n_tcoord_pts > 0)
            {
              size_t k = 0;
              for (const auto& iter : tcoords_map)
              {
                svtkFloatArray* tcoords = iter.second;
                svtkFloatArray* new_tcoords = new_tcoords_vector.at(k);
                new_tcoords->InsertNextTuple(tcoords->GetTuple(tcoord_pts[pointi]));
                ++k;
              }
            }
            // copy the normal for this point across (if there is one)
            if (n_normal_pts > 0)
            {
              new_normals->InsertNextTuple(normals->GetTuple(normal_pts[pointi]));
            }
            // copy the vertex into the new structure and update
            // the vertex index in the polys structure (pts is a pointer into it)
            tmpCell->SetId(pointi, new_points->InsertNextPoint(points->GetPoint(pts[pointi])));
          }
          polys->ReplaceCellAtId(celli, tmpCell);
          // copy this poly (pointing at the new points) into the new polys list
          new_polys->InsertNextCell(tmpCell);
          if (hasMaterials)
          {
            matIds->InsertNextValue(matid);
          }
        }
      }

      // use the new structures for the output
      output->SetPoints(new_points);
      output->SetPolys(new_polys);
      if (hasTCoords)
      {
        bool setTcoords = true;

        for (svtkFloatArray* new_tcoords : new_tcoords_vector)
        {
          output->GetPointData()->AddArray(new_tcoords);
          if (setTcoords)
          {
            setTcoords = false;
            output->GetPointData()->SetActiveTCoords(new_tcoords->GetName());
          }
        }
      }
      if (hasNormals)
      {
        output->GetPointData()->SetNormals(new_normals);
      }
      if (hasMaterials)
      {
        output->GetCellData()->AddArray(matIds);
        output->GetFieldData()->AddArray(matNames);
      }

      if (hasGroups && faceScalars)
      {
        output->GetCellData()->AddArray(faceScalars);
      }

      // TODO: fixup for pointElems and lineElems too

      output->Squeeze();

      new_points->Delete();
      new_polys->Delete();
      for (svtkFloatArray* new_tcoords : new_tcoords_vector)
      {
        new_tcoords->Delete();
      }
      new_normals->Delete();
    }
  }

  if (faceScalars)
  {
    faceScalars->Delete();
  }

  points->Delete();
  for (auto iter : tcoords_map)
  {
    iter.second->Delete();
  }
  normals->Delete();
  polys->Delete();
  tcoord_polys->Delete();
  normal_polys->Delete();

  lineElems->Delete();
  pointElems->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkOBJReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Comment: " << (this->Comment ? this->Comment : "(none)") << "\n";
}
