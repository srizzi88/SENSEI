/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPNMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPNMReader.h"

#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkPNMReader);

static char svtkPNMReaderGetChar(FILE* fp)
{
  char c;
  int result;

  if ((result = getc(fp)) == EOF)
  {
    return '\0';
  }

  c = (char)result;
  if (c == '#')
  {
    do
    {
      if ((result = getc(fp)) == EOF)
      {
        return '\0';
      }
      c = (char)result;
    } while (c != '\n');
  }

  return c;
}

static int svtkPNMReaderGetInt(FILE* fp)
{
  char c;
  int result = 0;

  do
  {
    c = svtkPNMReaderGetChar(fp);
  } while ((c < '1') || (c > '9'));
  do
  {
    result = result * 10 + (c - '0');
    c = svtkPNMReaderGetChar(fp);
  } while ((c >= '0') && (c <= '9'));

  // put the CR/LF or whitespace back.....
  ungetc(c, fp);
  return result;
}

void svtkPNMReader::ExecuteInformation()
{
  int xsize, ysize, comp;
  char magic[80];
  char c;
  FILE* fp;

  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4] == 0 && this->DataExtent[5] == 0 &&
    (this->DataVOI[4] || this->DataVOI[5]))
  {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
  }

  if (!this->FileName && !this->FilePattern)
  {
    svtkErrorMacro(<< "Either a FileName or FilePattern must be specified.");
    return;
  }

  // Allocate the space for the filename
  this->ComputeInternalFileName(this->DataExtent[4]);

  // get the magic number by reading in a file
  fp = svtksys::SystemTools::Fopen(this->InternalFileName, "rb");
  if (!fp)
  {
    svtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
  }

  do
  {
    c = svtkPNMReaderGetChar(fp);
    if (c == '\0')
    { // Bad file.
      int invalidExtent[6] = { 0, -1, 0, -1, 0, -1 };
      svtkStreamingDemandDrivenPipeline::SetWholeExtent(
        this->GetOutputInformation(0), invalidExtent);
      fclose(fp);
      return;
    }
  } while (c != 'P');
  magic[0] = c;
  magic[1] = svtkPNMReaderGetChar(fp);
  magic[2] = '\0';

  // now get the dimensions
  xsize = svtkPNMReaderGetInt(fp);
  ysize = svtkPNMReaderGetInt(fp);

  // read max pixel value into comp for now
  svtkPNMReaderGetInt(fp);
  // if file is ascii, any amount of whitespace may follow.
  // if file is binary, a single whitespace character will follow.
  // We only support binary ppm and pgm files right now.  So the next
  // character IS always ignored.
  c = getc(fp);

  // if this file was "written" on the PC, then a CR will have been
  // written as a CR/LF combination.  So, if this single whitespace
  // character is a CR and it is followed by a LF, then swallow the
  // linefeed character as well. (Not part of the PPM standard, but a
  // a hard fact of life.
  if (c == 0x0d)
  {
    c = getc(fp);
    if (c != 0x0a)
    {
      ungetc(c, fp);
    }
  }

  // Set the header size now that we have parsed it
  this->SetHeaderSize(ftell(fp));

  fclose(fp);

  // compare magic number to determine file type
  if (!strcmp(magic, "P5"))
  {
    comp = 1;
  }
  else if (!strcmp(magic, "P6"))
  {
    comp = 3;
  }
  else
  {
    svtkErrorMacro(<< "Unknown file type! " << this->InternalFileName
                  << " is not a binary PGM or PPM!");
    return;
  }

  // if the user has set the VOI, just make sure its valid
  if (this->DataVOI[0] || this->DataVOI[1] || this->DataVOI[2] || this->DataVOI[3] ||
    this->DataVOI[4] || this->DataVOI[5])
  {
    if ((this->DataVOI[0] < 0) || (this->DataVOI[1] >= xsize) || (this->DataVOI[2] < 0) ||
      (this->DataVOI[3] >= ysize))
    {
      svtkWarningMacro(
        "The requested VOI is larger than the file's (" << this->InternalFileName << ") extent ");
      this->DataVOI[0] = 0;
      this->DataVOI[1] = xsize - 1;
      this->DataVOI[2] = 0;
      this->DataVOI[3] = ysize - 1;
    }
  }

  this->DataExtent[0] = 0;
  this->DataExtent[1] = xsize - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = ysize - 1;

  this->SetDataScalarTypeToUnsignedChar();
  this->SetNumberOfScalarComponents(comp);

  this->svtkImageReader::ExecuteInformation();
}

inline int iseol(int c)
{
  return c == 10 || c == 13;
}

int svtkPNMReader::CanReadFile(const char* fname)
{
  FILE* fp = svtksys::SystemTools::Fopen(fname, "rb");
  if (!fp)
  {
    return 0;
  }
  unsigned char magic[3];
  if (fread(magic, 1, 3, fp) != 3)
  {
    fclose(fp);
    return 0;
  }
  int ok = ((magic[0] == 'P') && iseol(magic[2]) && (magic[1] >= '1' && magic[1] <= '6'));
  fclose(fp);
  if (ok)
  {
    return 3;
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkPNMReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
