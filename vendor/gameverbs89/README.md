# gameverbs89

A tiny named verb bus for C89 engines. It lets independent DSLs and systems
share condition/action vocabulary without depending on each other.

- fixed-capacity registry
- no heap
- no float/double
- providers keep gameplay authority
- condition and action namespaces are separate

Blank3D uses it to expose 3DKin-GF spatial conditions and MovementBaseVerbs89
locomotion actions to DDSL2/FPIL/RPYL through one host vocabulary.
