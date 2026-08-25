# Validation for c89jpeg v1.1.0

## Commands run locally

```sh
make clean
make all
make test
make examples
```

## Observed output

```text
resume_source_optimal ref=2211 out=2211 prefix=177 replay=9216
resume_source_optimal_requires_replay status=short buffer
resume_source_custom out=2190 rows=48
all tests passed

source_resume_optimal_replay jpeg=21184 prefix=177 rows=240 replay=230400
wrote build/example_source_resume_optimal_replay.jpg
wrote build/example_source_resume_optimal_replay.ppm
source_resume_session tables=580 frame=45773 rows=240
wrote build/example_source_resume_tables.jpg
wrote build/example_source_resume_frame1.abbr.jpg
wrote build/example_source_resume_frame1.ppm
```

## What the validation covers

- optimal source-resume + replay matches the non-resumable source-optimal encoder byte-for-byte
- the streamed encoder can emit a non-zero header/prefix before all image rows arrive
- missing replay storage for optimal mode is rejected early with `short buffer`
- custom Huffman replay works on the source-resume path
- trained shared-table resumable source sessions still work after the replay-layer change
