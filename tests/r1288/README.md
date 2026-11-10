# R1288 independent C410 FIRST / C412 execution (NEVERMERGE)

Fixed compiler parent: 870b14a9f2e6a3d94abbc2d4e4f0c2fcee1d66a7
Canonical: CURRENT_SPEC -> Draft17.35. Original source and oracle LF bytes
are reproduced from Issue #1288 C410 FIRST without modifying predictions.

P1: source SHA256 f1e802e28026c127e349af3e37fa6b6acc8cfeff30ec57fb53d887240b54a854; oracle SHA256 153ec7391c018fc3c4b54f273f0b7c93cf264fe41588cce797e26e558eae62b9
N1: source SHA256 3feaf8da92c7346b1d382ff6fffb9d8db7225f6230bb03dd320a9e372eb14582; oracle SHA256 8bf00b4cff64083a32d7479efae5ce27a21a6b62f8410d48175d167161e6900e
N2: source SHA256 138ca20ebd63a118ac1256436245b26d33c241147e881ca0d450b6e9df82ab05; oracle SHA256 2910af955be3b2f67c612cf450df3ad810fed6047d0bb043cc711e69f923c64b
N3: source SHA256 4c4dbd96f4addc4e0e979b6c98401ca7e0277082da5151055d16af6ad1eb9863; oracle SHA256 5e2c9a924be1b65b5298aafc38c7aaee08faf874328d7746ba499f3faddaea63
N4: source SHA256 afa92183d07b60f92fb55ea4c7351f6ad2f7343161c58eb06b91b9c9389f26a9; oracle SHA256 7b999dda0d81919c78481d9a5892390edf369884c71e723ec657c721d3aa14eb

The five T993 CTests execute real --check in natural FULL5 + PARTIAL1.
The extra R-only trace test independently reports exact raw CLI check bytes,
first diagnostic site, and attempts real C17/native only after P1 CHECK.
Target-site rejection is not proof of the predicted safety invariant;
parser-first failures do not count as semantic negatives. No separate
public-owned proof, internal T-state observation or injected OOM is claimed.
R-FAST builds and registers CTests but executes none. Never merge this PR.
