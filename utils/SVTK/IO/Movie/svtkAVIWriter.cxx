/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAVIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAVIWriter.h"
#include "svtkWindows.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkErrorCode.h"

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

#include <vfw.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

class svtkAVIWriterInternal
{
public:
  PAVISTREAM Stream;
  PAVISTREAM StreamCompressed;
  PAVIFILE AVIFile;
  LPBITMAPINFOHEADER lpbi; // pointer to BITMAPINFOHEADER
  HANDLE hDIB;             // handle to DIB, temp handle
};

//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkAVIWriter);

//---------------------------------------------------------------------------
svtkAVIWriter::svtkAVIWriter()
{
  this->Internals = new svtkAVIWriterInternal;
  this->Internals->Stream = nullptr;
  this->Internals->StreamCompressed = nullptr;
  this->Internals->AVIFile = nullptr;
  this->Time = 0;
  this->Quality = 10000;
  this->Rate = 1000;
  this->Internals->hDIB = nullptr; // handle to DIB, temp handle
  this->PromptCompressionOptions = 0;
  this->CompressorFourCC = nullptr;
  this->SetCompressorFourCC("MSVC");
}

//---------------------------------------------------------------------------
svtkAVIWriter::~svtkAVIWriter()
{
  if (this->Internals->AVIFile)
  {
    this->End();
  }
  delete this->Internals;
  this->SetCompressorFourCC(nullptr);
}

//---------------------------------------------------------------------------
void svtkAVIWriter::Start()
{
  // Error checking
  this->Error = 1;
  if (this->GetInput() == nullptr)
  {
    svtkErrorMacro(<< "Write:Please specify an input!");
    this->SetErrorCode(svtkGenericMovieWriter::NoInputError);
    return;
  }
  if (!this->FileName)
  {
    svtkErrorMacro(<< "Write:Please specify a FileName");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return;
  }

  // Fill in image information.
  this->GetInputAlgorithm(0, 0)->UpdateInformation();
  int wExtent[6];
  this->GetInputInformation(0, 0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

  LONG hr;
  AVISTREAMINFO strhdr;

  AVIFileInit();
  // opens AVIFile library
  hr = AVIFileOpen(&this->Internals->AVIFile, this->FileName, OF_WRITE | OF_CREATE, 0L);
  if (hr != 0)
  {
    svtkErrorMacro("Unable to open " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return;
  }

  // Fill in the header for the video stream....
  // The video stream will run in 15ths of a second....
  memset(&strhdr, 0, sizeof(strhdr));
  strhdr.fccType = streamtypeVIDEO; // stream type
  strhdr.fccHandler = 0;
  strhdr.dwScale = 1;
  strhdr.dwRate = this->Rate;
  strhdr.dwQuality = (DWORD)-1;
  strhdr.dwSuggestedBufferSize = (wExtent[1] - wExtent[0] + 1) * (wExtent[3] - wExtent[2] + 1) * 3;
  SetRect(&strhdr.rcFrame, 0, 0, wExtent[1] - wExtent[0] + 1, wExtent[3] - wExtent[2] + 1);

  // And create the stream;
  AVIFileCreateStream(this->Internals->AVIFile, // file pointer
    &this->Internals->Stream,                   // returned stream pointer
    &strhdr);                                   // stream header

  // do not want to display this dialog
  AVICOMPRESSOPTIONS opts;
  AVICOMPRESSOPTIONS FAR* aopts[1] = { &opts };
  memset(&opts, 0, sizeof(opts));

  // need to setup opts
  opts.fccType = 0;
  char fourcc[4] = { ' ', ' ', ' ', ' ' };
  if (this->CompressorFourCC)
  {
    size_t len = strlen(this->CompressorFourCC);
    memcpy(fourcc, this->CompressorFourCC, len > 4 ? 4 : len);
  }
  opts.fccHandler = mmioFOURCC(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  switch (this->GetQuality())
  {
    case 0:
      opts.dwQuality = 2500;
      break;
    case 1:
      opts.dwQuality = 5000;
      break;
    default:
      opts.dwQuality = 10000;
      break;
  }

  opts.dwBytesPerSecond = 0;
  opts.dwFlags = AVICOMPRESSF_VALID;

  if (this->PromptCompressionOptions)
  {
    if (!AVISaveOptions(nullptr, 0, 1, &this->Internals->Stream, (LPAVICOMPRESSOPTIONS FAR*)&aopts))
    {
      svtkErrorMacro("Unable to save " << this->FileName);
      return;
    }
  }

  int avierr = AVIMakeCompressedStream(
    &this->Internals->StreamCompressed, this->Internals->Stream, &opts, nullptr);
  if (avierr != AVIERR_OK)
  {
    svtkErrorMacro("Unable to compress "
      << this->FileName << ": "
      << (avierr == AVIERR_NOCOMPRESSOR
             ? "unknown compressor"
             : (avierr == AVIERR_MEMORY
                   ? "not enough memory"
                   : (avierr == AVIERR_UNSUPPORTED ? "unsupported data type" : "unknown error"))));
    this->SetErrorCode(svtkGenericMovieWriter::CanNotCompress);
    return;
  }

  DWORD dwLen; // size of memory block
  int dataWidth = (((wExtent[1] - wExtent[0] + 1) * 3 + 3) / 4) * 4;

  dwLen = sizeof(BITMAPINFOHEADER) + dataWidth * (wExtent[3] - wExtent[2] + 1);
  this->Internals->hDIB = ::GlobalAlloc(GHND, dwLen);
  this->Internals->lpbi = (LPBITMAPINFOHEADER)::GlobalLock(this->Internals->hDIB);

  this->Internals->lpbi->biSize = sizeof(BITMAPINFOHEADER);
  this->Internals->lpbi->biWidth = wExtent[1] - wExtent[0] + 1;
  this->Internals->lpbi->biHeight = wExtent[3] - wExtent[2] + 1;
  this->Internals->lpbi->biPlanes = 1;
  this->Internals->lpbi->biBitCount = 24;
  this->Internals->lpbi->biCompression = BI_RGB;
  this->Internals->lpbi->biClrUsed = 0;
  this->Internals->lpbi->biClrImportant = 0;
  this->Internals->lpbi->biSizeImage = dataWidth * (wExtent[3] - wExtent[2] + 1);

  if (AVIStreamSetFormat(
        this->Internals->StreamCompressed, 0, this->Internals->lpbi, this->Internals->lpbi->biSize))
  {
    svtkErrorMacro("Unable to format "
      << this->FileName
      << " Most likely this means that the video compression scheme you selected could not handle "
         "the data. Try selecting a different compression scheme.");
    this->SetErrorCode(svtkGenericMovieWriter::CanNotFormat);
    return;
  }

  this->Error = 0;
  this->Time = 0;
}

//---------------------------------------------------------------------------
void svtkAVIWriter::Write()
{
  if (this->Error)
  {
    return;
  }

  // get the data
  svtkImageData* input = this->GetImageDataInput(0);
  this->GetInputAlgorithm(0, 0)->UpdateWholeExtent();
  int* wExtent = input->GetExtent();

  // get the pointer to the data
  unsigned char* ptr = (unsigned char*)(input->GetScalarPointer());

  int dataWidth = (((wExtent[1] - wExtent[0] + 1) * 3 + 3) / 4) * 4;
  int srcWidth = (wExtent[1] - wExtent[0] + 1) * 3;

  // copy the data to the clipboard
  unsigned char* dest = (unsigned char*)this->Internals->lpbi + this->Internals->lpbi->biSize;
  int i, j;
  for (i = 0; i < this->Internals->lpbi->biHeight; i++)
  {
    for (j = 0; j < this->Internals->lpbi->biWidth; j++)
    {
      *dest++ = ptr[2];
      *dest++ = ptr[1];
      *dest++ = *ptr;
      ptr += 3;
    }
    dest = dest + (dataWidth - srcWidth);
  }

  AVIStreamWrite(this->Internals->StreamCompressed, // stream pointer
    this->Time,                                     // time of this frame
    1,                                              // number to write
    (LPBYTE)this->Internals->lpbi +                 // pointer to data
      this->Internals->lpbi->biSize,
    this->Internals->lpbi->biSizeImage, // size of this frame
    AVIIF_KEYFRAME,                     // flags....
    nullptr, nullptr);
  this->Time++;
}

//---------------------------------------------------------------------------
void svtkAVIWriter::End()
{
  ::GlobalUnlock(this->Internals->hDIB);
  if (this->Internals->Stream)
  {
    AVIStreamClose(this->Internals->Stream);
    this->Internals->Stream = nullptr;
  }

  if (this->Internals->StreamCompressed)
  {
    AVIStreamClose(this->Internals->StreamCompressed);
    this->Internals->StreamCompressed = nullptr;
  }

  if (this->Internals->AVIFile)
  {
    AVIFileClose(this->Internals->AVIFile);
    this->Internals->AVIFile = nullptr;
  }

  AVIFileExit(); // releases AVIFile library
}

//---------------------------------------------------------------------------
void svtkAVIWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Rate: " << this->Rate << endl;
  os << indent << "Quality: " << this->Quality << endl;
  os << indent
     << "PromptCompressionOptions: " << (this->GetPromptCompressionOptions() ? "on" : "off")
     << endl;
  os << indent
     << "CompressorFourCC: " << (this->CompressorFourCC ? this->CompressorFourCC : "(None)")
     << endl;
}
