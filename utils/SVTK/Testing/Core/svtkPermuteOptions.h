/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPermuteOptions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkPermuteOptions_h
#define svtkPermuteOptions_h

#include <svtkTimeStamp.h>

#include <cassert>
#include <functional>
#include <sstream>
#include <vector>

/**
 * svtkPermuteOptions is a class template designed to exhaustively explore the
 * parameter space of a svtkObject subclass.
 *
 * This testing utility can be taught to update parameters that are defined
 * using an API similar to the svtkSetGet macros. Concretely, consider testing
 * svtkXMLWriter. This class has a number of independent settings: byte order,
 * compressor, data mode, and more. When testing this class, it would be ideal
 * to test every combination of these parameters, but this would normally
 * require a lot of verbose, redundant, error-prone boilerplate code.
 *
 * This class simplifies this process. The following describes how to use
 * svtkPermuteOptions to run a test using all combinations of svtkXMLWriter's
 * byte order and compressor settings (just sticking to two options for
 * simplicity -- the class has no limit on number of options or number of
 * values for those options).
 *
 * First, the svtkPermuteOptions object must be instantiated, using the
 * configured class as the template parameter:
 *
 * @code
 * svtkPermuteOptions<svtkXMLWriter> config;
 * @endcode
 *
 * Next the options and their possible values are specified. Each call to
 * AddOptionValue adds a value to a specific option. Options are created
 * automatically as new option names are passed to AddOptionValue. The
 * following instructs svtkPermuteOptions to test option ByteOrder (with values
 * LittleEndian and BigEndian) and CompressorType (with values NONE, ZLIB, and
 * LZ4):
 *
 * @code
 * this->AddOptionValue("ByteOrder", &svtkXMLWriter::SetByteOrder,
 *                      "BigEndian", svtkXMLWriter::BigEndian);
 * this->AddOptionValue("ByteOrder", &svtkXMLWriter::SetByteOrder,
 *                      "LittleEndian", svtkXMLWriter::LittleEndian);
 *
 * this->AddOptionValues("CompressorType", &svtkXMLWriter::SetCompressorType,
 *                       "NONE", svtkXMLWriter::NONE,
 *                       "ZLIB", svtkXMLWriter::ZLIB,
 *                       "LZ4", svtkXMLWriter::LZ4);
 * @endcode
 *
 * Note that that there are two variations on how values may be added to an
 * option. For ByteOrder, we use AddOptionValue to specify a human-readable
 * string that uniquely identifies the option, a member function pointer to the
 * option's setter, a human readable string that uniquely identifies the value,
 * and the value itself (in this case, an enum value). The first call creates
 * the option named "ByteOrder" and adds the "BigEndian" value. The second call
 * adds the "LittleEndian" value to the same option.
 *
 * The CompressorType call uses the variatic function template AddOptionValues
 * to specify multiple values to the same option at once. The value-name and
 * value pairs are repeated, and each is added to the option with the supplied
 * name. Any number of values may be added to a single option this way.
 *
 * To run through the permutations, a svtk-esque iterator API is used:
 *
 * @code
 * config.InitPermutations();
 * while (!config.IsDoneWithPermutations())
 * {
 *   // Testing code...
 *
 *   // Apply the current option permutation to a svtkXMLWriter object:
 *   svtkXMLWriter *writer = ...;
 *   config.ApplyCurrentPermutation(writer);
 *
 *   // More testing code...
 *
 *   config.GoToNextPermutation();
 * }
 * @endcode
 *
 * This will repeat the testing code, but configure the svtkXMLWriter object
 * differently each time. It will perform a total of 6 iterations, with
 * parameters:
 *
 * @code
 * Test Iteration    ByteOrder         CompressorType
 * --------------    ---------         --------------
 * 1                 BigEndian         NONE
 * 2                 BigEndian         ZLIB
 * 3                 BigEndian         LZ4
 * 4                 LittleEndian      NONE
 * 5                 LittleEndian      ZLIB
 * 6                 LittleEndian      LZ4
 * @endcode
 *
 * thus exploring the entire parameter space.
 *
 * A unique, human-readable description of the current configuration can be
 * obtained with GetCurrentPermutationName() as long as IsDoneWithPermutations()
 * returns false. E.g. the third iteration will be named
 * "ByteOrder.BigEndian-CompressorType.LZ4".
 */
template <typename ObjType>
class svtkPermuteOptions
{
  using Permutation = std::vector<size_t>;

  struct Value
  {
    Value(const std::string& name, std::function<void(ObjType*)> setter)
      : Name(name)
      , Setter(setter)
    {
    }

    void Apply(ObjType* obj) const { this->Setter(obj); }

    std::string Name;                     // user-readable option name
    std::function<void(ObjType*)> Setter; // Sets the option to a single values
  };

  struct Option
  {
    Option(const std::string& name)
      : Name(name)
    {
    }
    std::string Name;          // user-readable option name
    std::vector<Value> Values; // list of values to test for this option
  };

  std::vector<Option> Options;
  std::vector<Permutation> Permutations;
  size_t CurrentPermutation;
  svtkTimeStamp OptionTime;
  svtkTimeStamp PermutationTime;

  Option& FindOrCreateOption(const std::string& name)
  {
    for (Option& opt : this->Options)
    {
      if (opt.Name == name)
      {
        return opt;
      }
    }

    this->Options.emplace_back(name);
    return this->Options.back();
  }

  void RecursePermutations(Permutation& perm, size_t level)
  {
    const size_t maxIdx = this->Options[level].Values.size();

    if (level == 0) // base case
    {
      for (size_t i = 0; i < maxIdx; ++i)
      {
        perm[0] = i;
        this->Permutations.push_back(perm);
      }
    }
    else // recursive case
    {
      for (size_t i = 0; i < maxIdx; ++i)
      {
        perm[level] = i;
        this->RecursePermutations(perm, level - 1);
      }
    }
  }

  void RebuildPermutations()
  {
    this->Permutations.clear();

    const size_t numOptions = this->Options.size();
    Permutation perm(numOptions, 0);
    this->RecursePermutations(perm, numOptions - 1);
    this->PermutationTime.Modified();
  }

  void Apply(ObjType* obj, const Permutation& perm) const
  {
    const size_t numOpts = this->Options.size();
    assert("Sane permutation" && perm.size() == numOpts);

    for (size_t i = 0; i < numOpts; ++i)
    {
      size_t valIdx = perm[i];
      assert("ValueIdx in range" && valIdx < this->Options[i].Values.size());
      this->Options[i].Values[valIdx].Apply(obj);
    }
  }

  std::string NamePermutation(const Permutation& perm) const
  {
    const size_t numOpts = this->Options.size();
    assert("Sane permutation" && perm.size() == numOpts);

    std::ostringstream out;
    for (size_t i = 0; i < numOpts; ++i)
    {
      size_t valIdx = perm[i];
      assert("ValueIdx in range" && valIdx < this->Options[i].Values.size());

      out << (i != 0 ? "-" : "") << this->Options[i].Name << "."
          << this->Options[i].Values[valIdx].Name;
    }

    return out.str();
  }

public:
  svtkPermuteOptions()
    : CurrentPermutation(0)
  {
  }

  template <typename SetterType, typename ValueType>
  void AddOptionValue(
    const std::string& optionName, SetterType setter, const std::string& valueName, ValueType value)
  {
    using std::placeholders::_1;

    std::function<void(ObjType*)> func = std::bind(setter, _1, value);
    Option& opt = this->FindOrCreateOption(optionName);
    opt.Values.emplace_back(valueName, func);
    this->OptionTime.Modified();
  }

  template <typename SetterType, typename ValueType>
  void AddOptionValues(
    const std::string& optionName, SetterType setter, const std::string& valueName, ValueType value)
  {
    this->AddOptionValue(optionName, setter, valueName, value);
  }

  template <typename SetterType, typename ValueType, typename... Tail>
  void AddOptionValues(const std::string& optionName, SetterType setter,
    const std::string& valueName, ValueType value, Tail... tail)
  {
    this->AddOptionValue(optionName, setter, valueName, value);
    this->AddOptionValues(optionName, setter, tail...);
  }

  void InitPermutations()
  {
    if (this->OptionTime > this->PermutationTime)
    {
      this->RebuildPermutations();
    }

    this->CurrentPermutation = 0;
  }

  bool IsDoneWithPermutations() const
  {
    assert("Modified options without resetting permutations." &&
      this->PermutationTime > this->OptionTime);

    return this->CurrentPermutation >= this->Permutations.size();
  }

  void GoToNextPermutation()
  {
    assert("Modified options without resetting permutations." &&
      this->PermutationTime > this->OptionTime);
    assert("Invalid permutation." && !this->IsDoneWithPermutations());

    ++this->CurrentPermutation;
  }

  void ApplyCurrentPermutation(ObjType* obj) const
  {
    assert("Modified options without resetting permutations." &&
      this->PermutationTime > this->OptionTime);
    assert("Invalid permutation." && !this->IsDoneWithPermutations());

    this->Apply(obj, this->Permutations[this->CurrentPermutation]);
  }

  std::string GetCurrentPermutationName() const
  {
    assert("Modified options without resetting permutations." &&
      this->PermutationTime > this->OptionTime);
    assert("Invalid permutation." && !this->IsDoneWithPermutations());
    return this->NamePermutation(this->Permutations[this->CurrentPermutation]);
  }
};

#endif
// SVTK-HeaderTest-Exclude: svtkPermuteOptions.h
