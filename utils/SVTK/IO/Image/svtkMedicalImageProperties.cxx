/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMedicalImageProperties.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMedicalImageProperties.h"
#include "svtkObjectFactory.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <cassert>
#include <cctype> // for isdigit
#include <ctime>  // for strftime

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkMedicalImageProperties);

static const char* svtkMedicalImagePropertiesOrientationString[] = { "AXIAL", "CORONAL", "SAGITTAL",
  nullptr };

//----------------------------------------------------------------------------
class svtkMedicalImagePropertiesInternals
{
public:
  class WindowLevelPreset
  {
  public:
    double Window;
    double Level;
    std::string Comment;
  };

  class UserDefinedValue
  {
  public:
    UserDefinedValue(const char* name = nullptr, const char* value = nullptr)
      : Name(name ? name : "")
      , Value(value ? value : "")
    {
    }
    std::string Name;
    std::string Value;
    // order for the std::set
    bool operator<(const UserDefinedValue& udv) const { return Name < udv.Name; }
  };
  typedef std::set<UserDefinedValue> UserDefinedValues;
  UserDefinedValues UserDefinedValuePool;
  void AddUserDefinedValue(const char* name, const char* value)
  {
    if (name && *name && value && *value)
    {
      UserDefinedValuePool.insert(UserDefinedValues::value_type(name, value));
    }
    // else raise a warning ?
  }
  const char* GetUserDefinedValue(const char* name) const
  {
    if (name && *name)
    {
      UserDefinedValue key(name);
      UserDefinedValues::const_iterator it = UserDefinedValuePool.find(key);
      if (it != UserDefinedValuePool.end())
      {
        assert(strcmp(it->Name.c_str(), name) == 0);
        return it->Value.c_str();
      }
    }
    return nullptr;
  }
  unsigned int GetNumberOfUserDefinedValues() const
  {
    return static_cast<unsigned int>(UserDefinedValuePool.size());
  }
  const char* GetUserDefinedNameByIndex(unsigned int idx)
  {
    if (idx < UserDefinedValuePool.size())
    {
      UserDefinedValues::const_iterator it = UserDefinedValuePool.begin();
      while (idx)
      {
        ++it;
        idx--;
      }
      return it->Name.c_str();
    }
    return nullptr;
  }
  const char* GetUserDefinedValueByIndex(unsigned int idx)
  {
    if (idx < UserDefinedValuePool.size())
    {
      UserDefinedValues::const_iterator it = UserDefinedValuePool.begin();
      while (idx)
      {
        ++it;
        idx--;
      }
      return it->Value.c_str();
    }
    return nullptr;
  }
  void RemoveAllUserDefinedValues() { UserDefinedValuePool.clear(); }

  typedef std::vector<WindowLevelPreset> WindowLevelPresetPoolType;
  typedef std::vector<WindowLevelPreset>::iterator WindowLevelPresetPoolIterator;

  WindowLevelPresetPoolType WindowLevelPresetPool;

  // It is also useful to have a mapping from DICOM UID to slice id, for application like VolView
  typedef std::map<unsigned int, std::string> SliceUIDType;
  typedef std::vector<SliceUIDType> VolumeSliceUIDType;
  VolumeSliceUIDType UID;
  void SetNumberOfVolumes(unsigned int n)
  {
    UID.resize(n);
    Orientation.resize(n);
  }
  void SetUID(unsigned int vol, unsigned int slice, const char* uid)
  {
    SetNumberOfVolumes(vol + 1);
    UID[vol][slice] = uid;
  }
  const char* GetUID(unsigned int vol, unsigned int slice)
  {
    assert(vol < UID.size());
    assert(UID[vol].find(slice) != UID[vol].end());
    // if( UID[vol].find(slice) == UID[vol].end() )
    //  {
    //  this->Print( cerr, svtkIndent() );
    //  }
    return UID[vol].find(slice)->second.c_str();
  }
  // Extensive lookup
  int FindSlice(int& vol, const char* uid)
  {
    vol = -1;
    for (unsigned int v = 0; v < UID.size(); ++v)
    {
      SliceUIDType::const_iterator cit = UID[v].begin();
      while (cit != UID[v].end())
      {
        if (cit->second == uid)
        {
          vol = v;
          return static_cast<int>(cit->first);
        }
        ++cit;
      }
    }
    return -1; // volume not found.
  }
  int GetSlice(unsigned int vol, const char* uid)
  {
    assert(vol < UID.size());
    SliceUIDType::const_iterator cit = UID[vol].begin();
    while (cit != UID[vol].end())
    {
      if (cit->second == uid)
      {
        return static_cast<int>(cit->first);
      }
      ++cit;
    }
    return -1; // uid not found.
  }
  void Print(ostream& os, svtkIndent indent)
  {
    os << indent << "WindowLevel: \n";
    for (WindowLevelPresetPoolIterator it = WindowLevelPresetPool.begin();
         it != WindowLevelPresetPool.end(); ++it)
    {
      const WindowLevelPreset& wlp = *it;
      os << indent.GetNextIndent() << "Window: " << wlp.Window << "\n";
      os << indent.GetNextIndent() << "Level: " << wlp.Level << "\n";
      os << indent.GetNextIndent() << "Comment: " << wlp.Comment << "\n";
    }
    os << indent << "UID(s):\n";
    for (VolumeSliceUIDType::const_iterator it = UID.begin(); it != UID.end(); ++it)
    {
      for (SliceUIDType::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
      {
        os << indent.GetNextIndent() << it2->first << "  " << it2->second << "\n";
      }
    }
    os << indent << "Orientation(s):\n";
    for (std::vector<unsigned int>::const_iterator it = Orientation.begin();
         it != Orientation.end(); ++it)
    {
      os << indent.GetNextIndent() << svtkMedicalImageProperties::GetStringFromOrientationType(*it)
         << "\n";
    }
    os << indent << "User Defined Values: (" << UserDefinedValuePool.size() << ")\n";
    UserDefinedValues::const_iterator it2 = UserDefinedValuePool.begin();
    for (; it2 != UserDefinedValuePool.end(); ++it2)
    {
      os << indent.GetNextIndent() << it2->Name << " -> " << it2->Value << "\n";
    }
  }
  std::vector<unsigned int> Orientation;
  void SetOrientation(unsigned int vol, unsigned int ori)
  {
    // see SetNumberOfVolumes for allocation
    assert(ori <= svtkMedicalImageProperties::SAGITTAL);
    Orientation[vol] = ori;
  }
  unsigned int GetOrientation(unsigned int vol)
  {
    assert(vol < Orientation.size());
    const unsigned int& val = Orientation[vol];
    assert(val <= svtkMedicalImageProperties::SAGITTAL);
    return val;
  }
  void DeepCopy(svtkMedicalImagePropertiesInternals* p)
  {
    WindowLevelPresetPool = p->WindowLevelPresetPool;
    UserDefinedValuePool = p->UserDefinedValuePool;
    UID = p->UID;
    Orientation = p->Orientation;
  }
};

//----------------------------------------------------------------------------
svtkMedicalImageProperties::svtkMedicalImageProperties()
{
  this->Internals = new svtkMedicalImagePropertiesInternals;

  this->StudyDate = nullptr;
  this->AcquisitionDate = nullptr;
  this->StudyTime = nullptr;
  this->AcquisitionTime = nullptr;
  this->ConvolutionKernel = nullptr;
  this->EchoTime = nullptr;
  this->EchoTrainLength = nullptr;
  this->Exposure = nullptr;
  this->ExposureTime = nullptr;
  this->GantryTilt = nullptr;
  this->ImageDate = nullptr;
  this->ImageNumber = nullptr;
  this->ImageTime = nullptr;
  this->InstitutionName = nullptr;
  this->KVP = nullptr;
  this->ManufacturerModelName = nullptr;
  this->Manufacturer = nullptr;
  this->Modality = nullptr;
  this->PatientAge = nullptr;
  this->PatientBirthDate = nullptr;
  this->PatientID = nullptr;
  this->PatientName = nullptr;
  this->PatientSex = nullptr;
  this->RepetitionTime = nullptr;
  this->SeriesDescription = nullptr;
  this->SeriesNumber = nullptr;
  this->SliceThickness = nullptr;
  this->StationName = nullptr;
  this->StudyDescription = nullptr;
  this->StudyID = nullptr;
  this->XRayTubeCurrent = nullptr;

  this->DirectionCosine[0] = 1;
  this->DirectionCosine[1] = 0;
  this->DirectionCosine[2] = 0;
  this->DirectionCosine[3] = 0;
  this->DirectionCosine[4] = 1;
  this->DirectionCosine[5] = 0;
}

//----------------------------------------------------------------------------
svtkMedicalImageProperties::~svtkMedicalImageProperties()
{
  this->Clear();

  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::AddUserDefinedValue(const char* name, const char* value)
{
  this->Internals->AddUserDefinedValue(name, value);
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetUserDefinedValue(const char* name)
{
  return this->Internals->GetUserDefinedValue(name);
}

//----------------------------------------------------------------------------
unsigned int svtkMedicalImageProperties::GetNumberOfUserDefinedValues()
{
  return this->Internals->GetNumberOfUserDefinedValues();
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetUserDefinedValueByIndex(unsigned int idx)
{
  return this->Internals->GetUserDefinedValueByIndex(idx);
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetUserDefinedNameByIndex(unsigned int idx)
{
  return this->Internals->GetUserDefinedNameByIndex(idx);
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::RemoveAllUserDefinedValues()
{
  this->Internals->RemoveAllUserDefinedValues();
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::Clear()
{
  this->SetStudyDate(nullptr);
  this->SetAcquisitionDate(nullptr);
  this->SetStudyTime(nullptr);
  this->SetAcquisitionTime(nullptr);
  this->SetConvolutionKernel(nullptr);
  this->SetEchoTime(nullptr);
  this->SetEchoTrainLength(nullptr);
  this->SetExposure(nullptr);
  this->SetExposureTime(nullptr);
  this->SetGantryTilt(nullptr);
  this->SetImageDate(nullptr);
  this->SetImageNumber(nullptr);
  this->SetImageTime(nullptr);
  this->SetInstitutionName(nullptr);
  this->SetKVP(nullptr);
  this->SetManufacturerModelName(nullptr);
  this->SetManufacturer(nullptr);
  this->SetModality(nullptr);
  this->SetPatientAge(nullptr);
  this->SetPatientBirthDate(nullptr);
  this->SetPatientID(nullptr);
  this->SetPatientName(nullptr);
  this->SetPatientSex(nullptr);
  this->SetRepetitionTime(nullptr);
  this->SetSeriesDescription(nullptr);
  this->SetSeriesNumber(nullptr);
  this->SetSliceThickness(nullptr);
  this->SetStationName(nullptr);
  this->SetStudyDescription(nullptr);
  this->SetStudyID(nullptr);
  this->SetXRayTubeCurrent(nullptr);

  this->RemoveAllWindowLevelPresets();
  this->RemoveAllUserDefinedValues();

  this->Internals->Orientation.clear();
  this->Internals->UID.clear();
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::DeepCopy(svtkMedicalImageProperties* p)
{
  if (p == nullptr)
  {
    return;
  }

  this->Clear();

  this->SetStudyDate(p->GetStudyDate());
  this->SetAcquisitionDate(p->GetAcquisitionDate());
  this->SetStudyTime(p->GetStudyTime());
  this->SetAcquisitionTime(p->GetAcquisitionTime());
  this->SetConvolutionKernel(p->GetConvolutionKernel());
  this->SetEchoTime(p->GetEchoTime());
  this->SetEchoTrainLength(p->GetEchoTrainLength());
  this->SetExposure(p->GetExposure());
  this->SetExposureTime(p->GetExposureTime());
  this->SetGantryTilt(p->GetGantryTilt());
  this->SetImageDate(p->GetImageDate());
  this->SetImageNumber(p->GetImageNumber());
  this->SetImageTime(p->GetImageTime());
  this->SetInstitutionName(p->GetInstitutionName());
  this->SetKVP(p->GetKVP());
  this->SetManufacturerModelName(p->GetManufacturerModelName());
  this->SetManufacturer(p->GetManufacturer());
  this->SetModality(p->GetModality());
  this->SetPatientAge(p->GetPatientAge());
  this->SetPatientBirthDate(p->GetPatientBirthDate());
  this->SetPatientID(p->GetPatientID());
  this->SetPatientName(p->GetPatientName());
  this->SetPatientSex(p->GetPatientSex());
  this->SetRepetitionTime(p->GetRepetitionTime());
  this->SetSeriesDescription(p->GetSeriesDescription());
  this->SetSeriesNumber(p->GetSeriesNumber());
  this->SetSliceThickness(p->GetSliceThickness());
  this->SetStationName(p->GetStationName());
  this->SetStudyDescription(p->GetStudyDescription());
  this->SetStudyID(p->GetStudyID());
  this->SetXRayTubeCurrent(p->GetXRayTubeCurrent());
  this->SetDirectionCosine(p->GetDirectionCosine());

  this->Internals->DeepCopy(p->Internals);
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::AddWindowLevelPreset(double w, double l)
{
  if (!this->Internals || this->HasWindowLevelPreset(w, l))
  {
    return -1;
  }

  svtkMedicalImagePropertiesInternals::WindowLevelPreset preset;
  preset.Window = w;
  preset.Level = l;
  this->Internals->WindowLevelPresetPool.push_back(preset);
  return static_cast<int>(this->Internals->WindowLevelPresetPool.size() - 1);
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetWindowLevelPresetIndex(double w, double l)
{
  if (this->Internals)
  {
    svtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator it =
      this->Internals->WindowLevelPresetPool.begin();
    svtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator end =
      this->Internals->WindowLevelPresetPool.end();
    int index = 0;
    for (; it != end; ++it, ++index)
    {
      if ((*it).Window == w && (*it).Level == l)
      {
        return index;
      }
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::HasWindowLevelPreset(double w, double l)
{
  return this->GetWindowLevelPresetIndex(w, l) >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::RemoveWindowLevelPreset(double w, double l)
{
  if (this->Internals)
  {
    svtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator it =
      this->Internals->WindowLevelPresetPool.begin();
    svtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator end =
      this->Internals->WindowLevelPresetPool.end();
    for (; it != end; ++it)
    {
      if ((*it).Window == w && (*it).Level == l)
      {
        this->Internals->WindowLevelPresetPool.erase(it);
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::RemoveAllWindowLevelPresets()
{
  if (this->Internals)
  {
    this->Internals->WindowLevelPresetPool.clear();
  }
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetNumberOfWindowLevelPresets()
{
  return this->Internals ? static_cast<int>(this->Internals->WindowLevelPresetPool.size()) : 0;
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetNthWindowLevelPreset(int idx, double* w, double* l)
{
  if (this->Internals && idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
  {
    *w = this->Internals->WindowLevelPresetPool[idx].Window;
    *l = this->Internals->WindowLevelPresetPool[idx].Level;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
double* svtkMedicalImageProperties::GetNthWindowLevelPreset(int idx)

{
  static double wl[2];
  if (this->GetNthWindowLevelPreset(idx, wl, wl + 1))
  {
    return wl;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetNthWindowLevelPresetComment(int idx)
{
  if (this->Internals && idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
  {
    return this->Internals->WindowLevelPresetPool[idx].Comment.c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::SetNthWindowLevelPresetComment(int idx, const char* comment)
{
  if (this->Internals && idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
  {
    this->Internals->WindowLevelPresetPool[idx].Comment = (comment ? comment : "");
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetInstanceUIDFromSliceID(int volumeidx, int sliceid)
{
  return this->Internals->GetUID(volumeidx, sliceid);
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetSliceIDFromInstanceUID(int& volumeidx, const char* uid)
{
  if (volumeidx == -1)
  {
    return this->Internals->FindSlice(volumeidx, uid);
  }
  else
  {
    return this->Internals->GetSlice(volumeidx, uid);
  }
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::SetInstanceUIDFromSliceID(
  int volumeidx, int sliceid, const char* uid)
{
  this->Internals->SetUID(volumeidx, sliceid, uid);
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::SetOrientationType(int volumeidx, int orientation)
{
  this->Internals->SetOrientation(volumeidx, orientation);
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetOrientationType(int volumeidx)
{
  return this->Internals->GetOrientation(volumeidx);
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageProperties::GetStringFromOrientationType(unsigned int type)
{
  static unsigned int numtypes = 0;
  // find length of table
  if (!numtypes)
  {
    while (svtkMedicalImagePropertiesOrientationString[numtypes] != nullptr)
    {
      numtypes++;
    }
  }

  if (type < numtypes)
  {
    return svtkMedicalImagePropertiesOrientationString[type];
  }

  return nullptr;
}

//----------------------------------------------------------------------------
double svtkMedicalImageProperties::GetSliceThicknessAsDouble()
{
  if (this->SliceThickness)
  {
    return atof(this->SliceThickness);
  }
  return 0;
}

//----------------------------------------------------------------------------
double svtkMedicalImageProperties::GetGantryTiltAsDouble()
{
  if (this->GantryTilt)
  {
    return atof(this->GantryTilt);
  }
  return 0;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetAgeAsFields(
  const char* age, int& year, int& month, int& week, int& day)
{
  year = month = week = day = -1;
  if (!age)
  {
    return 0;
  }

  size_t len = strlen(age);
  if (len == 4)
  {
    // DICOM V3
    unsigned int val;
    char type;
    if (!isdigit(age[0]) || !isdigit(age[1]) || !isdigit(age[2]))
    {
      return 0;
    }
    if (sscanf(age, "%3u%c", &val, &type) != 2)
    {
      return 0;
    }
    switch (type)
    {
      case 'Y':
        year = static_cast<int>(val);
        break;
      case 'M':
        month = static_cast<int>(val);
        break;
      case 'W':
        week = static_cast<int>(val);
        break;
      case 'D':
        day = static_cast<int>(val);
        break;
      default:
        return 0;
    }
  }
  else
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientAgeYear()
{
  const char* age = this->GetPatientAge();
  int year, month, week, day;
  svtkMedicalImageProperties::GetAgeAsFields(age, year, month, week, day);
  return year;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientAgeMonth()
{
  const char* age = this->GetPatientAge();
  int year, month, week, day;
  svtkMedicalImageProperties::GetAgeAsFields(age, year, month, week, day);
  return month;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientAgeWeek()
{
  const char* age = this->GetPatientAge();
  int year, month, week, day;
  svtkMedicalImageProperties::GetAgeAsFields(age, year, month, week, day);
  return week;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientAgeDay()
{
  const char* age = this->GetPatientAge();
  int year, month, week, day;
  svtkMedicalImageProperties::GetAgeAsFields(age, year, month, week, day);
  return day;
}

//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetTimeAsFields(
  const char* time, int& hour, int& minute, int& second /* , long &milliseconds */)
{
  if (!time)
  {
    return 0;
  }

  size_t len = strlen(time);
  if (len == 6)
  {
    // DICOM V3
    if (sscanf(time, "%02d%02d%02d", &hour, &minute, &second) != 3)
    {
      return 0;
    }
  }
  else if (len == 8)
  {
    // Some *very* old ACR-NEMA
    if (sscanf(time, "%02d.%02d.%02d", &hour, &minute, &second) != 3)
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }

  return 1;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetDateAsFields(const char* date, int& year, int& month, int& day)
{
  if (!date)
  {
    return 0;
  }

  size_t len = strlen(date);
  if (len == 8)
  {
    // DICOM V3
    if (sscanf(date, "%04d%02d%02d", &year, &month, &day) != 3)
    {
      return 0;
    }
  }
  else if (len == 10)
  {
    // Some *very* old ACR-NEMA
    if (sscanf(date, "%04d.%02d.%02d", &year, &month, &day) != 3)
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Some  buggy versions of gcc complain about the use of %c: warning: `%c'
// yields only last 2 digits of year in some locales.  Of course  program-
// mers  are  encouraged  to  use %c, it gives the preferred date and time
// representation. One meets all kinds of strange obfuscations to  circum-
// vent this gcc problem. A relatively clean one is to add an intermediate
// function. This is described as bug #3190 in gcc bugzilla:
// [-Wformat-y2k doesn't belong to -Wall - it's hard to avoid]
inline size_t my_strftime(char* s, size_t max, const char* fmt, const struct tm* tm)
{
  return strftime(s, max, fmt, tm);
}
// Helper function to convert a DICOM iso date format into a locale one
// locale buffer should be typically char locale[200]
int svtkMedicalImageProperties::GetDateAsLocale(const char* iso, char* locale)
{
  int year, month, day;
  if (svtkMedicalImageProperties::GetDateAsFields(iso, year, month, day))
  {
    if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31)
    {
      *locale = '\0';
    }
    else
    {
      struct tm date;
      memset(&date, 0, sizeof(date));
      date.tm_mday = day;
      // month are expressed in the [0-11] range:
      date.tm_mon = month - 1;
      // structure is date starting at 1900
      date.tm_year = year - 1900;
      my_strftime(locale, 200, "%x", &date);
    }
    return 1;
  }
  return 0;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientBirthDateYear()
{
  const char* date = this->GetPatientBirthDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientBirthDateMonth()
{
  const char* date = this->GetPatientBirthDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetPatientBirthDateDay()
{
  const char* date = this->GetPatientBirthDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetAcquisitionDateYear()
{
  const char* date = this->GetAcquisitionDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetAcquisitionDateMonth()
{
  const char* date = this->GetAcquisitionDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetAcquisitionDateDay()
{
  const char* date = this->GetAcquisitionDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetImageDateYear()
{
  const char* date = this->GetImageDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetImageDateMonth()
{
  const char* date = this->GetImageDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int svtkMedicalImageProperties::GetImageDateDay()
{
  const char* date = this->GetImageDate();
  int year = 0, month = 0, day = 0;
  svtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}

//----------------------------------------------------------------------------
void svtkMedicalImageProperties::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PatientName: ";
  if (this->PatientName)
  {
    os << this->PatientName;
  }

  os << "\n" << indent << "PatientID: ";
  if (this->PatientID)
  {
    os << this->PatientID;
  }

  os << "\n" << indent << "PatientAge: ";
  if (this->PatientAge)
  {
    os << this->PatientAge;
  }

  os << "\n" << indent << "PatientSex: ";
  if (this->PatientSex)
  {
    os << this->PatientSex;
  }

  os << "\n" << indent << "PatientBirthDate: ";
  if (this->PatientBirthDate)
  {
    os << this->PatientBirthDate;
  }

  os << "\n" << indent << "ImageDate: ";
  if (this->ImageDate)
  {
    os << this->ImageDate;
  }

  os << "\n" << indent << "ImageTime: ";
  if (this->ImageTime)
  {
    os << this->ImageTime;
  }

  os << "\n" << indent << "ImageNumber: ";
  if (this->ImageNumber)
  {
    os << this->ImageNumber;
  }

  os << "\n" << indent << "StudyDate: ";
  if (this->StudyDate)
  {
    os << this->StudyDate;
  }

  os << "\n" << indent << "AcquisitionDate: ";
  if (this->AcquisitionDate)
  {
    os << this->AcquisitionDate;
  }

  os << "\n" << indent << "StudyTime: ";
  if (this->StudyTime)
  {
    os << this->StudyTime;
  }

  os << "\n" << indent << "AcquisitionTime: ";
  if (this->AcquisitionTime)
  {
    os << this->AcquisitionTime;
  }

  os << "\n" << indent << "SeriesNumber: ";
  if (this->SeriesNumber)
  {
    os << this->SeriesNumber;
  }

  os << "\n" << indent << "SeriesDescription: ";
  if (this->SeriesDescription)
  {
    os << this->SeriesDescription;
  }

  os << "\n" << indent << "StudyDescription: ";
  if (this->StudyDescription)
  {
    os << this->StudyDescription;
  }

  os << "\n" << indent << "StudyID: ";
  if (this->StudyID)
  {
    os << this->StudyID;
  }

  os << "\n" << indent << "Modality: ";
  if (this->Modality)
  {
    os << this->Modality;
  }

  os << "\n" << indent << "ManufacturerModelName: ";
  if (this->ManufacturerModelName)
  {
    os << this->ManufacturerModelName;
  }

  os << "\n" << indent << "Manufacturer: ";
  if (this->Manufacturer)
  {
    os << this->Manufacturer;
  }

  os << "\n" << indent << "StationName: ";
  if (this->StationName)
  {
    os << this->StationName;
  }

  os << "\n" << indent << "InstitutionName: ";
  if (this->InstitutionName)
  {
    os << this->InstitutionName;
  }

  os << "\n" << indent << "ConvolutionKernel: ";
  if (this->ConvolutionKernel)
  {
    os << this->ConvolutionKernel;
  }

  os << "\n" << indent << "SliceThickness: ";
  if (this->SliceThickness)
  {
    os << this->SliceThickness;
  }

  os << "\n" << indent << "KVP: ";
  if (this->KVP)
  {
    os << this->KVP;
  }

  os << "\n" << indent << "GantryTilt: ";
  if (this->GantryTilt)
  {
    os << this->GantryTilt;
  }

  os << "\n" << indent << "EchoTime: ";
  if (this->EchoTime)
  {
    os << this->EchoTime;
  }

  os << "\n" << indent << "EchoTrainLength: ";
  if (this->EchoTrainLength)
  {
    os << this->EchoTrainLength;
  }

  os << "\n" << indent << "RepetitionTime: ";
  if (this->RepetitionTime)
  {
    os << this->RepetitionTime;
  }

  os << "\n" << indent << "ExposureTime: ";
  if (this->ExposureTime)
  {
    os << this->ExposureTime;
  }

  os << "\n" << indent << "XRayTubeCurrent: ";
  if (this->XRayTubeCurrent)
  {
    os << this->XRayTubeCurrent;
  }

  os << "\n" << indent << "Exposure: ";
  if (this->Exposure)
  {
    os << this->Exposure;
  }

  os << "\n"
     << indent << "DirectionCosine: (" << this->DirectionCosine[0] << ", "
     << this->DirectionCosine[1] << ", " << this->DirectionCosine[2] << "), ("
     << this->DirectionCosine[3] << ", " << this->DirectionCosine[4] << ", "
     << this->DirectionCosine[5] << ")\n";

  this->Internals->Print(os, indent);
}
