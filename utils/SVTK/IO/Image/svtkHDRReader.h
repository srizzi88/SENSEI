/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHDRReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHDRReader
 * @brief   read Radiance HDR files
 *
 * svtkHDRReader is a source object that reads Radiance HDR files.
 * HDR files are converted into 32 bit images.
 */

#ifndef svtkHDRReader_h
#define svtkHDRReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader.h"
#include <string>
#include <vector>

class SVTKIOIMAGE_EXPORT svtkHDRReader : public svtkImageReader
{
public:
  static svtkHDRReader* New();
  svtkTypeMacro(svtkHDRReader, svtkImageReader);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum FormatType
  {
    FORMAT_32BIT_RLE_RGBE = 0,
    FORMAT_32BIT_RLE_XYZE
  };

  //@{
  /**
   * Format is either 32-bit_rle_rgbe or 32-bit_rle_xyze.
   */
  svtkGetMacro(Format, int);
  //@}

  //@{
  /**
   * Get gamma correction.
   * Default value is 1.0.
   */
  svtkGetMacro(Gamma, double);
  //@}

  //@{
  /**
   * Get exposure.
   * Default value is 1.0.
   */
  svtkGetMacro(Exposure, double);
  //@}

  //@{
  /**
   * Get pixel aspect, the ratio of height by the width of a pixel.
   * Default value is 1.0.
   */
  svtkGetMacro(PixelAspect, double);
  //@}

  /**
   * Is the given file a HDR file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".hdr .pic"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Radiance HDR"; }

protected:
  svtkHDRReader();
  ~svtkHDRReader() override;

  std::string ProgramType;
  FormatType Format;
  double Gamma;
  double Exposure;
  double PixelAspect;

  /**
   * If true, the X axis has been flipped.
   */
  bool FlippedX = false;

  /**
   * If true, the Y axis is the X, and the height and width has been swapped.
   */
  bool SwappedAxis = false;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;
  bool HDRReaderUpdateSlice(float* outPtr, int* outExt);
  void HDRReaderUpdate(svtkImageData* data, float* outPtr);

  /**
   * If the stream has an error, close the file and return true.
   * Else return false.
   */
  bool HasError(istream* is);

  int GetWidth() const;
  int GetHeight() const;

  /**
   * Read the header data and fill attributes of HDRReader, as well as DataExtent.
   * Return true if the read succeed, else false.
   */
  bool ReadHeaderData();

  void ConvertAllDataFromRGBToXYZ(float* outPtr, int size);

  void FillOutPtrRLE(int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer);
  void FillOutPtrNoRLE(int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer);

  /**
   * Read the file from is into outPtr with no RLE encoding.
   * Return false if a reading error occured, else true.
   */
  bool ReadAllFileNoRLE(istream* is, float* outPtr, int decrPtr, int* outExt);

  /**
   * Read a line of the file from is into lineBuffer with RLE encoding.
   * Return false if a reading error occured, else true.
   */
  bool ReadLineRLE(istream* is, unsigned char* lineBufferPtr);

  /**
   * Standard conversion from rgbe to float pixels
   */
  void RGBE2Float(unsigned char rgbe[4], float& r, float& g, float& b);

  /**
   * Conversion from xyz to rgb float using the 3x3 convert matrix.
   * Inplace version, r,g,b are in xyz color space in input, in rgb color space
   * in output
   */
  static void XYZ2RGB(const float convertMatrix[3][3], float& r, float& g, float& b);

private:
  svtkHDRReader(const svtkHDRReader&) = delete;
  void operator=(const svtkHDRReader&) = delete;
};
#endif
