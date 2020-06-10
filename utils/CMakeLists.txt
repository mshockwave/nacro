
set(CLANG_EXE_PATH "${LLVM_TOOLS_BINARY_DIR}/clang")
# FIXME: Unfortunately there seems to be no portable way to get
# output path of NacroPlugin. Since generator expression (i.e. $<TARGET_FILE>)
# will only evaluated during build time and get_property on LOCATION doesn't
# work for this target, somehow.
set(NACRO_PLUGIN_PATH "${CMAKE_BINARY_DIR}/NacroPlugin.so")

configure_file(clang-nacro.sh.in ${CMAKE_BINARY_DIR}/clang-nacro @ONLY)
