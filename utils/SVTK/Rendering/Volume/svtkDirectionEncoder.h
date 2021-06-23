/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDirectionEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkDirectionEncoder
 * @brief   encode a direction into a one or two byte value
 *
 *
 * Given a direction, encode it into an integer value. This value should
 * be less than 65536, which is the maximum number of encoded directions
 * supported by this superclass. A direction encoded is used to encode
 * normals in a volume for use during volume rendering, and the
 * amount of space that is allocated per normal is 2 bytes.
 * This is an abstract superclass - see the subclasses for specific
 * implementation details.
 *
 * @sa
 * svtkRecursiveSphereDirectionEncoder
 */

#ifndef svtkDirectionEncoder_h
#define svtkDirectionEncoder_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class SVTKRENDERINGVOLUME_EXPORT svtkDirectionEncoder : public svtkObject
{
public:
  //@{
  /**
   * Get the name of this class
   */
  svtkTypeMacro(svtkDirectionEncoder, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Given a normal vector n, return the encoded direction
   */
  virtual int GetEncodedDirection(float n[3]) = 0;

  /**
   * / Given an encoded value, return a pointer to the normal vector
   */
  virtual float* GetDecodedGradient(int value) SVTK_SIZEHINT(3) = 0;

  /**
   * Return the number of encoded directions
   */
  virtual int GetNumberOfEncodedDirections(void) = 0;

  /**
   * Get the decoded gradient table. There are
   * this->GetNumberOfEncodedDirections() entries in the table, each
   * containing a normal (direction) vector. This is a flat structure -
   * 3 times the number of directions floats in an array.
   */
  virtual float* GetDecodedGradientTable(void) = 0;

protected:
  svtkDirectionEncoder() {}
  ~svtkDirectionEncoder() override {}

private:
  svtkDirectionEncoder(const svtkDirectionEncoder&) = delete;
  void operator=(const svtkDirectionEncoder&) = delete;
};

#endif
