# Credit: QuarksLab 2015
# Source: https://github.com/quarkslab/llvm-dev-meeting-tutorial-2015/blob/master/cmake/Python.cmake

include(FindPackageHandleStandardArgs)
find_package(PythonInterp 3 REQUIRED)

function(find_python_module module)
	string(TOUPPER ${module} module_upper)
        set(_SUPPORT_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/cmake/find_python_module.py")
	if(NOT PY_${module_upper})
		# A module's location is usually a directory, but for binary modules
		# it's a .so file.
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${_SUPPORT_SCRIPT_PATH}" "${module}"
			RESULT_VARIABLE _${module}_status
			OUTPUT_VARIABLE _${module}_location
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(NOT _${module}_status)
			set(PY_${module_upper} ${_${module}_location} CACHE STRING 
				"Location of Python module ${module}")
		endif(NOT _${module}_status)
	endif(NOT PY_${module_upper})
	find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED" AND NOT PY_${module_upper})
        message(FATAL_ERROR "Python module ${module} not found")
    endif()
endfunction(find_python_module)
