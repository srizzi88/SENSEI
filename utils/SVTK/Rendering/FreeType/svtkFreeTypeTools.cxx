/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFreeTypeTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkFreeTypeTools.h"

#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPath.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

#include "svtkStdString.h"
#include "svtkUnicodeString.h"

// The embedded fonts
#include "fonts/svtkEmbeddedFonts.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

// Print debug info
#define SVTK_FTFC_DEBUG 0
#define SVTK_FTFC_DEBUG_CD 0

namespace
{
// Some helper functions:
void rotateVector2i(svtkVector2i& vec, float sinTheta, float cosTheta)
{
  int x = static_cast<int>(std::round(cosTheta * vec[0] - sinTheta * vec[1]));
  int y = static_cast<int>(std::round(sinTheta * vec[0] + cosTheta * vec[1]));
  vec = svtkVector2i(x, y);
}

} // end anon namespace

class svtkTextPropertyLookup : public std::map<size_t, svtkSmartPointer<svtkTextProperty> >
{
public:
  bool contains(const size_t id) { return this->find(id) != this->end(); }
};

class svtkFreeTypeTools::MetaData
{
public:
  // Set by PrepareMetaData
  svtkTextProperty* textProperty;
  size_t textPropertyCacheId;
  size_t unrotatedTextPropertyCacheId;
  FTC_ScalerRec scaler;
  FTC_ScalerRec unrotatedScaler;
  FT_Face face;
  bool faceHasKerning;
  bool faceIsRotated;
  FT_Matrix rotation;
  FT_Matrix inverseRotation;

  // Set by CalculateBoundingBox
  svtkVector2i ascent;  // Vector from baseline to top of characters
  svtkVector2i descent; // Vector from baseline to bottom of characters
  int height;
  struct LineMetrics
  {
    svtkVector2i origin;
    int width;
    // bbox relative to origin[XY]:
    int xmin;
    int xmax;
    int ymin;
    int ymax;
  };
  svtkVector2i dx; // Vector representing the data width after rotation
  svtkVector2i dy; // Vector representing the data height after rotation
  svtkVector2i TL; // Top left corner of the rotated data
  svtkVector2i TR; // Top right corner of the rotated data
  svtkVector2i BL; // Bottom left corner of the rotated data
  svtkVector2i BR; // Bottom right corner of the rotated data
  std::vector<LineMetrics> lineMetrics;
  int maxLineWidth;
  svtkTuple<int, 4> bbox;
};

class svtkFreeTypeTools::ImageMetaData : public svtkFreeTypeTools::MetaData
{
public:
  // Set by PrepareImageMetaData
  int imageDimensions[3];
  svtkIdType imageIncrements[3];
  unsigned char rgba[4];
};

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup counter
svtkFreeTypeTools* svtkFreeTypeTools::Instance;
static unsigned int svtkFreeTypeToolsCleanupCounter;

//----------------------------------------------------------------------------
// The embedded fonts
// Create a lookup table between the text mapper attributes
// and the font buffers.
struct EmbeddedFontStruct
{
  size_t length;
  unsigned char* ptr;
};

//------------------------------------------------------------------------------
// Clean up the svtkFreeTypeTools instance at exit. Using a separate class allows
// us to delay initialization of the svtkFreeTypeTools class.
svtkFreeTypeToolsCleanup::svtkFreeTypeToolsCleanup()
{
  svtkFreeTypeToolsCleanupCounter++;
}

svtkFreeTypeToolsCleanup::~svtkFreeTypeToolsCleanup()
{
  if (--svtkFreeTypeToolsCleanupCounter == 0)
  {
    svtkFreeTypeTools::SetInstance(nullptr);
  }
}

//----------------------------------------------------------------------------
svtkFreeTypeTools* svtkFreeTypeTools::GetInstance()
{
  if (!svtkFreeTypeTools::Instance)
  {
    svtkFreeTypeTools::Instance =
      static_cast<svtkFreeTypeTools*>(svtkObjectFactory::CreateInstance("svtkFreeTypeTools"));
    if (!svtkFreeTypeTools::Instance)
    {
      svtkFreeTypeTools::Instance = new svtkFreeTypeTools;
      svtkFreeTypeTools::Instance->InitializeObjectBase();
    }
  }
  return svtkFreeTypeTools::Instance;
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::SetInstance(svtkFreeTypeTools* instance)
{
  if (svtkFreeTypeTools::Instance == instance)
  {
    return;
  }

  if (svtkFreeTypeTools::Instance)
  {
    svtkFreeTypeTools::Instance->Delete();
  }

  svtkFreeTypeTools::Instance = instance;

  // User will call ->Delete() after setting instance

  if (instance)
  {
    instance->Register(nullptr);
  }
}

//----------------------------------------------------------------------------
svtkFreeTypeTools::svtkFreeTypeTools()
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::svtkFreeTypeTools\n");
#endif
  // Force use of compiled fonts by default.
  this->ForceCompiledFonts = true;
  this->DebugTextures = false;
  this->MaximumNumberOfFaces = 30; // combinations of family+bold+italic
  this->MaximumNumberOfSizes = this->MaximumNumberOfFaces * 20; // sizes
  this->MaximumNumberOfBytes = 300000UL * this->MaximumNumberOfSizes;
  this->TextPropertyLookup = new svtkTextPropertyLookup();
  this->CacheManager = nullptr;
  this->ImageCache = nullptr;
  this->CMapCache = nullptr;
  this->ScaleToPowerTwo = true;

  // Ideally this should be thread-local to support SMP:
  FT_Error err;
  this->Library = new FT_Library;
  err = FT_Init_FreeType(this->Library);
  if (err)
  {
    svtkErrorMacro("FreeType library initialization failed with error code: " << err << ".");
    delete this->Library;
    this->Library = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkFreeTypeTools::~svtkFreeTypeTools()
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::~svtkFreeTypeTools\n");
#endif
  this->ReleaseCacheManager();
  delete TextPropertyLookup;

  FT_Done_FreeType(*this->Library);
  delete this->Library;
  this->Library = nullptr;
}

//----------------------------------------------------------------------------
FT_Library* svtkFreeTypeTools::GetLibrary()
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetLibrary\n");
#endif

  return this->Library;
}

//----------------------------------------------------------------------------
svtkFreeTypeTools::FaceMetrics svtkFreeTypeTools::GetFaceMetrics(svtkTextProperty* tprop)
{
  FT_Face face;
  this->GetFace(tprop, &face);

  FaceMetrics metrics;
  metrics.UnitsPerEM = face->units_per_EM;
  metrics.Ascender = face->ascender;
  metrics.Descender = face->descender;
  metrics.HorizAdvance = face->max_advance_width;
  metrics.BoundingBox = { { static_cast<int>(face->bbox.xMin), static_cast<int>(face->bbox.xMax),
    static_cast<int>(face->bbox.yMin), static_cast<int>(face->bbox.yMax) } };
  metrics.FamilyName = face->family_name;
  metrics.Scalable = (face->face_flags & FT_FACE_FLAG_SCALABLE) != 0;
  metrics.Bold = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
  metrics.Italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

  return metrics;
}

//----------------------------------------------------------------------------
svtkFreeTypeTools::GlyphOutline svtkFreeTypeTools::GetUnscaledGlyphOutline(
  svtkTextProperty* tprop, svtkUnicodeStringValueType charId)
{
  size_t tpropCacheId;
  this->MapTextPropertyToId(tprop, &tpropCacheId);
  FTC_FaceID faceId = reinterpret_cast<FTC_FaceID>(tpropCacheId);
  GlyphOutline result;
  result.HorizAdvance = 0;

  FTC_CMapCache* cmapCache = this->GetCMapCache();
  if (!cmapCache)
  {
    svtkErrorMacro("CMapCache not found!");
    return result;
  }

  FT_UInt glyphId = FTC_CMapCache_Lookup(*cmapCache, faceId, 0, charId);

  FTC_ImageCache* imgCache = this->GetImageCache();
  if (!imgCache)
  {
    svtkErrorMacro("ImageCache not found!");
    return result;
  }

  FTC_ImageTypeRec type;
  type.face_id = faceId;
  type.width = 0;
  type.height = 0;
  type.flags = FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_TRANSFORM;

  FT_Glyph glyph;
  FT_Error error = FTC_ImageCache_Lookup(*imgCache, &type, glyphId, &glyph, nullptr);
  if (!error && glyph && glyph->format == ft_glyph_format_outline)
  {
    FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
    result.HorizAdvance = (glyph->advance.x + 0x8000) >> 16;
    result.Path = svtkSmartPointer<svtkPath>::New();
    this->OutlineToPath(0, 0, &outlineGlyph->outline, result.Path);
  }

  return result;
}

//----------------------------------------------------------------------------
std::array<int, 2> svtkFreeTypeTools::GetUnscaledKerning(
  svtkTextProperty* tprop, svtkUnicodeStringValueType leftChar, svtkUnicodeStringValueType rightChar)
{
  std::array<int, 2> result{ { 0, 0 } };
  if (leftChar == 0 || rightChar == 0)
  {
    return result;
  }

  size_t tpropCacheId;
  this->MapTextPropertyToId(tprop, &tpropCacheId);
  FT_Face face = nullptr;

  if (!this->GetFace(tpropCacheId, &face) || !face)
  {
    svtkErrorMacro("Error loading font face.");
    return result;
  }

  if (FT_HAS_KERNING(face) != 0)
  {
    FTC_FaceID faceId = reinterpret_cast<FTC_FaceID>(tpropCacheId);
    FTC_CMapCache* cmapCache = this->GetCMapCache();
    if (!cmapCache)
    {
      svtkErrorMacro("CMapCache not found!");
      return result;
    }

    FT_UInt leftGIdx = FTC_CMapCache_Lookup(*cmapCache, faceId, 0, leftChar);
    FT_UInt rightGIdx = FTC_CMapCache_Lookup(*cmapCache, faceId, 0, rightChar);
    FT_Vector kerning;
    FT_Error error = FT_Get_Kerning(face, leftGIdx, rightGIdx, FT_KERNING_UNSCALED, &kerning);
    if (!error)
    {
      result[0] = kerning.x >> 6;
      result[1] = kerning.y >> 6;
    }
  }

  return result;
}

//----------------------------------------------------------------------------
FTC_Manager* svtkFreeTypeTools::GetCacheManager()
{
  if (!this->CacheManager)
  {
    this->InitializeCacheManager();
  }

  return this->CacheManager;
}

//----------------------------------------------------------------------------
FTC_ImageCache* svtkFreeTypeTools::GetImageCache()
{
  if (!this->ImageCache)
  {
    this->InitializeCacheManager();
  }

  return this->ImageCache;
}

//----------------------------------------------------------------------------
FTC_CMapCache* svtkFreeTypeTools::GetCMapCache()
{
  if (!this->CMapCache)
  {
    this->InitializeCacheManager();
  }

  return this->CMapCache;
}

//----------------------------------------------------------------------------
FT_CALLBACK_DEF(FT_Error)
svtkFreeTypeToolsFaceRequester(
  FTC_FaceID face_id, FT_Library lib, FT_Pointer request_data, FT_Face* face)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeToolsFaceRequester()\n");
#endif

  // Get a pointer to the current svtkFreeTypeTools object
  svtkFreeTypeTools* self = reinterpret_cast<svtkFreeTypeTools*>(request_data);

  // Map the ID to a text property
  svtkSmartPointer<svtkTextProperty> tprop = svtkSmartPointer<svtkTextProperty>::New();
  self->MapIdToTextProperty(reinterpret_cast<intptr_t>(face_id), tprop);

  bool faceIsSet = self->LookupFace(tprop, lib, face);

  if (!faceIsSet)
  {
    return static_cast<FT_Error>(1);
  }

  if (tprop->GetOrientation() != 0.0)
  {
    // FreeType documentation says that the transform should not be set
    // but we cache faces also by transform, so that there is a unique
    // (face, orientation) cache entry
    FT_Matrix matrix;
    float angle = svtkMath::RadiansFromDegrees(tprop->GetOrientation());
    matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);
    FT_Set_Transform(*face, &matrix, nullptr);
  }

  return static_cast<FT_Error>(0);
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::InitializeCacheManager()
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::InitializeCacheManager()\n");
#endif

  this->ReleaseCacheManager();

  FT_Error error;

  // Create the cache manager itself
  this->CacheManager = new FTC_Manager;

  error = this->CreateFTCManager();

  if (error)
  {
    svtkErrorMacro(<< "Failed allocating a new FreeType Cache Manager");
  }

  // The image cache
  this->ImageCache = new FTC_ImageCache;
  error = FTC_ImageCache_New(*this->CacheManager, this->ImageCache);

  if (error)
  {
    svtkErrorMacro(<< "Failed allocating a new FreeType Image Cache");
  }

  // The charmap cache
  this->CMapCache = new FTC_CMapCache;
  error = FTC_CMapCache_New(*this->CacheManager, this->CMapCache);

  if (error)
  {
    svtkErrorMacro(<< "Failed allocating a new FreeType CMap Cache");
  }
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::ReleaseCacheManager()
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::ReleaseCacheManager()\n");
#endif

  if (this->CacheManager)
  {
    FTC_Manager_Done(*this->CacheManager);

    delete this->CacheManager;
    this->CacheManager = nullptr;
  }

  delete this->ImageCache;
  this->ImageCache = nullptr;

  delete this->CMapCache;
  this->CMapCache = nullptr;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetBoundingBox(
  svtkTextProperty* tprop, const svtkStdString& str, int dpi, int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr or zero");
    return false;
  }

  if (str.empty())
  {
    std::fill(bbox, bbox + 4, 0);
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      memcpy(bbox, metaData.bbox.GetData(), sizeof(int) * 4);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetBoundingBox(
  svtkTextProperty* tprop, const svtkUnicodeString& str, int dpi, int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr or zero");
    return false;
  }

  if (str.empty())
  {
    std::fill(bbox, bbox + 4, 0);
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      memcpy(bbox, metaData.bbox.GetData(), sizeof(int) * 4);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetMetrics(
  svtkTextProperty* tprop, const svtkStdString& str, int dpi, svtkTextRenderer::Metrics& metrics)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "nullptr text property.");
    return false;
  }

  if (str.empty())
  {
    metrics = svtkTextRenderer::Metrics();
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      metrics.BoundingBox = metaData.bbox;
      metrics.TopLeft = metaData.TL;
      metrics.TopRight = metaData.TR;
      metrics.BottomLeft = metaData.BL;
      metrics.BottomRight = metaData.BR;
      metrics.Ascent = metaData.ascent;
      metrics.Descent = metaData.descent;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetMetrics(
  svtkTextProperty* tprop, const svtkUnicodeString& str, int dpi, svtkTextRenderer::Metrics& metrics)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "nullptr text property.");
    return false;
  }

  if (str.empty())
  {
    metrics = svtkTextRenderer::Metrics();
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      metrics.BoundingBox = metaData.bbox;
      metrics.TopLeft = metaData.TL;
      metrics.TopRight = metaData.TR;
      metrics.BottomLeft = metaData.BL;
      metrics.BottomRight = metaData.BR;
      metrics.Ascent = metaData.ascent;
      metrics.Descent = metaData.descent;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::RenderString(
  svtkTextProperty* tprop, const svtkStdString& str, int dpi, svtkImageData* data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, dpi, data, textDims);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::RenderString(
  svtkTextProperty* tprop, const svtkUnicodeString& str, int dpi, svtkImageData* data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, dpi, data, textDims);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::StringToPath(
  svtkTextProperty* tprop, const svtkStdString& str, int dpi, svtkPath* path)
{
  return this->StringToPathInternal(tprop, str, dpi, path);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::StringToPath(
  svtkTextProperty* tprop, const svtkUnicodeString& str, int dpi, svtkPath* path)
{
  return this->StringToPathInternal(tprop, str, dpi, path);
}

//----------------------------------------------------------------------------
int svtkFreeTypeTools::GetConstrainedFontSize(
  const svtkStdString& str, svtkTextProperty* tprop, int dpi, int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
  {
    svtkErrorMacro(<< "Could not prepare metadata.");
    return false;
  }
  return this->FitStringToBBox(str, metaData, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
int svtkFreeTypeTools::GetConstrainedFontSize(
  const svtkUnicodeString& str, svtkTextProperty* tprop, int dpi, int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
  {
    svtkErrorMacro(<< "Could not prepare metadata.");
    return false;
  }
  return this->FitStringToBBox(str, metaData, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
svtkTypeUInt16 svtkFreeTypeTools::HashString(const char* str)
{
  if (str == nullptr)
    return 0;

  svtkTypeUInt16 hash = 0;
  while (*str != 0)
  {
    svtkTypeUInt8 high = ((hash << 8) ^ hash) >> 8;
    svtkTypeUInt8 low = tolower(*str) ^ (hash << 2);
    hash = (high << 8) ^ low;
    ++str;
  }

  return hash;
}

//----------------------------------------------------------------------------
svtkTypeUInt32 svtkFreeTypeTools::HashBuffer(const void* buffer, size_t n, svtkTypeUInt32 hash)
{
  if (buffer == nullptr)
  {
    return 0;
  }

  const char* key = reinterpret_cast<const char*>(buffer);

  // Jenkins hash function
  for (size_t i = 0; i < n; ++i)
  {
    hash += key[i];
    hash += (hash << 10);
    hash += (hash << 15);
  }

  return hash;
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::MapTextPropertyToId(svtkTextProperty* tprop, size_t* id)
{
  if (!tprop || !id)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr");
    return;
  }

  // The font family is hashed into 16 bits (= 17 bits so far)
  const char* fontFamily =
    tprop->GetFontFamily() != SVTK_FONT_FILE ? tprop->GetFontFamilyAsString() : tprop->GetFontFile();
  size_t fontFamilyLength = 0;
  if (fontFamily)
  {
    fontFamilyLength = strlen(fontFamily);
  }
  svtkTypeUInt32 hash = svtkFreeTypeTools::HashBuffer(fontFamily, fontFamilyLength);

  // Create a "string" of text properties
  unsigned char ucValue = tprop->GetBold();
  hash = svtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  ucValue = tprop->GetItalic();
  hash = svtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  ucValue = tprop->GetShadow();
  hash = svtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  hash = svtkFreeTypeTools::HashBuffer(tprop->GetColor(), 3 * sizeof(double), hash);
  double dValue = tprop->GetOpacity();
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = svtkFreeTypeTools::HashBuffer(tprop->GetBackgroundColor(), 3 * sizeof(double), hash);
  dValue = tprop->GetBackgroundOpacity();
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = svtkFreeTypeTools::HashBuffer(tprop->GetFrameColor(), 3 * sizeof(double), hash);
  ucValue = tprop->GetFrame();
  hash = svtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  int iValue = tprop->GetFrameWidth();
  hash = svtkFreeTypeTools::HashBuffer(&iValue, sizeof(int), hash);
  iValue = tprop->GetFontSize();
  hash = svtkFreeTypeTools::HashBuffer(&iValue, sizeof(int), hash);
  hash = svtkFreeTypeTools::HashBuffer(tprop->GetShadowOffset(), 2 * sizeof(int), hash);
  dValue = tprop->GetOrientation();
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  dValue = tprop->GetLineSpacing();
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  dValue = tprop->GetLineOffset();
  hash = svtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  iValue = tprop->GetUseTightBoundingBox();
  hash = svtkFreeTypeTools::HashBuffer(&iValue, sizeof(int), hash);

  // Set the first bit to avoid id = 0
  // (the id will be mapped to a pointer, FTC_FaceID, so let's avoid nullptr)
  *id = 1;

  // Add in the hash.
  // We're dropping a bit here, but that should be okay.
  *id |= hash << 1;

  // Insert the TextProperty into the lookup table
  if (!this->TextPropertyLookup->contains(*id))
    (*this->TextPropertyLookup)[*id] = tprop;
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::MapIdToTextProperty(size_t id, svtkTextProperty* tprop)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr");
    return;
  }

  svtkTextPropertyLookup::const_iterator tpropIt = this->TextPropertyLookup->find(id);

  if (tpropIt == this->TextPropertyLookup->end())
  {
    svtkErrorMacro(<< "Unknown id; call MapTextPropertyToId first!");
    return;
  }

  tprop->ShallowCopy(tpropIt->second);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetSize(size_t tprop_cache_id, int font_size, FT_Size* size)
{
  if (!size || font_size <= 0)
  {
    svtkErrorMacro(<< "Wrong parameters, size is nullptr or invalid font size");
    return 0;
  }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  FTC_ScalerRec scaler_rec;
  scaler_rec.face_id = face_id;
  scaler_rec.width = font_size;
  scaler_rec.height = font_size;
  scaler_rec.pixel = 1;

  return this->GetSize(&scaler_rec, size);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetSize(FTC_Scaler scaler, FT_Size* size)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetSize()\n");
#endif

  if (!size)
  {
    svtkErrorMacro(<< "Size is nullptr.");
    return 0;
  }

  FTC_Manager* manager = this->GetCacheManager();
  if (!manager)
  {
    svtkErrorMacro(<< "Failed querying the cache manager !");
    return 0;
  }

  FT_Error error = FTC_Manager_LookupSize(*manager, scaler, size);
  if (error)
  {
    svtkErrorMacro(<< "Failed looking up a FreeType Size");
  }

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetSize(svtkTextProperty* tprop, FT_Size* size)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "Wrong parameters, text property is nullptr");
    return 0;
  }

  // Map the text property to a unique id that will be used as face id
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetSize(tprop_cache_id, tprop->GetFontSize(), size);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetFace(size_t tprop_cache_id, FT_Face* face)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetFace()\n");
#endif

  if (!face)
  {
    svtkErrorMacro(<< "Wrong parameters, face is nullptr");
    return false;
  }

  FTC_Manager* manager = this->GetCacheManager();
  if (!manager)
  {
    svtkErrorMacro(<< "Failed querying the cache manager !");
    return false;
  }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  FT_Error error = FTC_Manager_LookupFace(*manager, face_id, face);
  if (error)
  {
    svtkErrorMacro(<< "Failed looking up a FreeType Face");
  }

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetFace(svtkTextProperty* tprop, FT_Face* face)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "Wrong parameters, face is nullptr");
    return 0;
  }

  // Map the text property to a unique id that will be used as face id
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetFace(tprop_cache_id, face);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetGlyphIndex(size_t tprop_cache_id, FT_UInt32 c, FT_UInt* gindex)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetGlyphIndex()\n");
#endif

  if (!gindex)
  {
    svtkErrorMacro(<< "Wrong parameters, gindex is nullptr");
    return 0;
  }

  FTC_CMapCache* cmap_cache = this->GetCMapCache();
  if (!cmap_cache)
  {
    svtkErrorMacro(<< "Failed querying the charmap cache manager !");
    return 0;
  }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  // Lookup the glyph index
  *gindex = FTC_CMapCache_Lookup(*cmap_cache, face_id, 0, c);

  return *gindex ? true : false;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetGlyphIndex(svtkTextProperty* tprop, FT_UInt32 c, FT_UInt* gindex)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "Wrong parameters, text property is nullptr");
    return 0;
  }

  // Map the text property to a unique id that will be used as face id
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetGlyphIndex(tprop_cache_id, c, gindex);
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetGlyph(
  size_t tprop_cache_id, int font_size, FT_UInt gindex, FT_Glyph* glyph, int request)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetGlyph()\n");
#endif

  if (!glyph)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr");
    return false;
  }

  FTC_ImageCache* image_cache = this->GetImageCache();
  if (!image_cache)
  {
    svtkErrorMacro(<< "Failed querying the image cache manager !");
    return false;
  }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  // Which font are we looking for
  FTC_ImageTypeRec image_type_rec;
  image_type_rec.face_id = face_id;
  image_type_rec.width = font_size;
  image_type_rec.height = font_size;
  image_type_rec.flags = FT_LOAD_DEFAULT;
  if (request == GLYPH_REQUEST_BITMAP)
  {
    image_type_rec.flags |= FT_LOAD_RENDER;
  }
  else if (request == GLYPH_REQUEST_OUTLINE)
  {
    image_type_rec.flags |= FT_LOAD_NO_BITMAP;
  }

  // Lookup the glyph
  FT_Error error = FTC_ImageCache_Lookup(*image_cache, &image_type_rec, gindex, glyph, nullptr);

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetGlyph(FTC_Scaler scaler, FT_UInt gindex, FT_Glyph* glyph, int request)
{
#if SVTK_FTFC_DEBUG_CD
  printf("svtkFreeTypeTools::GetGlyph()\n");
#endif

  if (!glyph)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr");
    return false;
  }

  FTC_ImageCache* image_cache = this->GetImageCache();
  if (!image_cache)
  {
    svtkErrorMacro(<< "Failed querying the image cache manager !");
    return false;
  }

  FT_ULong loadFlags = FT_LOAD_DEFAULT;
  if (request == GLYPH_REQUEST_BITMAP)
  {
    loadFlags |= FT_LOAD_RENDER;
  }
  else if (request == GLYPH_REQUEST_OUTLINE)
  {
    loadFlags |= FT_LOAD_NO_BITMAP;
  }

  // Lookup the glyph
  FT_Error error =
    FTC_ImageCache_LookupScaler(*image_cache, scaler, loadFlags, gindex, glyph, nullptr);

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::LookupFace(svtkTextProperty* tprop, FT_Library lib, FT_Face* face)
{
  // Fonts, organized by [Family][Bold][Italic]
  static EmbeddedFontStruct EmbeddedFonts[3][2][2] = {
    { { { // SVTK_ARIAL: Bold [ ] Italic [ ]
          face_arial_buffer_length, face_arial_buffer },
        { // SVTK_ARIAL: Bold [ ] Italic [x]
          face_arial_italic_buffer_length, face_arial_italic_buffer } },
      { { // SVTK_ARIAL: Bold [x] Italic [ ]
          face_arial_bold_buffer_length, face_arial_bold_buffer },
        { // SVTK_ARIAL: Bold [x] Italic [x]
          face_arial_bold_italic_buffer_length, face_arial_bold_italic_buffer } } },
    { { { // SVTK_COURIER: Bold [ ] Italic [ ]
          face_courier_buffer_length, face_courier_buffer },
        { // SVTK_COURIER: Bold [ ] Italic [x]
          face_courier_italic_buffer_length, face_courier_italic_buffer } },
      { { // SVTK_COURIER: Bold [x] Italic [ ]
          face_courier_bold_buffer_length, face_courier_bold_buffer },
        { // SVTK_COURIER: Bold [x] Italic [x]
          face_courier_bold_italic_buffer_length, face_courier_bold_italic_buffer } } },
    { { { // SVTK_TIMES: Bold [ ] Italic [ ]
          face_times_buffer_length, face_times_buffer },
        { // SVTK_TIMES: Bold [ ] Italic [x]
          face_times_italic_buffer_length, face_times_italic_buffer } },
      { { // SVTK_TIMES: Bold [x] Italic [ ]
          face_times_bold_buffer_length, face_times_bold_buffer },
        { // SVTK_TIMES: Bold [x] Italic [x]
          face_times_bold_italic_buffer_length, face_times_bold_italic_buffer } } }
  };

  int family = tprop->GetFontFamily();
  // If font family is unknown, fall back to Arial.
  if (family == SVTK_UNKNOWN_FONT)
  {
    svtkDebugWithObjectMacro(tprop, << "Requested font '" << tprop->GetFontFamilyAsString()
                                   << "'"
                                      " unavailable. Substituting Arial.");
    family = SVTK_ARIAL;
  }
  else if (family == SVTK_FONT_FILE)
  {
    svtkDebugWithObjectMacro(
      tprop, << "Attempting to load font from file: " << tprop->GetFontFile());

    if (FT_New_Face(lib, tprop->GetFontFile(), 0, face) == 0)
    {
      return true;
    }

    svtkDebugWithObjectMacro(tprop,
      << "Error loading font from file '" << tprop->GetFontFile() << "'. Falling back to arial.");
    family = SVTK_ARIAL;
  }

  FT_Long length =
    static_cast<FT_Long>(EmbeddedFonts[family][tprop->GetBold()][tprop->GetItalic()].length);
  FT_Byte* ptr = EmbeddedFonts[family][tprop->GetBold()][tprop->GetItalic()].ptr;

  // Create a new face from the embedded fonts if possible
  FT_Error error = FT_New_Memory_Face(lib, ptr, length, 0, face);

  if (error)
  {
    svtkErrorWithObjectMacro(tprop, << "Unable to create font !"
                                   << " (family: " << family << ", bold: " << tprop->GetBold()
                                   << ", italic: " << tprop->GetItalic() << ", length: " << length
                                   << ")");
    return false;
  }
  else
  {
#if SVTK_FTFC_DEBUG
    cout << "Requested: " << *face << " (F: " << tprop->GetFontFamily()
         << ", B: " << tprop->GetBold() << ", I: " << tprop->GetItalic()
         << ", O: " << tprop->GetOrientation() << ")" << endl;
#endif
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::GetGlyph(svtkTextProperty* tprop, FT_UInt32 c, FT_Glyph* glyph, int request)
{
  if (!tprop)
  {
    svtkErrorMacro(<< "Wrong parameters, text property is nullptr");
    return 0;
  }

  // Map the text property to a unique id that will be used as face id
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  // Get the character/glyph index
  FT_UInt gindex;
  if (!this->GetGlyphIndex(tprop_cache_id, c, &gindex))
  {
    svtkErrorMacro(<< "Failed querying a glyph index");
    return false;
  }

  // Get the glyph
  return this->GetGlyph(tprop_cache_id, tprop->GetFontSize(), gindex, glyph, request);
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaximumNumberOfFaces: " << this->MaximumNumberOfFaces << endl;

  os << indent << "MaximumNumberOfSizes: " << this->MaximumNumberOfSizes << endl;

  os << indent << "MaximumNumberOfBytes: " << this->MaximumNumberOfBytes << endl;

  os << indent << "Scale to nearest power of 2 for image sizes: " << this->ScaleToPowerTwo << endl;
}

//----------------------------------------------------------------------------
FT_Error svtkFreeTypeTools::CreateFTCManager()
{
  return FTC_Manager_New(*this->GetLibrary(), this->MaximumNumberOfFaces,
    this->MaximumNumberOfSizes, this->MaximumNumberOfBytes, svtkFreeTypeToolsFaceRequester,
    static_cast<FT_Pointer>(this), this->CacheManager);
}

//----------------------------------------------------------------------------
inline bool svtkFreeTypeTools::PrepareImageMetaData(
  svtkTextProperty* tprop, svtkImageData* image, ImageMetaData& metaData)
{
  // Image properties
  image->GetIncrements(metaData.imageIncrements);
  image->GetDimensions(metaData.imageDimensions);

  double color[3];
  tprop->GetColor(color);
  metaData.rgba[0] = static_cast<unsigned char>(color[0] * 255);
  metaData.rgba[1] = static_cast<unsigned char>(color[1] * 255);
  metaData.rgba[2] = static_cast<unsigned char>(color[2] * 255);
  metaData.rgba[3] = static_cast<unsigned char>(tprop->GetOpacity() * 255);

  return true;
}

//----------------------------------------------------------------------------
inline bool svtkFreeTypeTools::PrepareMetaData(svtkTextProperty* tprop, int dpi, MetaData& metaData)
{
  // Text properties
  metaData.textProperty = tprop;
  this->MapTextPropertyToId(tprop, &metaData.textPropertyCacheId);

  metaData.scaler.face_id = reinterpret_cast<FTC_FaceID>(metaData.textPropertyCacheId);
  metaData.scaler.width = tprop->GetFontSize() * 64; // 26.6 format point size
  metaData.scaler.height = tprop->GetFontSize() * 64;
  metaData.scaler.pixel = 0;
  metaData.scaler.x_res = dpi;
  metaData.scaler.y_res = dpi;

  FT_Size size;
  if (!this->GetSize(&metaData.scaler, &size))
  {
    return false;
  }

  metaData.face = size->face;
  metaData.faceHasKerning = (FT_HAS_KERNING(metaData.face) != 0);

  // Store an unrotated version of this font, as we'll need this to get accurate
  // ascenders/descenders (see CalculateBoundingBox).
  if (tprop->GetOrientation() != 0.0)
  {
    svtkNew<svtkTextProperty> unrotatedTProp;
    unrotatedTProp->ShallowCopy(tprop);
    unrotatedTProp->SetOrientation(0);
    this->MapTextPropertyToId(unrotatedTProp, &metaData.unrotatedTextPropertyCacheId);

    metaData.unrotatedScaler.face_id =
      reinterpret_cast<FTC_FaceID>(metaData.unrotatedTextPropertyCacheId);
    metaData.unrotatedScaler.width = tprop->GetFontSize() * 64; // 26.6 format point size
    metaData.unrotatedScaler.height = tprop->GetFontSize() * 64;
    metaData.unrotatedScaler.pixel = 0;
    metaData.unrotatedScaler.x_res = dpi;
    metaData.unrotatedScaler.y_res = dpi;
  }
  else
  {
    metaData.unrotatedTextPropertyCacheId = metaData.textPropertyCacheId;
    metaData.unrotatedScaler = metaData.scaler;
  }

  // Rotation matrices:
  metaData.faceIsRotated = (fabs(metaData.textProperty->GetOrientation()) > 1e-5);
  if (metaData.faceIsRotated)
  {
    float angle =
      svtkMath::RadiansFromDegrees(static_cast<float>(metaData.textProperty->GetOrientation()));
    // 0 -> orientation (used to adjust kerning, PR#15301)
    float c = cos(angle);
    float s = sin(angle);
    metaData.rotation.xx = (FT_Fixed)(c * 0x10000L);
    metaData.rotation.xy = (FT_Fixed)(-s * 0x10000L);
    metaData.rotation.yx = (FT_Fixed)(s * 0x10000L);
    metaData.rotation.yy = (FT_Fixed)(c * 0x10000L);

    // orientation -> 0 (used for width calculations)
    c = cos(-angle);
    s = sin(-angle);
    metaData.inverseRotation.xx = (FT_Fixed)(c * 0x10000L);
    metaData.inverseRotation.xy = (FT_Fixed)(-s * 0x10000L);
    metaData.inverseRotation.yx = (FT_Fixed)(s * 0x10000L);
    metaData.inverseRotation.yy = (FT_Fixed)(c * 0x10000L);
  }

  return true;
}

//----------------------------------------------------------------------------
template <typename StringType>
bool svtkFreeTypeTools::RenderStringInternal(
  svtkTextProperty* tprop, const StringType& str, int dpi, svtkImageData* data, int textDims[2])
{
  // Check parameters
  if (!tprop || !data)
  {
    svtkErrorMacro(<< "Wrong parameters, one of them is nullptr or zero");
    return false;
  }

  if (data->GetNumberOfScalarComponents() > 4)
  {
    svtkErrorMacro("The image data must have a maximum of four components");
    return false;
  }

  if (str.empty())
  {
    data->Initialize();
    if (textDims)
    {
      textDims[0] = 0;
      textDims[1] = 0;
    }
    return true;
  }

  ImageMetaData metaData;

  // Setup the metadata cache
  if (!this->PrepareMetaData(tprop, dpi, metaData))
  {
    svtkErrorMacro(<< "Error prepare text metadata.");
    return false;
  }

  // Calculate the bounding box.
  if (!this->CalculateBoundingBox(str, metaData))
  {
    svtkErrorMacro(<< "Could not get a valid bounding box.");
    return false;
  }

  // Calculate the text dimensions:
  if (textDims)
  {
    textDims[0] = metaData.bbox[1] - metaData.bbox[0] + 1;
    textDims[1] = metaData.bbox[3] - metaData.bbox[2] + 1;
  }

  // Prepare the ImageData to receive the text
  this->PrepareImageData(data, metaData.bbox.GetData());

  // Setup the image metadata
  if (!this->PrepareImageMetaData(tprop, data, metaData))
  {
    svtkErrorMacro(<< "Error prepare image metadata.");
    return false;
  }

  // Render the background:
  this->RenderBackground(tprop, data, metaData);

  // Render shadow if needed
  if (metaData.textProperty->GetShadow())
  {
    // Modify the line offsets with the shadow offset
    svtkVector2i shadowOffset;
    metaData.textProperty->GetShadowOffset(shadowOffset.GetData());
    std::vector<MetaData::LineMetrics> origMetrics = metaData.lineMetrics;
    metaData.lineMetrics.clear();
    for (std::vector<MetaData::LineMetrics>::const_iterator it = origMetrics.begin(),
                                                            itEnd = origMetrics.end();
         it < itEnd; ++it)
    {
      MetaData::LineMetrics line = *it;
      line.origin = line.origin + shadowOffset;
      metaData.lineMetrics.push_back(line);
    }

    // Set the color
    unsigned char origColor[3] = { metaData.rgba[0], metaData.rgba[1], metaData.rgba[2] };
    double shadowColor[3];
    metaData.textProperty->GetShadowColor(shadowColor);
    metaData.rgba[0] = static_cast<unsigned char>(shadowColor[0] * 255);
    metaData.rgba[1] = static_cast<unsigned char>(shadowColor[1] * 255);
    metaData.rgba[2] = static_cast<unsigned char>(shadowColor[2] * 255);

    if (!this->PopulateData(str, data, metaData))
    {
      svtkErrorMacro(<< "Error rendering shadow");
      return false;
    }

    // Restore color and line metrics
    metaData.lineMetrics = origMetrics;
    memcpy(metaData.rgba, origColor, 3 * sizeof(unsigned char));
  }

  // Mark the image data as modified, as it is possible that only
  // svtkImageData::Get*Pointer methods will be called, which do not update the
  // MTime.
  data->Modified();

  // Render image
  if (!this->PopulateData(str, data, metaData))
  {
    svtkErrorMacro(<< "Error rendering text.");
    return false;
  }

  // Draw a red dot at the anchor point:
  if (this->DebugTextures)
  {
    unsigned char* ptr = static_cast<unsigned char*>(data->GetScalarPointer(0, 0, 0));
    if (ptr)
    {
      ptr[0] = 255;
      ptr[1] = 0;
      ptr[2] = 0;
      ptr[3] = 255;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
template <typename StringType>
bool svtkFreeTypeTools::StringToPathInternal(
  svtkTextProperty* tprop, const StringType& str, int dpi, svtkPath* path)
{
  // Setup the metadata
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
  {
    svtkErrorMacro(<< "Could not prepare metadata.");
    return false;
  }

  // Layout the text, calculate bounding box
  if (!this->CalculateBoundingBox(str, metaData))
  {
    svtkErrorMacro(<< "Could not calculate bounding box.");
    return false;
  }

  // Create the path
  if (!this->PopulateData(str, path, metaData))
  {
    svtkErrorMacro(<< "Could not populate path.");
    return false;
  }

  return true;
}

namespace
{
const char* DEFAULT_HEIGHT_STRING = "_/7Agfy";
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::CalculateBoundingBox(const svtkUnicodeString& str, MetaData& metaData)
{
  return CalculateBoundingBox(str, metaData, svtkUnicodeString::from_utf8(DEFAULT_HEIGHT_STRING));
}

//----------------------------------------------------------------------------
bool svtkFreeTypeTools::CalculateBoundingBox(const svtkStdString& str, MetaData& metaData)
{
  return CalculateBoundingBox(str, metaData, svtkStdString(DEFAULT_HEIGHT_STRING));
}

namespace
{

template <typename T>
constexpr typename T::value_type newline()
{
  return static_cast<typename T::value_type>('\n');
}

}

//----------------------------------------------------------------------------
template <typename T>
bool svtkFreeTypeTools::CalculateBoundingBox(
  const T& str, MetaData& metaData, const T& defaultHeightString)
{
  // Calculate the metrics for each line. These will be used to calculate
  // a bounding box, but first we need to know the maximum line length to
  // get justification right.
  metaData.lineMetrics.clear();
  metaData.maxLineWidth = 0;

  // Go through the string, line by line, and build the metrics data.
  typename T::const_iterator beginLine = str.begin();
  typename T::const_iterator endLine = std::find(beginLine, str.end(), newline<T>());
  while (endLine != str.end())
  {
    metaData.lineMetrics.push_back(MetaData::LineMetrics());
    this->GetLineMetrics(beginLine, endLine, metaData, metaData.lineMetrics.back().width,
      &metaData.lineMetrics.back().xmin);
    metaData.maxLineWidth = std::max(metaData.maxLineWidth, metaData.lineMetrics.back().width);
    beginLine = endLine;
    ++beginLine;
    endLine = std::find(beginLine, str.end(), newline<T>());
  }
  // Last line...
  metaData.lineMetrics.push_back(MetaData::LineMetrics());
  this->GetLineMetrics(beginLine, endLine, metaData, metaData.lineMetrics.back().width,
    &metaData.lineMetrics.back().xmin);
  metaData.maxLineWidth = std::max(metaData.maxLineWidth, metaData.lineMetrics.back().width);

  size_t numLines = metaData.lineMetrics.size();
  T heightString;
  if (metaData.textProperty->GetUseTightBoundingBox() && numLines == 1)
  {
    // Calculate line height from actual characters. This works only for single line text
    // and may result in a height that does not include descent. It is used to get
    // a centered label.
    heightString = str;
  }
  else
  {
    // Calculate line height from a reference set of characters, since the global
    // face values are usually way too big.
    heightString = defaultHeightString;
  }
  int ascent = 0;
  int descent = 0;
  typename T::const_iterator it = heightString.begin();
  while (it != heightString.end())
  {
    FT_BitmapGlyph bitmapGlyph;
    FT_UInt glyphIndex;
    // Use the unrotated face to get correct metrics:
    FT_Bitmap* bitmap = this->GetBitmap(*it, &metaData.unrotatedScaler, glyphIndex, bitmapGlyph);
    if (bitmap)
    {
      ascent = std::max(bitmapGlyph->top, ascent);
      descent = std::min(-static_cast<int>((bitmap->rows - bitmapGlyph->top - 1)), descent);
    }
    ++it;
  }
  // Set line height. Descent is negative.
  metaData.height = ascent - descent + 1;

  // The unrotated height of the text
  int interLineSpacing = (metaData.textProperty->GetLineSpacing() - 1) * metaData.height;
  int fullHeight = numLines * metaData.height + (numLines - 1) * interLineSpacing +
    metaData.textProperty->GetLineOffset();

  // Will we be rendering a background?
  bool hasBackground =
    (static_cast<unsigned char>(metaData.textProperty->GetBackgroundOpacity() * 255) > 0);
  bool hasFrame = metaData.textProperty->GetFrame() && metaData.textProperty->GetFrameWidth() > 0;
  int padWidth = hasFrame ? 1 + metaData.textProperty->GetFrameWidth() : 2;

  int pad = (hasBackground || hasFrame) ? padWidth : 0; // pixels on each side.

  // sin, cos of orientation
  float angle = svtkMath::RadiansFromDegrees(metaData.textProperty->GetOrientation());
  float c = cos(angle);
  float s = sin(angle);

  // The width and height of the text + background/frame, as rotated vectors:
  metaData.dx = svtkVector2i(metaData.maxLineWidth + 2 * pad, 0);
  metaData.dy = svtkVector2i(0, fullHeight + 2 * pad);
  rotateVector2i(metaData.dx, s, c);
  rotateVector2i(metaData.dy, s, c);

  // Rotate the ascent/descent:
  metaData.ascent = svtkVector2i(0, ascent);
  metaData.descent = svtkVector2i(0, descent);
  rotateVector2i(metaData.ascent, s, c);
  rotateVector2i(metaData.descent, s, c);

  // The rotated padding on the text's vertical and horizontal axes:
  svtkVector2i hPad(pad, 0);
  svtkVector2i vPad(0, pad);
  svtkVector2i hOne(1, 0);
  svtkVector2i vOne(0, 1);
  rotateVector2i(hPad, s, c);
  rotateVector2i(vPad, s, c);
  rotateVector2i(hOne, s, c);
  rotateVector2i(vOne, s, c);

  // Calculate the bottom left corner of the data rect. Start at anchor point
  // (0, 0) and subtract out justification. Account for background/frame padding to
  // ensure that we're aligning to the text, not the background/frame.
  metaData.BL = svtkVector2i(0, 0);
  switch (metaData.textProperty->GetJustification())
  {
    case SVTK_TEXT_CENTERED:
      metaData.BL = metaData.BL - (metaData.dx * 0.5);
      break;
    case SVTK_TEXT_RIGHT:
      metaData.BL = metaData.BL - metaData.dx + hPad + hOne;
      break;
    case SVTK_TEXT_LEFT:
      metaData.BL = metaData.BL - hPad;
      break;
    default:
      svtkErrorMacro(<< "Bad horizontal alignment flag: "
                    << metaData.textProperty->GetJustification());
      break;
  }
  switch (metaData.textProperty->GetVerticalJustification())
  {
    case SVTK_TEXT_CENTERED:
      metaData.BL = metaData.BL - (metaData.dy * 0.5);
      break;
    case SVTK_TEXT_BOTTOM:
      metaData.BL = metaData.BL - vPad;
      break;
    case SVTK_TEXT_TOP:
      metaData.BL = metaData.BL - metaData.dy + vPad + vOne;
      break;
    default:
      svtkErrorMacro(<< "Bad vertical alignment flag: "
                    << metaData.textProperty->GetVerticalJustification());
      break;
  }

  // Compute the other corners of the data:
  metaData.TL = metaData.BL + metaData.dy - vOne;
  metaData.TR = metaData.TL + metaData.dx - hOne;
  metaData.BR = metaData.BL + metaData.dx - hOne;

  // First baseline offset from top-left corner.
  svtkVector2i penOffset(pad, -pad);
  // Account for line spacing to center the text vertically in the bbox:
  penOffset[1] -= ascent;
  penOffset[1] -= metaData.textProperty->GetLineOffset();
  rotateVector2i(penOffset, s, c);

  svtkVector2i pen = metaData.TL + penOffset;

  // Calculate bounding box of text:
  svtkTuple<int, 4> textBbox;
  textBbox[0] = textBbox[1] = pen[0];
  textBbox[2] = textBbox[3] = pen[1];

  // Calculate line offset:
  svtkVector2i lineFeed(0, -(metaData.height + interLineSpacing));
  rotateVector2i(lineFeed, s, c);

  // Compile the metrics data to determine the final bounding box. Set line
  // origins here, too.
  svtkVector2i origin;
  int justification = metaData.textProperty->GetJustification();
  for (size_t i = 0; i < metaData.lineMetrics.size(); ++i)
  {
    MetaData::LineMetrics& metrics = metaData.lineMetrics[i];

    // Apply justification
    origin = pen;
    if (justification != SVTK_TEXT_LEFT)
    {
      int xShift = metaData.maxLineWidth - metrics.width;
      if (justification == SVTK_TEXT_CENTERED)
      {
        xShift /= 2;
      }
      origin[0] += static_cast<int>(std::round(c * xShift));
      origin[1] += static_cast<int>(std::round(s * xShift));
    }

    // Set line origin
    metrics.origin = origin;

    // Merge bounding boxes
    textBbox[0] = std::min(textBbox[0], metrics.xmin + origin[0]);
    textBbox[1] = std::max(textBbox[1], metrics.xmax + origin[0]);
    textBbox[2] = std::min(textBbox[2], metrics.ymin + origin[1]);
    textBbox[3] = std::max(textBbox[3], metrics.ymax + origin[1]);

    // Update pen position
    pen = pen + lineFeed;
  }

  // Adjust for shadow
  if (metaData.textProperty->GetShadow())
  {
    int shadowOffset[2];
    metaData.textProperty->GetShadowOffset(shadowOffset);
    if (shadowOffset[0] < 0)
    {
      textBbox[0] += shadowOffset[0];
    }
    else
    {
      textBbox[1] += shadowOffset[0];
    }
    if (shadowOffset[1] < 0)
    {
      textBbox[2] += shadowOffset[1];
    }
    else
    {
      textBbox[3] += shadowOffset[1];
    }
  }

  // Compute the background/frame bounding box.
  svtkTuple<int, 4> bgBbox;
  bgBbox[0] =
    std::min(std::min(metaData.TL[0], metaData.TR[0]), std::min(metaData.BL[0], metaData.BR[0]));
  bgBbox[1] =
    std::max(std::max(metaData.TL[0], metaData.TR[0]), std::max(metaData.BL[0], metaData.BR[0]));
  bgBbox[2] =
    std::min(std::min(metaData.TL[1], metaData.TR[1]), std::min(metaData.BL[1], metaData.BR[1]));
  bgBbox[3] =
    std::max(std::max(metaData.TL[1], metaData.TR[1]), std::max(metaData.BL[1], metaData.BR[1]));

  // Calculate the final bounding box (should just be the bg, but just in
  // case...)
  metaData.bbox[0] = std::min(textBbox[0], bgBbox[0]);
  metaData.bbox[1] = std::max(textBbox[1], bgBbox[1]);
  metaData.bbox[2] = std::min(textBbox[2], bgBbox[2]);
  metaData.bbox[3] = std::max(textBbox[3], bgBbox[3]);

  return true;
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::PrepareImageData(svtkImageData* data, int textBbox[4])
{
  // Calculate the bbox's dimensions
  int textDims[2];
  textDims[0] = (textBbox[1] - textBbox[0] + 1);
  textDims[1] = (textBbox[3] - textBbox[2] + 1);

  // Calculate the size the image needs to be.
  int targetDims[3];
  targetDims[0] = textDims[0];
  targetDims[1] = textDims[1];
  targetDims[2] = 1;
  // Scale to the next highest power of 2 if required.
  if (this->ScaleToPowerTwo)
  {
    targetDims[0] = svtkMath::NearestPowerOfTwo(targetDims[0]);
    targetDims[1] = svtkMath::NearestPowerOfTwo(targetDims[1]);
  }

  // Calculate the target extent of the image.
  int targetExtent[6];
  targetExtent[0] = textBbox[0];
  targetExtent[1] = textBbox[0] + targetDims[0] - 1;
  targetExtent[2] = textBbox[2];
  targetExtent[3] = textBbox[2] + targetDims[1] - 1;
  targetExtent[4] = 0;
  targetExtent[5] = 0;

  // Get the actual image extents and increments
  int imageExtent[6];
  double imageSpacing[3];
  data->GetExtent(imageExtent);
  data->GetSpacing(imageSpacing);

  // Do we need to reallocate the image memory?
  if (data->GetScalarType() != SVTK_UNSIGNED_CHAR || data->GetNumberOfScalarComponents() != 4 ||
    imageExtent[0] != targetExtent[0] || imageExtent[1] != targetExtent[1] ||
    imageExtent[2] != targetExtent[2] || imageExtent[3] != targetExtent[3] ||
    imageExtent[4] != targetExtent[4] || imageExtent[5] != targetExtent[5] ||
    fabs(imageSpacing[0] - 1.0) > 1e-10 || fabs(imageSpacing[1] - 1.0) > 1e-10 ||
    fabs(imageSpacing[2] - 1.0) > 1e-10)
  {
    data->SetSpacing(1.0, 1.0, 1.0);
    data->SetExtent(targetExtent);
    data->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);
  }

  // Clear the image buffer
  memset(data->GetScalarPointer(), this->DebugTextures ? 64 : 0,
    (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));
}

// Helper functions for rasterizing the background/frame quad:
namespace RasterScanQuad
{

// Return true and set t1 (if 0 <= t1 <= 1) for the intersection of lines:
//
// P1(t1) = p1 + t1 * v1 and
// P2(t2) = p2 + t2 * v2.
//
// This method is specialized for the case of P2(t2) always being a horizontal
// line (v2 = {1, 0}) with p1 defined as {0, y}.
//
// If the lines do not intersect or t1 is outside of the specified range, return
// false.
inline bool getIntersectionParameter(const svtkVector2i& p1, const svtkVector2i& v1, int y, float& t1)
{
  // First check if the input vector is parallel to the scan line, returning
  // false if it is:
  if (v1[1] == 0)
  {
    return false;
  }

  // Given the lines:
  // P1(t1) = p1 + t1 * v1 (The polygon edge)
  // P2(t2) = p2 + t2 * v2 (The horizontal scan line)
  //
  // And defining the vector:
  // w = p1 - p2
  //
  // The value of t1 at the intersection of P1 and P2 is:
  // t1 = (v2[1] * w[0] - v2[0] * w[1]) / (v2[0] * v1[1] - v2[1] * v1[0])
  //
  // We know that p2 = {0, y} and v2 = {1, 0}, since we're scanning along the
  // x axis, so the above becomes:
  // t1 = (-w[1]) / (v1[1])
  //
  // Expanding the definition of w, w[1] --> (p1[1] - p2[1]) --> p1[1] - y,
  // resulting in the final:
  // t1 = -(p1[1] - y) / v1[1], or
  // t1 = (y - p1[1]) / v1[1]

  t1 = (y - p1[1]) / static_cast<float>(v1[1]);
  return t1 >= 0.f && t1 <= 1.f;
}

// Evaluate the line equation P(t) = p + t * v at the supplied t, and return
// the x value of the resulting point.
inline int evaluateLineXOnly(const svtkVector2i& p, const svtkVector2i& v, float t)
{
  return p.GetX() + static_cast<int>(std::round(v.GetX() * t));
}

// Given the corners of a rectangle (TL, TR, BL, BR), the vectors that
// separate them (dx = TR - TL = BR - BL, dy = TR - BR = TL - BL), and the
// y value to scan, return the minimum and maximum x values that the rectangle
// contains.
bool findScanRange(const svtkVector2i& TL, const svtkVector2i& TR, const svtkVector2i& BL,
  const svtkVector2i& BR, const svtkVector2i& dx, const svtkVector2i& dy, int y, int& min, int& max)
{
  // Initialize the min and max to a known invalid range using the bounds of the
  // rectangle:
  min = std::max(std::max(TL[0], TR[0]), std::max(BL[0], BR[0]));
  max = std::min(std::min(TL[0], TR[0]), std::min(BL[0], BR[0]));

  float lineParam;
  int numIntersections = 0;

  // Top
  if (getIntersectionParameter(TL, dx, y, lineParam))
  {
    int x = evaluateLineXOnly(TL, dx, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Bottom
  if (getIntersectionParameter(BL, dx, y, lineParam))
  {
    int x = evaluateLineXOnly(BL, dx, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Left
  if (getIntersectionParameter(BL, dy, y, lineParam))
  {
    int x = evaluateLineXOnly(BL, dy, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Right
  if (getIntersectionParameter(BR, dy, y, lineParam))
  {
    int x = evaluateLineXOnly(BR, dy, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }

  return numIntersections != 0;
}

// Clamp value to stay between the minimum and maximum extent for the
// specified dimension.
inline void clampToExtent(int extent[6], int dim, int& value)
{
  value = std::min(extent[2 * dim + 1], std::max(extent[2 * dim], value));
}

} // end namespace RasterScanQuad

//----------------------------------------------------------------------------
void svtkFreeTypeTools::RenderBackground(
  svtkTextProperty* tprop, svtkImageData* image, ImageMetaData& metaData)
{
  unsigned char* color;
  unsigned char backgroundColor[4] = { static_cast<unsigned char>(
                                         tprop->GetBackgroundColor()[0] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundColor()[1] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundColor()[2] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundOpacity() * 255) };
  unsigned char frameColor[4] = { static_cast<unsigned char>(tprop->GetFrameColor()[0] * 255),
    static_cast<unsigned char>(tprop->GetFrameColor()[1] * 255),
    static_cast<unsigned char>(tprop->GetFrameColor()[2] * 255),
    static_cast<unsigned char>(tprop->GetFrame() ? 255 : 0) };

  if (backgroundColor[3] == 0 && frameColor[3] == 0)
  {
    return;
  }

  const svtkVector2i& dx = metaData.dx;
  const svtkVector2i& dy = metaData.dy;
  const svtkVector2i& TL = metaData.TL;
  const svtkVector2i& TR = metaData.TR;
  const svtkVector2i& BL = metaData.BL;
  const svtkVector2i& BR = metaData.BR;

  // Find the minimum and maximum y values:
  int yMin = std::min(std::min(TL[1], TR[1]), std::min(BL[1], BR[1]));
  int yMax = std::max(std::max(TL[1], TR[1]), std::max(BL[1], BR[1]));

  // Clamp these to prevent out of bounds errors:
  int extent[6];
  image->GetExtent(extent);
  RasterScanQuad::clampToExtent(extent, 1, yMin);
  RasterScanQuad::clampToExtent(extent, 1, yMax);

  // Scan from yMin to yMax, finding the x values on that horizontal line that
  // are contained by the data rectangle, then paint them with the background
  // color.
  int frameWidth = tprop->GetFrameWidth();
  for (int y = yMin; y <= yMax; ++y)
  {
    int xMin, xMax;
    if (RasterScanQuad::findScanRange(TL, TR, BL, BR, dx, dy, y, xMin, xMax))
    {
      // Clamp to prevent out of bounds errors:
      RasterScanQuad::clampToExtent(extent, 0, xMin);
      RasterScanQuad::clampToExtent(extent, 0, xMax);

      // Get a pointer into the image data:
      unsigned char* dataPtr = static_cast<unsigned char*>(image->GetScalarPointer(xMin, y, 0));
      for (int x = xMin; x <= xMax; ++x)
      {
        color = (frameColor[3] != 0 &&
                  (y < (yMin + frameWidth) || y > (yMax - frameWidth) || x < (xMin + frameWidth) ||
                    x > (xMax - frameWidth)))
          ? frameColor
          : backgroundColor;
        *(dataPtr++) = color[0];
        *(dataPtr++) = color[1];
        *(dataPtr++) = color[2];
        *(dataPtr++) = color[3];
      }
    }
  }
}

//----------------------------------------------------------------------------
template <typename StringType, typename DataType>
bool svtkFreeTypeTools::PopulateData(const StringType& str, DataType data, MetaData& metaData)
{
  // Go through the string, line by line
  typename StringType::const_iterator beginLine = str.begin();
  typename StringType::const_iterator endLine =
    std::find(beginLine, str.end(), newline<StringType>());

  int lineIndex = 0;
  while (endLine != str.end())
  {
    if (!this->RenderLine(beginLine, endLine, lineIndex, data, metaData))
    {
      return false;
    }

    beginLine = endLine;
    ++beginLine;
    endLine = std::find(beginLine, str.end(), newline<StringType>());
    ++lineIndex;
  }

  // Render the last line:
  return this->RenderLine(beginLine, endLine, lineIndex, data, metaData);
}

//----------------------------------------------------------------------------
template <typename IteratorType, typename DataType>
bool svtkFreeTypeTools::RenderLine(
  IteratorType begin, IteratorType end, int lineIndex, DataType data, MetaData& metaData)
{
  int x = metaData.lineMetrics[lineIndex].origin.GetX();
  int y = metaData.lineMetrics[lineIndex].origin.GetY();

  // Render char by char
  FT_UInt previousGlyphIndex = 0; // for kerning
  for (; begin != end; ++begin)
  {
    this->RenderCharacter(*begin, x, y, previousGlyphIndex, data, metaData);
  }

  return true;
}

//----------------------------------------------------------------------------
template <typename CharType>
bool svtkFreeTypeTools::RenderCharacter(CharType character, int& x, int& y,
  FT_UInt& previousGlyphIndex, svtkImageData* image, MetaData& metaData)
{
  ImageMetaData* iMetaData = reinterpret_cast<ImageMetaData*>(&metaData);
  FT_BitmapGlyph bitmapGlyph = nullptr;
  FT_UInt glyphIndex;
  FT_Bitmap* bitmap = this->GetBitmap(character, &iMetaData->scaler, glyphIndex, bitmapGlyph);

  // Add the kerning
  if (iMetaData->faceHasKerning && previousGlyphIndex && glyphIndex)
  {
    FT_Vector kerningDelta;
    if (FT_Get_Kerning(
          iMetaData->face, previousGlyphIndex, glyphIndex, FT_KERNING_DEFAULT, &kerningDelta) == 0)
    {
      if (metaData.faceIsRotated) // PR#15301
      {
        FT_Vector_Transform(&kerningDelta, &metaData.rotation);
      }
      x += kerningDelta.x >> 6;
      y += kerningDelta.y >> 6;
    }
  }
  previousGlyphIndex = glyphIndex;

  if (!bitmap)
  {
    // TODO This should draw an empty rectangle.
    return false;
  }

  if (bitmap->width && bitmap->rows)
  {
    // Starting position given the bearings.
    svtkVector2i pen(x + bitmapGlyph->left, y + bitmapGlyph->top);

    // Render the current glyph into the image
    unsigned char* ptr = static_cast<unsigned char*>(image->GetScalarPointer(pen[0], pen[1], 0));
    if (ptr)
    {
      int dataPitch =
        (-iMetaData->imageDimensions[0] - bitmap->width) * iMetaData->imageIncrements[0];
      unsigned char* glyphPtrRow = bitmap->buffer;
      unsigned char* glyphPtr;
      const unsigned char* fgRGB = iMetaData->rgba;
      const float fgA = static_cast<float>(metaData.textProperty->GetOpacity());

      for (int j = 0; j < static_cast<int>(bitmap->rows); ++j)
      {
        glyphPtr = glyphPtrRow;

        for (int i = 0; i < static_cast<int>(bitmap->width); ++i)
        {
          if (*glyphPtr == 0) // Pixel is not drawn
          {
            ptr += 4;
          }
          else if (ptr[3] > 0) // Need to overblend
          {
            // This is a pixel we've drawn before since it has non-zero alpha.
            // We must therefore blend the colors.
            const float glyphA = *glyphPtr / 255.f;
            const float bgA = ptr[3] / 255.f;

            const float fg_blend = fgA * glyphA;
            const float bg_blend = bgA * (1.f - fg_blend);

            // src_a + dst_a ( 1 - src_a )
            const float a = 255.f * (fg_blend + bg_blend);
            const float invA = 1.f / (fg_blend + bg_blend);

            // (src_c * src_a + dst_c * dst_a * (1 - src_a)) / out_a
            const float r = (bg_blend * ptr[0] + fg_blend * fgRGB[0]) * invA;
            const float g = (bg_blend * ptr[1] + fg_blend * fgRGB[1]) * invA;
            const float b = (bg_blend * ptr[2] + fg_blend * fgRGB[2]) * invA;

            // Update the buffer:
            ptr[0] = static_cast<unsigned char>(r);
            ptr[1] = static_cast<unsigned char>(g);
            ptr[2] = static_cast<unsigned char>(b);
            ptr[3] = static_cast<unsigned char>(a);

            ptr += 4;
          }
          else // No existing color
          {
            *ptr = fgRGB[0];
            ++ptr;
            *ptr = fgRGB[1];
            ++ptr;
            *ptr = fgRGB[2];
            ++ptr;
            *ptr = static_cast<unsigned char>((*glyphPtr) * fgA);
            ++ptr;
          }
          ++glyphPtr;
        }
        glyphPtrRow += bitmap->pitch;
        ptr += dataPitch;
      }
    }
  }

  // Advance to next char
  x += (bitmapGlyph->root.advance.x + 0x8000) >> 16;
  y += (bitmapGlyph->root.advance.y + 0x8000) >> 16;
  return true;
}

//----------------------------------------------------------------------------
template <typename CharType>
bool svtkFreeTypeTools::RenderCharacter(CharType character, int& x, int& y,
  FT_UInt& previousGlyphIndex, svtkPath* path, MetaData& metaData)
{
  FT_UInt glyphIndex = 0;
  FT_OutlineGlyph outlineGlyph = nullptr;
  FT_Outline* outline = this->GetOutline(character, &metaData.scaler, glyphIndex, outlineGlyph);

  // Add the kerning
  if (metaData.faceHasKerning && previousGlyphIndex && glyphIndex)
  {
    FT_Vector kerningDelta;
    FT_Get_Kerning(
      metaData.face, previousGlyphIndex, glyphIndex, FT_KERNING_DEFAULT, &kerningDelta);
    if (metaData.faceIsRotated) // PR#15301
    {
      FT_Vector_Transform(&kerningDelta, &metaData.rotation);
    }
    x += kerningDelta.x >> 6;
    y += kerningDelta.y >> 6;
  }
  previousGlyphIndex = glyphIndex;

  if (!outline)
  {
    // TODO render an empty box.
    return false;
  }

  this->OutlineToPath(x, y, outline, path);

  // Advance to next char
  x += (outlineGlyph->root.advance.x + 0x8000) >> 16;
  y += (outlineGlyph->root.advance.y + 0x8000) >> 16;

  return true;
}

//----------------------------------------------------------------------------
void svtkFreeTypeTools::OutlineToPath(int x, int y, FT_Outline* outline, svtkPath* path)
{
  // The FT_CURVE defines don't really work in a switch...only the first two
  // bits are meaningful, and the rest appear to be garbage. We'll convert them
  // into values in the enum below:
  enum controlType
  {
    FIRST_POINT,
    ON_POINT,
    CUBIC_POINT,
    CONIC_POINT
  };

  if (outline->n_points > 0)
  {
    short point = 0;
    for (short contour = 0; contour < outline->n_contours; ++contour)
    {
      short contourEnd = outline->contours[contour];
      controlType lastTag = FIRST_POINT;
      double contourStartVec[2];
      contourStartVec[0] = contourStartVec[1] = 0.0;
      double lastVec[2];
      lastVec[0] = lastVec[1] = 0.0;
      for (; point <= contourEnd; ++point)
      {
        FT_Vector ftvec = outline->points[point];
        char fttag = outline->tags[point];
        controlType tag = FIRST_POINT;

        // Mask the tag and convert to our known-good control types:
        // (0x3 mask is because these values often have trailing garbage --
        // see note above controlType enum).
        switch (fttag & 0x3)
        {
          case (FT_CURVE_TAG_ON & 0x3): // 0b01
            tag = ON_POINT;
            break;
          case (FT_CURVE_TAG_CUBIC & 0x3): // 0b11
            tag = CUBIC_POINT;
            break;
          case (FT_CURVE_TAG_CONIC & 0x3): // 0b00
            tag = CONIC_POINT;
            break;
          default:
            svtkWarningMacro("Invalid control code returned from FreeType: "
              << static_cast<int>(fttag) << " (masked: " << static_cast<int>(fttag & 0x3));
            return;
        }

        double vec[2];
        vec[0] = ftvec.x / 64.0 + x;
        vec[1] = ftvec.y / 64.0 + y;

        // Handle the first point here, unless it is a CONIC point, in which
        // case the switches below handle it.
        if (lastTag == FIRST_POINT && tag != CONIC_POINT)
        {
          path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::MOVE_TO);
          lastTag = tag;
          lastVec[0] = vec[0];
          lastVec[1] = vec[1];
          contourStartVec[0] = vec[0];
          contourStartVec[1] = vec[1];
          continue;
        }

        switch (tag)
        {
          case ON_POINT:
            switch (lastTag)
            {
              case ON_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::LINE_TO);
                break;
              case CONIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CONIC_CURVE);
                break;
              case CUBIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CUBIC_CURVE);
                break;
              case FIRST_POINT:
              default:
                break;
            }
            break;
          case CONIC_POINT:
            switch (lastTag)
            {
              case ON_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CONIC_CURVE);
                break;
              case CONIC_POINT:
              {
                // Two conic points indicate a virtual "ON" point between
                // them. Insert both points.
                double virtualOn[2] = { (vec[0] + lastVec[0]) * 0.5, (vec[1] + lastVec[1]) * 0.5 };
                path->InsertNextPoint(virtualOn[0], virtualOn[1], 0.0, svtkPath::CONIC_CURVE);
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CONIC_CURVE);
              }
              break;
              case FIRST_POINT:
              {
                // The first point in the contour can be a conic control
                // point. Use the last point of the contour as the starting
                // point. If the last point is a conic point as well, start
                // on a virtual point between the two:
                FT_Vector lastContourFTVec = outline->points[contourEnd];
                double lastContourVec[2] = { lastContourFTVec.x / 64.0 + x,
                  lastContourFTVec.y / 64.0 + y };
                char lastContourFTTag = outline->tags[contourEnd];
                if (lastContourFTTag & FT_CURVE_TAG_CONIC)
                {
                  double virtualOn[2] = { (vec[0] + lastContourVec[0]) * 0.5,
                    (vec[1] + lastContourVec[1]) * 0.5 };
                  path->InsertNextPoint(virtualOn[0], virtualOn[1], 0.0, svtkPath::MOVE_TO);
                  path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CONIC_CURVE);
                }
                else
                {
                  path->InsertNextPoint(
                    lastContourVec[0], lastContourVec[1], 0.0, svtkPath::MOVE_TO);
                  path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CONIC_CURVE);
                }
              }
              break;
              case CUBIC_POINT:
              default:
                break;
            }
            break;
          case CUBIC_POINT:
            switch (lastTag)
            {
              case ON_POINT:
              case CUBIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, svtkPath::CUBIC_CURVE);
                break;
              case CONIC_POINT:
              case FIRST_POINT:
              default:
                break;
            }
            break;
          case FIRST_POINT:
          default:
            break;
        } // end switch

        lastTag = tag;
        lastVec[0] = vec[0];
        lastVec[1] = vec[1];
      } // end contour

      // The contours are always implicitly closed to the start point of the
      // contour:
      switch (lastTag)
      {
        case ON_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0, svtkPath::LINE_TO);
          break;
        case CUBIC_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0, svtkPath::CUBIC_CURVE);
          break;
        case CONIC_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0, svtkPath::CONIC_CURVE);
          break;
        case FIRST_POINT:
        default:
          break;
      } // end switch (lastTag)
    }   // end contour points iteration
  }     // end contour iteration
}

//----------------------------------------------------------------------------
// Similar to implementations in svtkFreeTypeUtilities and svtkTextMapper.
template <typename T>
int svtkFreeTypeTools::FitStringToBBox(
  const T& str, MetaData& metaData, int targetWidth, int targetHeight)
{
  if (str.empty() || targetWidth == 0 || targetHeight == 0 || metaData.textProperty == nullptr)
  {
    return 0;
  }

  // Use the current font size as a first guess
  int size[2];
  double fontSize = metaData.textProperty->GetFontSize();
  if (!this->CalculateBoundingBox(str, metaData))
  {
    return -1;
  }
  size[0] = metaData.bbox[1] - metaData.bbox[0];
  size[1] = metaData.bbox[3] - metaData.bbox[2];

  // Bad assumption but better than nothing -- assume the bbox grows linearly
  // with the font size:
  if (size[0] != 0 && size[1] != 0)
  {
    fontSize *= std::min(static_cast<double>(targetWidth) / static_cast<double>(size[0]),
      static_cast<double>(targetHeight) / static_cast<double>(size[1]));
    metaData.textProperty->SetFontSize(static_cast<int>(fontSize));
    metaData.scaler.height = fontSize * 64;          // 26.6 format points
    metaData.scaler.width = fontSize * 64;           // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64;  // 26.6 format points
    if (!this->CalculateBoundingBox(str, metaData))
    {
      return -1;
    }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
  }

  // Now just step up/down until the bbox matches the target.
  while (size[0] < targetWidth && size[1] < targetHeight && fontSize < 200.)
  {
    fontSize += 1.;
    metaData.textProperty->SetFontSize(fontSize);
    metaData.scaler.height = fontSize * 64;          // 26.6 format points
    metaData.scaler.width = fontSize * 64;           // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64;  // 26.6 format points
    if (!this->CalculateBoundingBox(str, metaData))
    {
      return -1;
    }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
  }

  while ((size[0] > targetWidth || size[1] > targetHeight) && fontSize > 1.)
  {
    fontSize -= 1.;
    metaData.textProperty->SetFontSize(fontSize);
    metaData.scaler.height = fontSize * 64;          // 26.6 format points
    metaData.scaler.width = fontSize * 64;           // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64;  // 26.6 format points
    if (!this->CalculateBoundingBox(str, metaData))
    {
      return -1;
    }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
  }

  return fontSize;
}

//----------------------------------------------------------------------------
inline bool svtkFreeTypeTools::GetFace(
  svtkTextProperty* prop, size_t& prop_cache_id, FT_Face& face, bool& face_has_kerning)
{
  this->MapTextPropertyToId(prop, &prop_cache_id);
  if (!this->GetFace(prop_cache_id, &face))
  {
    svtkErrorMacro(<< "Failed retrieving the face");
    return false;
  }
  face_has_kerning = (FT_HAS_KERNING(face) != 0);
  return true;
}

//----------------------------------------------------------------------------
inline FT_Bitmap* svtkFreeTypeTools::GetBitmap(FT_UInt32 c, size_t prop_cache_id, int prop_font_size,
  FT_UInt& gindex, FT_BitmapGlyph& bitmap_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(prop_cache_id, c, &gindex))
  {
    return nullptr;
  }
  FT_Glyph glyph;
  // Get the glyph as a bitmap
  if (!this->GetGlyph(
        prop_cache_id, prop_font_size, gindex, &glyph, svtkFreeTypeTools::GLYPH_REQUEST_BITMAP) ||
    glyph->format != ft_glyph_format_bitmap)
  {
    return nullptr;
  }

  bitmap_glyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
  FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

  if (bitmap->pixel_mode != ft_pixel_mode_grays)
  {
    return nullptr;
  }

  return bitmap;
}

//----------------------------------------------------------------------------
FT_Bitmap* svtkFreeTypeTools::GetBitmap(
  FT_UInt32 c, FTC_Scaler scaler, FT_UInt& gindex, FT_BitmapGlyph& bitmap_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(reinterpret_cast<size_t>(scaler->face_id), c, &gindex))
  {
    return nullptr;
  }

  // Get the glyph as a bitmap
  FT_Glyph glyph;
  if (!this->GetGlyph(scaler, gindex, &glyph, svtkFreeTypeTools::GLYPH_REQUEST_BITMAP) ||
    glyph->format != ft_glyph_format_bitmap)
  {
    return nullptr;
  }

  bitmap_glyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
  FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

  if (bitmap->pixel_mode != ft_pixel_mode_grays)
  {
    return nullptr;
  }

  return bitmap;
}

//----------------------------------------------------------------------------
inline FT_Outline* svtkFreeTypeTools::GetOutline(FT_UInt32 c, size_t prop_cache_id,
  int prop_font_size, FT_UInt& gindex, FT_OutlineGlyph& outline_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(prop_cache_id, c, &gindex))
  {
    return nullptr;
  }
  FT_Glyph glyph;
  // Get the glyph as a outline
  if (!this->GetGlyph(
        prop_cache_id, prop_font_size, gindex, &glyph, svtkFreeTypeTools::GLYPH_REQUEST_OUTLINE) ||
    glyph->format != ft_glyph_format_outline)
  {
    return nullptr;
  }

  outline_glyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
  FT_Outline* outline = &outline_glyph->outline;

  return outline;
}

//----------------------------------------------------------------------------
FT_Outline* svtkFreeTypeTools::GetOutline(
  FT_UInt32 c, FTC_Scaler scaler, FT_UInt& gindex, FT_OutlineGlyph& outline_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(reinterpret_cast<size_t>(scaler->face_id), c, &gindex))
  {
    return nullptr;
  }

  // Get the glyph as a outline
  FT_Glyph glyph;
  if (!this->GetGlyph(scaler, gindex, &glyph, svtkFreeTypeTools::GLYPH_REQUEST_OUTLINE) ||
    glyph->format != ft_glyph_format_outline)
  {
    return nullptr;
  }

  outline_glyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
  FT_Outline* outline = &outline_glyph->outline;

  return outline;
}

//----------------------------------------------------------------------------
template <typename T>
void svtkFreeTypeTools::GetLineMetrics(T begin, T end, MetaData& metaData, int& width, int bbox[4])
{
  FT_BitmapGlyph bitmapGlyph = nullptr;
  FT_UInt gindex = 0;
  FT_UInt gindexLast = 0;
  FT_Vector delta;
  width = 0;
  int pen[2] = { 0, 0 };
  bbox[0] = bbox[1] = pen[0];
  bbox[2] = bbox[3] = pen[1];

  for (; begin != end; ++begin)
  {
    // Get the bitmap and glyph index:
    FT_Bitmap* bitmap = this->GetBitmap(*begin, &metaData.scaler, gindex, bitmapGlyph);

    // Adjust the pen location for kerning
    if (metaData.faceHasKerning && gindexLast && gindex)
    {
      if (FT_Get_Kerning(metaData.face, gindexLast, gindex, FT_KERNING_DEFAULT, &delta) == 0)
      {
        // Kerning is not rotated with the face, no need to rotate/adjust for
        // width:
        width += delta.x >> 6;
        // But we do need to rotate for pen location (see PR#15301)
        if (metaData.faceIsRotated)
        {
          FT_Vector_Transform(&delta, &metaData.rotation);
        }
        pen[0] += delta.x >> 6;
        pen[1] += delta.y >> 6;
      }
    }
    gindexLast = gindex;

    // Use the dimensions of the bitmap glyph to get a tight bounding box.
    if (bitmap)
    {
      bbox[0] = std::min(bbox[0], pen[0] + bitmapGlyph->left);
      bbox[1] = std::max(bbox[1], pen[0] + bitmapGlyph->left + static_cast<int>(bitmap->width) - 1);
      bbox[2] = std::min(bbox[2], pen[1] + bitmapGlyph->top + 1 - static_cast<int>(bitmap->rows));
      bbox[3] = std::max(bbox[3], pen[1] + bitmapGlyph->top);
    }
    else
    {
      // FIXME: do something more elegant here.
      // We should render an empty rectangle to adhere to the specs...
      svtkDebugMacro(<< "Unrecognized character: " << *begin);
      continue;
    }

    // Update advance.
    delta = bitmapGlyph->root.advance;
    pen[0] += (delta.x + 0x8000) >> 16;
    pen[1] += (delta.y + 0x8000) >> 16;

    if (metaData.faceIsRotated)
    {
      FT_Vector_Transform(&delta, &metaData.inverseRotation);
    }
    width += (delta.x + 0x8000) >> 16;
  }
}
