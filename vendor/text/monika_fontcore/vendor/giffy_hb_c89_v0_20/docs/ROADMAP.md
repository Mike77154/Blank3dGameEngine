# Roadmap after v0.20

## v0.21 natural targets

- HVAR
  - stronger DeltaSetIndexMap edge-case parser in the offline packer
  - compact per-glyph contribution table for debug HUDs
  - optional packer report for region-axis invalidity

- GSUB/GPOS
  - emit contextual PairPos rows from packer into ghb_context_pair_adjust
  - expand ContextSubst format 2 extraction
  - expand ChainContextSubst format 1/2 extraction

- Indic/SEA
  - reph final placement per script
  - final cluster status after feature application
  - dotted-circle pass based on final cluster status

- Debug
  - documented stable HUD event table
  - numeric-only dump formatter examples
  - overlay/event mask cookbook
