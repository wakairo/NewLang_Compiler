# R1066 original Actions ZIP custody

Authority: Compiler Issue #1066, Coordination C154/C155 and the user-authorized binary transfer. This is archival custody only, not an independent R execution, compiler validation, or adoption decision.

Archive branch: `r-archive/r1066-raw-zip-batch`.
Fixed sole parent: `3a03f528ca7a8296d1fd5e98577a24ee08a07ace`; parent tree `b5bb06c89a3fb78a326a3ae2dacd4ec7b5f7130e`.

The initial authenticated Actions ZIP download failed with Forbidden and produced zero bytes. The user then supplied the three ZIP attachments and explicitly authorized their use if matching. Each attached byte stream matches the original SHA256 and size recorded in C154/C155 and the GitHub artifact metadata. Files are stored directly, without extraction/recompression, wrapping, text transcription, or running any contained program.

| File | Artifact | Run / attempt | Bytes | SHA256 | Git blob OID | ZIP entries / CRC | JSONL records |
| --- | --- | --- | ---: | --- | --- | --- | ---: |
| `r1057-original-41015924765-attempt1.zip` | 12959691321 | 41015924765 / 1 | 106658 | `aae11fae2100b39fd8c69a46e6ef07fe378bfde77e40b1b40360c5a462a5a2c0` | `fe89ed154089924726e0c41900ebf0bb69125321` | 115 unique / all clean | 163 |
| `r1060-original-41026541892-attempt1.zip` | 12963384714 | 41026541892 / 1 | 106879 | `6e68c98864bd4711cff09bf8dfdbab69485bb43a77b07ca266d43f70bd121206` | `a34e78f57e04a287de6a6d6ae1e4c35a458ccd28` | 115 unique / all clean | 164 |
| `r1064-original-41103378465-attempt1.zip` | 13000855868 | 41103378465 / 1 | 168341 | `ece0898191ba20de7818172903dc5fa99b1b4b2fe386dbdbcf1895cebb82fe43` | `5534aee8095ebd8ee4af9fed36513172c3c6eeb7` | 170 unique / all clean | 244 |

`CALIBRATED-RAW.jsonl` SHA256, computed by reading the original archive member in memory:

- R1057: `e2f10870840a22a9d223d8597c087e88109f35593e79298826d318eb08ba9d4b` (46562 bytes).
- R1060: `7bbe9771a2c15ecb706fcb78f675d27607cf9f10e02b7892cfc6cbb00a052f38` (46783 bytes).
- R1064: `a471db09667dc2ab231cd0291a7456bc24266f660499e8885873ec8955599c97` (72830 bytes).

Before publication, the nine inherited workflow event/branch filters were inspected: the archive branch has no push trigger. No workflow or CI configuration is changed. Only these three ZIPs and this document are added; all inherited R sources, oracles, FIRST/RED/raw records and Production files remain unchanged.

Post-push verification must fetch the archive from GitHub into a separate object database, read the actual remote Git blobs, compare their full bytes with the supplied originals, and verify SHA256, sizes, ZIP CRC/unique entries, JSONL counts, parent/tree/diff and zero archive-branch workflow runs. The terminal Issue comment records that post-write result and commit/tree identifiers.

No compiler/native/runner/CI/test execution or per-stream semantic replay is performed by this archival task. Previous findings, including the older POS-B C17 RED and the later bounded native result, are unchanged. READY, merge and Product GO are not granted. Keep the archive ref reachable; Git custody does not depend on Actions artifact retention.
