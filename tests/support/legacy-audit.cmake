# C293: ordinary v0 CI exercises behavior; old hash-chain audits are opt-in.
option(NEWLANG_LEGACY_AUDIT "Register and run optional legacy SHA/history audits" OFF)

function(newlang_add_legacy_audit)
    if(NEWLANG_LEGACY_AUDIT)
        add_test(${ARGV})
    endif()
endfunction()
