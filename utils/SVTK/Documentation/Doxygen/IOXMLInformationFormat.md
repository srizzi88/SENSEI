@page IOXMLInformationFormat SVTK XML Reader/Writer Information Format
@tableofcontents

# Overview #

The svtk xml data file readers / writers store certain `svtkInformation`
entries that are set on `svtkAbstractArray`'s `GetInformation()` object. Support
is currently limited to numeric and string information keys, both single- and
vector-valued. Only the information objects attached to arrays are written/read.

# Array Information #

Array information is embedded in the `<DataArray>` XML element as a series of
`<InformationKey>` elements. The required attributes `name` and `location`
specify the name and location strings associated with the key -- for instance,
the `svtkDataArray::UNITS_LABEL()` key has `name="UNITS_LABEL"` and
`location="svtkDataArray"`. The `length` attribute is required for vector keys.

```
<DataArray [...]>
  <InformationKey name="KeyName" location="KeyLocation" [ length="N" ]>
    [...]
  </InformationKey>
  <InformationKey [...]>
    [...]
  </InformationKey>
  [...]
</DataArray>
```

Specific examples of supported key types:

### svtkInformationDoubleKey ###

```
<InformationKey name="Double" location="XMLTestKey">
  1
</InformationKey>
```

### svtkInformationDoubleVectorKey ###

```
<InformationKey name="DoubleVector" location="XMLTestKey" length="3">
  <Value index="0">
    1
  </Value>
  <Value index="1">
    90
  </Value>
  <Value index="2">
    260
  </Value>
</InformationKey>
```

### svtkInformationIdTypeKey ###

```
<InformationKey name="IdType" location="XMLTestKey">
  5
</InformationKey>
```

### svtkInformationStringKey ###

```
<InformationKey name="String" location="XMLTestKey">
  Test String!
Line2
</InformationKey>
```

### svtkInformationIntegerKey ###

```
<InformationKey name="Integer" location="XMLTestKey">
  408
</InformationKey>
```

### svtkInformationIntegerVectorKey ###

```
<InformationKey name="IntegerVector" location="XMLTestKey" length="3">
  <Value index="0">
    1
  </Value>
  <Value index="1">
    5
  </Value>
  <Value index="2">
    45
  </Value>
</InformationKey>
```

### svtkInformationStringVectorKey ###

```
<InformationKey name="StringVector" location="XMLTestKey" length="3">
  <Value index="0">
    First
  </Value>
  <Value index="1">
    Second (with whitespace!)
  </Value>
  <Value index="2">
    Third (with
newline!)
  </Value>
</InformationKey>
```

### svtkInformationUnsignedLongKey ###

```
<InformationKey name="UnsignedLong" location="XMLTestKey">
  9
</InformationKey>
```
