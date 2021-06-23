/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageConnector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageConnector
 * @brief   Create a binary image of a sphere.
 *
 * svtkImageConnector is a helper class for connectivity filters.
 * It is not meant to be used directly.
 * It implements a stack and breadth first search necessary for
 * some connectivity filters.  Filtered axes sets the dimensionality
 * of the neighbor comparison, and
 * cannot be more than three dimensions.
 * As implemented, only voxels which share faces are considered
 * neighbors.
 */

#ifndef svtkImageConnector_h
#define svtkImageConnector_h

#include "svtkImagingMorphologicalModule.h" // For export macro
#include "svtkObject.h"

class svtkImageData;

//
// Special classes for manipulating data
//
// For the breadth first search
class svtkImageConnectorSeed
{ //;prevent man page generation
public:
  static svtkImageConnectorSeed* New() { return new svtkImageConnectorSeed; }
  void* Pointer;
  int Index[3];
  svtkImageConnectorSeed* Next;
};

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageConnector : public svtkObject
{
public:
  static svtkImageConnector* New();

  svtkTypeMacro(svtkImageConnector, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkImageConnectorSeed* NewSeed(int index[3], void* ptr);
  void AddSeed(svtkImageConnectorSeed* seed);
  void AddSeedToEnd(svtkImageConnectorSeed* seed);

  void RemoveAllSeeds();

  //@{
  /**
   * Values used by the MarkRegion method
   */
  svtkSetMacro(ConnectedValue, unsigned char);
  svtkGetMacro(ConnectedValue, unsigned char);
  svtkSetMacro(UnconnectedValue, unsigned char);
  svtkGetMacro(UnconnectedValue, unsigned char);
  //@}

  /**
   * Input a data of 0's and "UnconnectedValue"s. Seeds of this object are
   * used to find connected pixels.  All pixels connected to seeds are set to
   * ConnectedValue.  The data has to be unsigned char.
   */
  void MarkData(svtkImageData* data, int dimensionality, int ext[6]);

protected:
  svtkImageConnector();
  ~svtkImageConnector() override;

  unsigned char ConnectedValue;
  unsigned char UnconnectedValue;

  svtkImageConnectorSeed* PopSeed();

  svtkImageConnectorSeed* Seeds;
  svtkImageConnectorSeed* LastSeed;

private:
  svtkImageConnector(const svtkImageConnector&) = delete;
  void operator=(const svtkImageConnector&) = delete;
};

#endif
