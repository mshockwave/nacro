# TODO: Try to make lit module on pypi works such that
# users don't need a LLVM tree by them
set(LLVM_LIT "${LLVM_BUILD_BINARY_DIR}/bin/llvm-lit")
if(NOT EXISTS ${LLVM_LIT})
  message(FATAL_ERROR "Tests require llvm-lit to run")
endif()

configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check
  COMMAND ${LLVM_LIT}
          "${CMAKE_CURRENT_BINARY_DIR}" -v
  DEPENDS NacroPlugin)
