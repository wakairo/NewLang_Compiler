# R970 own isolated first-pass tests, all 12 preregistered sources.
foreach(r970_case
    "A-POS"
    "B-POS"
    "A-NEG-WRONG-PTR"
    "A-NEG-BAD-FIELD"
    "B-NEG-BAD-PRODUCER"
    "B-NEG-DOUBLE-OWNER"
    "A2-POS-TF"
    "B2-POS-FT"
    "A2-NEG-MISSING-ELSE"
    "A2-NEG-ARM-RESULT"
    "B2-NEG-TAIL-DOMAIN"
    "B2-NEG-OWNER-SWAP"
)
    add_test(NAME "r970.${r970_case}.first_pass"
        COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/r970/r970_probe.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/r970/manifest.json"
          "${r970_case}" "$<TARGET_FILE:newlangc>"
          "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
    set_tests_properties("r970.${r970_case}.first_pass" PROPERTIES TIMEOUT 240)
endforeach()
