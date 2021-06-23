/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLCellToSVTKCellMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLCellToSVTKCellMap
 * @brief   OpenGL rendering utility functions
 *
 * svtkOpenGLCellToSVTKCellMap provides functions map from opengl primitive ID to svtk
 *
 *
 */

#ifndef svtkOpenGLCellToSVTKCellMap_h
#define svtkOpenGLCellToSVTKCellMap_h

#include "svtkNew.h" // for ivars
#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkStateStorage.h"           // used for ivars

class svtkCellArray;
class svtkPoints;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLCellToSVTKCellMap : public svtkObject
{
public:
  static svtkOpenGLCellToSVTKCellMap* New();
  svtkTypeMacro(svtkOpenGLCellToSVTKCellMap, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Create supporting arrays that are needed when rendering cell data
  // Some SVTK cells have to be broken into smaller cells for OpenGL
  // When we have cell data we have to map cell attributes from the SVTK
  // cell number to the actual OpenGL cell
  //
  // The same concept applies to cell based picking
  //
  void BuildCellSupportArrays(svtkCellArray * [4], int representation, svtkPoints* points);

  void BuildPrimitiveOffsetsIfNeeded(svtkCellArray * [4], int representation, svtkPoints* points);

  svtkIdType ConvertOpenGLCellIdToSVTKCellId(bool pointPicking, svtkIdType openGLId);

  // rebuilds if needed
  void Update(svtkCellArray** prims, int representation, svtkPoints* points);

  size_t GetSize() { return this->CellCellMap.size(); }

  svtkIdType* GetPrimitiveOffsets() { return this->PrimitiveOffsets; }

  svtkIdType GetValue(size_t i) { return this->CellCellMap[i]; }

  // what offset should verts start at
  void SetStartOffset(svtkIdType start);

  svtkIdType GetFinalOffset() { return this->PrimitiveOffsets[3] + this->CellMapSizes[3]; }

protected:
  svtkOpenGLCellToSVTKCellMap();
  ~svtkOpenGLCellToSVTKCellMap() override;

  std::vector<svtkIdType> CellCellMap;
  svtkIdType CellMapSizes[4];
  svtkIdType PrimitiveOffsets[4];
  int BuildRepresentation;
  int StartOffset = 0;
  svtkStateStorage MapBuildState;
  svtkStateStorage TempState;

private:
  svtkOpenGLCellToSVTKCellMap(const svtkOpenGLCellToSVTKCellMap&) = delete;
  void operator=(const svtkOpenGLCellToSVTKCellMap&) = delete;
};

#endif
