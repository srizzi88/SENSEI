/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSeedConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSeedConnectivity
 * @brief   SeedConnectivity with user defined seeds.
 *
 * svtkImageSeedConnectivity marks pixels connected to user supplied seeds.
 * The input must be unsigned char, and the output is also unsigned char.  If
 * a seed supplied by the user does not have pixel value "InputTrueValue",
 * then the image is scanned +x, +y, +z until a pixel is encountered with
 * value "InputTrueValue".  This new pixel is used as the seed .  Any pixel
 * with out value "InputTrueValue" is consider off.  The output pixels values
 * are 0 for any off pixel in input, "OutputTrueValue" for any pixels
 * connected to seeds, and "OutputUnconnectedValue" for any on pixels not
 * connected to seeds.  The same seeds are used for all images in the image
 * set.
 */

#ifndef svtkImageSeedConnectivity_h
#define svtkImageSeedConnectivity_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkImageConnector;
class svtkImageConnectorSeed;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageSeedConnectivity : public svtkImageAlgorithm
{
public:
  static svtkImageSeedConnectivity* New();
  svtkTypeMacro(svtkImageSeedConnectivity, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods for manipulating the seed pixels.
   */
  void RemoveAllSeeds();
  void AddSeed(int num, int* index);
  void AddSeed(int i0, int i1, int i2);
  void AddSeed(int i0, int i1);
  //@}

  //@{
  /**
   * Set/Get what value is considered as connecting pixels.
   */
  svtkSetMacro(InputConnectValue, unsigned char);
  svtkGetMacro(InputConnectValue, unsigned char);
  //@}

  //@{
  /**
   * Set/Get the value to set connected pixels to.
   */
  svtkSetMacro(OutputConnectedValue, unsigned char);
  svtkGetMacro(OutputConnectedValue, unsigned char);
  //@}

  //@{
  /**
   * Set/Get the value to set unconnected pixels to.
   */
  svtkSetMacro(OutputUnconnectedValue, unsigned char);
  svtkGetMacro(OutputUnconnectedValue, unsigned char);
  //@}

  //@{
  /**
   * Get the svtkImageCOnnector used by this filter.
   */
  svtkGetObjectMacro(Connector, svtkImageConnector);
  //@}

  //@{
  /**
   * Set the number of axes to use in connectivity.
   */
  svtkSetMacro(Dimensionality, int);
  svtkGetMacro(Dimensionality, int);
  //@}

protected:
  svtkImageSeedConnectivity();
  ~svtkImageSeedConnectivity() override;

  unsigned char InputConnectValue;
  unsigned char OutputConnectedValue;
  unsigned char OutputUnconnectedValue;
  svtkImageConnectorSeed* Seeds;
  svtkImageConnector* Connector;
  int Dimensionality;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageSeedConnectivity(const svtkImageSeedConnectivity&) = delete;
  void operator=(const svtkImageSeedConnectivity&) = delete;
};

#endif
