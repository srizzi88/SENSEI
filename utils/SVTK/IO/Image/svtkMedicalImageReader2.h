/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMedicalImageReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMedicalImageReader2
 * @brief   svtkImageReader2 with medical meta data.
 *
 * svtkMedicalImageReader2 is a parent class for medical image readers.
 * It provides a place to store patient information that may be stored
 * in the image header.
 * @sa
 * svtkImageReader2 svtkGESignaReader svtkMedicalImageProperties
 */

#ifndef svtkMedicalImageReader2_h
#define svtkMedicalImageReader2_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class svtkMedicalImageProperties;

class SVTKIOIMAGE_EXPORT svtkMedicalImageReader2 : public svtkImageReader2
{
public:
  static svtkMedicalImageReader2* New();
  svtkTypeMacro(svtkMedicalImageReader2, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the medical image properties object
   */
  svtkGetObjectMacro(MedicalImageProperties, svtkMedicalImageProperties);
  //@}

  //@{
  /**
   * For backward compatibility, propagate calls to the MedicalImageProperties
   * object.
   */
  virtual void SetPatientName(const char*);
  virtual const char* GetPatientName();
  virtual void SetPatientID(const char*);
  virtual const char* GetPatientID();
  virtual void SetDate(const char*);
  virtual const char* GetDate();
  virtual void SetSeries(const char*);
  virtual const char* GetSeries();
  virtual void SetStudy(const char*);
  virtual const char* GetStudy();
  virtual void SetImageNumber(const char*);
  virtual const char* GetImageNumber();
  virtual void SetModality(const char*);
  virtual const char* GetModality();
  //@}

protected:
  svtkMedicalImageReader2();
  ~svtkMedicalImageReader2() override;

  /**
   * Medical Image properties
   */
  svtkMedicalImageProperties* MedicalImageProperties;

private:
  svtkMedicalImageReader2(const svtkMedicalImageReader2&) = delete;
  void operator=(const svtkMedicalImageReader2&) = delete;
};

#endif
