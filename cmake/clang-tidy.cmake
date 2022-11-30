find_program(CLANG_TIDY_PROGRAM clang-tidy-14 clang-tidy)
if(CLANG_TIDY_PROGRAM)
    # Here we add checks that we know are good, and produce a minimal amount of false positives
    # One or two false positives can be dealt with using // NOLINT

    set(CLANG_TIDY_WERROR "")
    if(WERROR)
        set(CLANG_TIDY_WERROR "--warnings-as-errors=*")
    endif(WERROR)

    execute_process(COMMAND ${CLANG_TIDY_PROGRAM} --version OUTPUT_VARIABLE CLANG_TIDY_VERSION_STRING)
    string(FIND ${CLANG_TIDY_VERSION_STRING} "LLVM version" startloc)
    string(SUBSTRING ${CLANG_TIDY_VERSION_STRING} ${startloc} -1 withoutstart)
    string(FIND ${withoutstart} "\n" endloc)
    string(SUBSTRING ${withoutstart} 13 ${endloc} TIDY_VERSION)

    string(FIND ${TIDY_VERSION} "." major_end)
    string(SUBSTRING ${TIDY_VERSION} 0 ${major_end} TIDY_MAJOR_VERSION)

    # We use NOLINTBEGIN and NOLINTEND to preserve some macros
    # This was only introduced in version 14
    if (TIDY_MAJOR_VERSION GREATER_EQUAL 14)
        message(STATUS "Found clang-tidy version ${TIDY_VERSION}")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PROGRAM} -checks=-*,cppcoreguidelines-macro-usage,hicpp-deprecated-headers,modernize-deprecated-headers,hicpp-use-override,hicpp-use-emplace,modernize-use-emplace,hicpp-use-auto,readability-container-size-empty,readability-implicit-bool-conversion,readability-redundant-smartptr-get,readability-qualified-auto,performance-unnecessary-value-param,modernize-make-unique,modernize-make-shared,misc-unused-using-decls,performance-move-const-arg,modernize-use-using,modernize-use-nullptr,modernize-deprecated-headers,modernize-loop-convert,misc-unused-using-decls,misc-static-assert,misc-redundant-expression,modernize-use-bool-literals,readability-delete-null-pointer,readability-redundant-member-init --warnings-as-errors=*)
    else()
        message(WARNING "Found clang-tidy version ${TIDY_VERSION}, but 14 or higher is required. Disabling clang-tidy")
    endif()
else(CLANG_TIDY_PROGRAM)
    message(WARNING "Failed to find clang-tidy >=14. Some checks are not enabled")
endif(CLANG_TIDY_PROGRAM)
