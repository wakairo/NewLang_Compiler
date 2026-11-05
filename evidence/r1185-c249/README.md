# R1185 C249 immutable evidence archive

This archive is append-only beneath evidence/r1185-c249. Its sole commit parent is the completed R execution commit a6367e6bc8e52acfee620ca9d840afdc4b700d38. The execution branch and DRAFT/NEVERMERGE PR1189 are not moved. New PR/Actions/compiler/build/native/AST/observer semantic execution: zero.

## Restore exact original files

Run `python restore.py . /path/to/new-empty-output` from this directory. For hash verification without writing recovered files, run `python restore.py . --verify-only`. Only Python's standard library is needed. No compiler, target program, observer, network, or subprocess is invoked. Do not run the restored execution drivers locally.

The manifest lists every original path, byte length, mode and SHA256 through file-index parts. Object-index parts describe exact-byte custody. Whole decoded logs are stored objects. Each original raw gzip is recovered by joining its original log's R1185_RAW payload lines between the unique declared BEGIN/END pair and verifying its full byte count/hash. Original C/AST/map/passive/raw, metadata, FIRST/RED/NOT_RUN and frozen inputs are exact tar members. Repeated files share content identities; the six frozen base64 reuse assets are the exact base64 encoding of their original gzip objects. This removes duplicate storage without removing any original file or changing any original bytes.

Objects that cannot be recovered from existing raw are stored under content hashes in the gzip bundle. Join decoded chunks in manifest order; the script checks each encoded/decoded chunk, the complete bundle, each stored/reconstructed object and every original file. The root MANIFEST.sha256 covers all repository archive files except itself. Git blob hashes are recorded separately in the publication receipt.

## Evidence and limits

All 22 whole decoded logs, 10 terminal run/job metadata sets, 18 NORMAL1/2/3 original raw gzip exports, V1/V2, E0-E6, original failures and NOT_RUN, all captured artifacts, final reports and reconstruction scripts are preserved. UTF-8 decoded job-log bytes are what the connector supplied; no independently acquired binary job-log ZIP hash is claimed. V1 remains NOT_RUN. V2 is unchanged. Historical target/budget text in immutable V2 is retained; actual execution binding is C247/C248 fixed faad092b5b0deb608a048ff60bee0056082f9a59 and explicit release receipts.

Final finite results were accepted by C249. This is evidence preservation, not a new execution or adoption gate. Ternary choices are inference from captured operands and actual AST; caller results and internal RETURN_VALUE observations remain separate. Public IDs require artifact/world qualifiers and are not native addresses. No machine CALL/STORE survival, general freshness/authority/soundness or unavailable whole-loop-validator proof is asserted. The complete concise result is in results/FINAL.md; the small results bundle is independently reconstructible from results/result-receipts.tar.gz.b64.
