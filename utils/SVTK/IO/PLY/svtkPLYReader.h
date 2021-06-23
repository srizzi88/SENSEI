/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPLYReader
 * @brief   read Stanford University PLY polygonal file format
 *
 * svtkPLYReader is a source object that reads polygonal data in
 * Stanford University PLY file format (see
 * http://graphics.stanford.edu/data/3Dscanrep). It requires that
 * the elements "vertex" and "face" are defined. The "vertex" element
 * must have the properties "x", "y", and "z". The "face" element must
 * have the property "vertex_indices" defined. Optionally, if the "face"
 * element has the properties "intensity" and/or the triplet "red",
 * "green", "blue", and optionally "alpha"; these are read and added as scalars
 * to the output data.
 * If the "face" element has the property "texcoord" a new TCoords
 * point array is created and points are duplicated if they have 2 or
 * more different texture coordinates. Points are duplicated only if
 * DuplicatePointsForFaceTexture is true (default).
 * This creates a polygonal data that can be textured without
 * artifacts. If unique points are required use a svtkCleanPolyData
 * filter after this reader or use this reader with DuplicatePointsForFaceTexture
 * set to false.
 *
 * @sa
 * svtkPLYWriter, svtkCleanPolyData
 */

#ifndef svtkPLYReader_h
#define svtkPLYReader_h

#include "svtkAbstractPolyDataReader.h"
#include "svtkIOPLYModule.h" // For export macro

class svtkStringArray;

class SVTKIOPLY_EXPORT svtkPLYReader : public svtkAbstractPolyDataReader
{
public:
  svtkTypeMacro(svtkPLYReader, svtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with merging set to true.
   */
  static svtkPLYReader* New();

  /**
   * A simple, non-exhaustive check to see if a file is a valid ply file.
   */
  static int CanReadFile(const char* filename);

  svtkGetObjectMacro(Comments, svtkStringArray);

  /**
   * Tolerance used to detect different texture coordinates for shared
   * points for faces.
   */
  svtkGetMacro(FaceTextureTolerance, float);
  svtkSetMacro(FaceTextureTolerance, float);

  //@{
  /**
   * Enable reading from an InputString instead of the default, a file.
   * Note that reading from an input stream would be more flexible (enabling
   * other kind of streams) and possibly more efficient because we don't need
   * to save the whole stream to a string. However a stream interface
   * does not translate well to python and the string interface satisfies
   * our current needs. So we leave the stream interface for future work.
   */
  svtkSetMacro(ReadFromInputString, bool);
  svtkGetMacro(ReadFromInputString, bool);
  svtkBooleanMacro(ReadFromInputString, bool);
  void SetInputString(const std::string& s) { this->InputString = s; }
  //@}

  /**
   * If true (default) and the "face" element has the property "texcoord" duplicate
   * face points if they have 2 or more different texture coordinates.
   * Otherwise, each texture coordinate for a face point overwrites previously set
   * texture coordinates for that point.
   */
  svtkGetMacro(DuplicatePointsForFaceTexture, bool);
  svtkSetMacro(DuplicatePointsForFaceTexture, bool);

protected:
  svtkPLYReader();
  ~svtkPLYReader() override;

  svtkStringArray* Comments;
  // Whether this object is reading from a string or a file.
  // Default is 0: read from file.
  bool ReadFromInputString;
  // The input string.
  std::string InputString;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPLYReader(const svtkPLYReader&) = delete;
  void operator=(const svtkPLYReader&) = delete;

  float FaceTextureTolerance;
  bool DuplicatePointsForFaceTexture;
};

#endif
