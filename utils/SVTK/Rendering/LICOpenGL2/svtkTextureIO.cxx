/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextureIO.h"

#include "svtkCellData.h"
#include "svtkDataSetWriter.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPixelBufferObject.h"
#include "svtkPixelExtent.h"
#include "svtkPixelTransfer.h"
#include "svtkPointData.h"
#include "svtkTextureObject.h"
#include "svtkXMLMultiBlockDataWriter.h"
#include <cstddef>
#include <deque>
#include <sstream>

using std::deque;
using std::ostringstream;
using std::string;

//----------------------------------------------------------------------------
static svtkFloatArray* DownloadTexture(svtkTextureObject* texture, const unsigned int* sub)
{
  int tt = texture->GetSVTKDataType();
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();
  unsigned int tnc = texture->GetComponents();

  svtkPixelExtent texExt(0U, tw - 1U, 0U, th - 1U);
  svtkPixelExtent subExt(texExt);
  if (sub)
  {
    subExt.SetData(sub);
  }

  svtkFloatArray* ta = svtkFloatArray::New();
  ta->SetNumberOfComponents(tnc);
  ta->SetNumberOfTuples(static_cast<svtkIdType>(subExt.Size()));
  ta->SetName("tex");
  float* pTa = ta->GetPointer(0);

  svtkPixelBufferObject* pbo = texture->Download();

  svtkPixelTransfer::Blit(
    texExt, subExt, subExt, subExt, tnc, tt, pbo->MapPackedBuffer(), tnc, SVTK_FLOAT, pTa);

  pbo->UnmapPackedBuffer();
  pbo->Delete();

  return ta;
}

//----------------------------------------------------------------------------
void svtkTextureIO::Write(
  const char* filename, svtkTextureObject* texture, const unsigned int* subset, const double* origin)
{
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();

  svtkPixelExtent subExt(0U, tw - 1U, 0U, th - 1U);
  if (subset)
  {
    subExt.SetData(subset);
  }

  int dataExt[6] = { 0, 0, 0, 0, 0, 0 };
  subExt.CellToNode();
  subExt.GetData(dataExt);

  double dataOrigin[6] = { 0, 0, 0, 0, 0, 0 };
  if (origin)
  {
    dataOrigin[0] = origin[0];
    dataOrigin[1] = origin[1];
  }

  svtkFloatArray* ta = DownloadTexture(texture, subset);

  svtkImageData* id = svtkImageData::New();
  id->SetExtent(dataExt);
  id->SetOrigin(dataOrigin);
  id->GetCellData()->AddArray(ta);
  ta->Delete();

  svtkDataSetWriter* w = svtkDataSetWriter::New();
  cerr << "writing to: " << filename << endl;
  w->SetFileName(filename);
  w->SetInputData(id);
  w->Write();

  id->Delete();
  w->Delete();
}

//----------------------------------------------------------------------------
void svtkTextureIO::Write(const char* filename, svtkTextureObject* texture,
  const deque<svtkPixelExtent>& exts, const double* origin)
{
  int n = static_cast<int>(exts.size());
  if (n == 0)
  {
    // svtkGenericWarningMacro("Empty extents nothing will be written");
    return;
  }
  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();
  for (int i = 0; i < n; ++i)
  {
    svtkPixelExtent ext = exts[i];
    if (ext.Empty())
      continue;

    svtkFloatArray* ta = DownloadTexture(texture, ext.GetDataU());

    int dataExt[6] = { 0, 0, 0, 0, 0, 0 };
    ext.CellToNode();
    ext.GetData(dataExt);

    double dataOrigin[6] = { 0, 0, 0, 0, 0, 0 };
    if (origin)
    {
      dataOrigin[0] = origin[0];
      dataOrigin[1] = origin[1];
    }

    svtkImageData* id = svtkImageData::New();
    id->SetExtent(dataExt);
    id->SetOrigin(dataOrigin);
    id->GetCellData()->AddArray(ta);
    ta->Delete();

    mb->SetBlock(i, id);
    id->Delete();
  }

  svtkXMLMultiBlockDataWriter* w = svtkXMLMultiBlockDataWriter::New();
  cerr << "writing to: " << filename << endl;
  w->SetFileName(filename);
  w->SetInputData(mb);
  w->Write();
  w->Delete();
  mb->Delete();
}
