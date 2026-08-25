# BMP compatibility matrix

| Case | Origin | Parse | Decode | Expectation |
|---|---|---:|---:|---:|
| generated-indexed2 | generated | True | True | True |
| generated-os2v2-64 | generated | True | True | True |
| generated-invalid-topdown-rle8 | generated | False | False | True |
| generated-payload-png | generated | True | False | True |
| generated-payload-jpeg | generated | True | False | True |
| generated-valid-rgb24 | generated | True | True | True |
| pal2 | external | n/a | n/a | missing |
| pal8os2 | external | n/a | n/a | missing |
| pal8os2v2 | external | n/a | n/a | missing |
| pal8os2v2-16 | external | n/a | n/a | missing |
| rgb32h52 | external | n/a | n/a | missing |
| rgba32h56 | external | n/a | n/a | missing |
| pal8rle | external | n/a | n/a | missing |
| rgb24png | external | n/a | n/a | missing |
| rgb24jpeg | external | n/a | n/a | missing |
| rletopdown | external | n/a | n/a | missing |
