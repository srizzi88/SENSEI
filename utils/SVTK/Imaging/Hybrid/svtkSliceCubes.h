/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliceCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSliceCubes
 * @brief   generate isosurface(s) from volume four slices at a time
 *
 * svtkSliceCubes is a special version of the marching cubes filter. Instead
 * of ingesting an entire volume at once it processes only four slices at
 * a time. This way, it can generate isosurfaces from huge volumes. Also, the
 * output of this object is written to a marching cubes triangle file. That
 * way, output triangles do not need to be held in memory.
 *
 * To use svtkSliceCubes you must specify an instance of svtkVolumeReader to
 * read the data. Set this object up with the proper file prefix, image range,
 * data origin, data dimensions, header size, data mask, and swap bytes flag.
 * The svtkSliceCubes object will then take over and read slices as necessary.
 * You also will need to specify the name of an output marching cubes triangle
 * file.
 *
 * @warning
 * This process object is both a source and mapper (i.e., it reads and writes
 * data to a file). This is different than the other marching cubes objects
 * (and most process objects in the system). It's specialized to handle very
 * large data.
 *
 * @warning
 * This object only extracts a single isosurface. This compares with the other
 * contouring objects in svtk that generate multiple surfaces.
 *
 * @warning
 * To read the output file use svtkMCubesReader.
 *
 * @sa
 * svtkMarchingCubes svtkContourFilter svtkMCubesReader svtkDividingCubes svtkVolumeReader
 */

#ifndef svtkSliceCubes_h
#define svtkSliceCubes_h

#include "svtkImagingHybridModule.h" // For export macro
#include "svtkObject.h"

class svtkVolumeReader;

class SVTKIMAGINGHYBRID_EXPORT svtkSliceCubes : public svtkObject
{
public:
  static svtkSliceCubes* New();
  svtkTypeMacro(svtkSliceCubes, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // methods to make it look like a filter
  void Write() { this->Update(); }
  void Update();

  //@{
  /**
   * Set/get object to read slices.
   */
  virtual void SetReader(svtkVolumeReader*);
  svtkGetObjectMacro(Reader, svtkVolumeReader);
  //@}

  //@{
  /**
   * Specify file name of marching cubes output file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/get isosurface contour value.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
  //@}

  //@{
  /**
   * Specify file name of marching cubes limits file. The limits file
   * speeds up subsequent reading of output triangle file.
   */
  svtkSetStringMacro(LimitsFileName);
  svtkGetStringMacro(LimitsFileName);
  //@}

protected:
  svtkSliceCubes();
  ~svtkSliceCubes() override;

  void Execute();

  svtkVolumeReader* Reader;
  char* FileName;
  double Value;
  char* LimitsFileName;

private:
  svtkSliceCubes(const svtkSliceCubes&) = delete;
  void operator=(const svtkSliceCubes&) = delete;
};

#endif
