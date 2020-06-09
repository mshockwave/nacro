include(Python)

find_python_module(lit REQUIRED)
set(LLVM_LIT "${CMAKE_CURRENT_SOURCE_DIR}/llvm-lit.py")

find_python_module(filecheck REQUIRED)
set(FILECHECK_PATH "${PY_FILECHECK}/FileCheck.py")

configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check
  COMMAND ${LLVM_LIT}
          "${CMAKE_CURRENT_BINARY_DIR}" -v
  DEPENDS NacroPlugin)
