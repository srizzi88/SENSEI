/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageEllipsoidSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageEllipsoidSource
 * @brief   Create a binary image of an ellipsoid.
 *
 * svtkImageEllipsoidSource creates a binary image of a ellipsoid.  It was created
 * as an example of a simple source, and to test the mask filter.
 * It is also used internally in svtkImageDilateErode3D.
 */

#ifndef svtkImageEllipsoidSource_h
#define svtkImageEllipsoidSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageEllipsoidSource : public svtkImageAlgorithm
{
public:
  static svtkImageEllipsoidSource* New();
  svtkTypeMacro(svtkImageEllipsoidSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetWholeExtent(int extent[6]);
  int* GetWholeExtent() SVTK_SIZEHINT(6) { return this->WholeExtent; }
  //@}

  //@{
  /**
   * Set/Get the center of the ellipsoid.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the radius of the ellipsoid.
   */
  svtkSetVector3Macro(Radius, double);
  svtkGetVector3Macro(Radius, double);
  //@}

  //@{
  /**
   * Set/Get the inside pixel values.
   */
  svtkSetMacro(InValue, double);
  svtkGetMacro(InValue, double);
  //@}

  //@{
  /**
   * Set/Get the outside pixel values.
   */
  svtkSetMacro(OutValue, double);
  svtkGetMacro(OutValue, double);
  //@}

  //@{
  /**
   * Set what type of scalar data this source should generate.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(SVTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(SVTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(SVTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(SVTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(SVTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(SVTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(SVTK_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(SVTK_UNSIGNED_CHAR); }
  //@}

protected:
  svtkImageEllipsoidSource();
  ~svtkImageEllipsoidSource() override;

  int WholeExtent[6];
  double Center[3];
  double Radius[3];
  double InValue;
  double OutValue;
  int OutputScalarType;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageEllipsoidSource(const svtkImageEllipsoidSource&) = delete;
  void operator=(const svtkImageEllipsoidSource&) = delete;
};

#endif
