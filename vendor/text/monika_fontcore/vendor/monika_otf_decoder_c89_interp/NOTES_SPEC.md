# Spec notes implemented in this build

## HVAR

The HVAR table provides variation deltas for horizontal glyph metrics. This decoder parses:

- major/minor version
- ItemVariationStore offset
- advanceWidthMappingOffset
- lsbMappingOffset
- rsbMappingOffset is parsed/stored but not exposed yet

If advanceWidthMappingOffset is NULL, glyph IDs are used as implicit delta-set indices: outer index 0, inner index glyph ID.

## VVAR

The VVAR table is analogous to HVAR for vertical metrics. This decoder parses:

- ItemVariationStore offset
- advanceHeightMappingOffset
- tsbMappingOffset
- bsbMappingOffset is parsed/stored but not exposed yet
- vOrgMappingOffset

The decoder exposes advance height, top side bearing and vertical-origin adjustment.

## MVAR

The MVAR table provides font-wide metric deltas keyed by four-byte tags.

This decoder parses:

- valueRecordSize
- valueRecordCount
- ItemVariationStore
- ValueRecord array: valueTag, outer index, inner index

The API returns the interpolated delta for a tag. The caller applies that delta to the default metric from OS/2, hhea, vhea, post, etc.

## Generic ItemVariationStore

Implemented for HVAR/VVAR/MVAR:

- VariationRegionList
- ItemVariationData offsets
- region indices
- int16/int8 rows
- int32/int16 rows when LONG_WORDS is set
- special 0xFFFF/0xFFFF no-variation index
- cached region scalars in fixed 16.16

## DeltaSetIndexMap

Implemented:

- format 0 with uint16 mapCount
- format 1 with uint32 mapCount
- packed entryFormat decoding
- last-entry fallback when requested index >= mapCount

## Numeric model

All scalar math is fixed 16.16. Deltas are rounded back to integer font units after multiplying by scalars.

## Known limitations

- HVAR right side bearing and VVAR bottom side bearing are parsed but not currently exposed through a public accessor.
- MVAR returns raw deltas; it does not mutate the stored `otf_font` base metrics.
- Static limits reject extreme fonts instead of allocating memory dynamically.
