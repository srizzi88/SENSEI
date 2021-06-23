/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTIFFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTIFFReader
 * @brief   read TIFF files
 *
 * svtkTIFFReader is a source object that reads TIFF files.
 * It should be able to read almost any TIFF file
 *
 * @sa
 * svtkTIFFWriter
 */

#ifndef svtkTIFFReader_h
#define svtkTIFFReader_h

#include "svtkImageReader2.h"

class SVTKIOIMAGE_EXPORT svtkTIFFReader : public svtkImageReader2
{
public:
  static svtkTIFFReader* New();
  svtkTypeMacro(svtkTIFFReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Is the given file name a tiff file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".tif .tiff"; }

  /**
   * Return a descriptive name for the file format that might be useful
   * in a GUI.
   */
  const char* GetDescriptiveName() override { return "TIFF"; }

  /**
   * Set orientation type
   * ORIENTATION_TOPLEFT         1       (row 0 top, col 0 lhs)
   * ORIENTATION_TOPRIGHT        2       (row 0 top, col 0 rhs)
   * ORIENTATION_BOTRIGHT        3       (row 0 bottom, col 0 rhs)
   * ORIENTATION_BOTLEFT         4       (row 0 bottom, col 0 lhs)
   * ORIENTATION_LEFTTOP         5       (row 0 lhs, col 0 top)
   * ORIENTATION_RIGHTTOP        6       (row 0 rhs, col 0 top)
   * ORIENTATION_RIGHTBOT        7       (row 0 rhs, col 0 bottom)
   * ORIENTATION_LEFTBOT         8       (row 0 lhs, col 0 bottom)
   * User need to explicitly include svtk_tiff.h header to have access to those #define
   */
  void SetOrientationType(unsigned int orientationType);
  svtkGetMacro(OrientationType, unsigned int);

  //@{
  /**
   * Get method to check if orientation type is specified.
   */
  svtkGetMacro(OrientationTypeSpecifiedFlag, bool);
  //@}

  //@{
  /**
   * Set/get methods to see if manual origin has been set.
   */
  svtkSetMacro(OriginSpecifiedFlag, bool);
  svtkGetMacro(OriginSpecifiedFlag, bool);
  svtkBooleanMacro(OriginSpecifiedFlag, bool);
  //@}

  //@{
  /**
   * Set/get if the spacing flag has been specified.
   */
  svtkSetMacro(SpacingSpecifiedFlag, bool);
  svtkGetMacro(SpacingSpecifiedFlag, bool);
  svtkBooleanMacro(SpacingSpecifiedFlag, bool);
  //@}

  //@{
  /**
   * When set to true (default false), TIFFTAG_COLORMAP, if any, will be
   * ignored.
   */
  svtkSetMacro(IgnoreColorMap, bool);
  svtkGetMacro(IgnoreColorMap, bool);
  svtkBooleanMacro(IgnoreColorMap, bool);
  //@}
protected:
  svtkTIFFReader();
  ~svtkTIFFReader() override;

  enum
  {
    NOFORMAT,
    RGB,
    GRAYSCALE,
    PALETTE_RGB,
    PALETTE_GRAYSCALE,
    OTHER
  };

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;

  class svtkTIFFReaderInternal;
  svtkTIFFReaderInternal* InternalImage;

private:
  svtkTIFFReader(const svtkTIFFReader&) = delete;
  void operator=(const svtkTIFFReader&) = delete;

  /**
   * Evaluates the image at a single pixel location.
   */
  template <typename T>
  int EvaluateImageAt(T* out, T* in);

  /**
   * Look up color paletter values.
   */
  void GetColor(int index, unsigned short* r, unsigned short* g, unsigned short* b);

  // To support Zeiss images
  void ReadTwoSamplesPerPixelImage(void* out, unsigned int svtkNotUsed(width), unsigned int height);

  unsigned int GetFormat();

  /**
   * Auxiliary methods used by the reader internally.
   */
  void Initialize();

  /**
   * Internal method, do not use.
   */
  template <typename T>
  void ReadImageInternal(T* buffer);

  /**
   * Reads 3D data from multi-pages tiff.
   */
  template <typename T>
  void ReadVolume(T* buffer);

  /**
   * Reads 3D data from tiled tiff
   */
  void ReadTiles(void* buffer);

  /**
   * Reads a generic image.
   */
  template <typename T>
  void ReadGenericImage(T* out, unsigned int width, unsigned int height);

  /**
   * Dispatch template to determine pixel type and decide on reader actions.
   */
  template <typename T>
  void Process(T* outPtr, int outExtent[6], svtkIdType outIncr[3]);

  /**
   * Second layer of dispatch necessary for some TIFF types.
   */
  template <typename T>
  void Process2(T* outPtr, int* outExt);

  unsigned short* ColorRed;
  unsigned short* ColorGreen;
  unsigned short* ColorBlue;
  int TotalColors;
  unsigned int ImageFormat;
  int OutputExtent[6];
  svtkIdType OutputIncrements[3];
  unsigned int OrientationType;
  bool OrientationTypeSpecifiedFlag;
  bool OriginSpecifiedFlag;
  bool SpacingSpecifiedFlag;
  bool IgnoreColorMap;
};

#endif
