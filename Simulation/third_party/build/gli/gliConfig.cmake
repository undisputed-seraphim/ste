set(GLI_VERSION "0.8.2")
set(GLI_INCLUDE_DIRS "D:/src/git/StE/Simulation/third_party/packages/gli")

if (NOT CMAKE_VERSION VERSION_LESS "3.0")
	include("${CMAKE_CURRENT_LIST_DIR}/gliTargets.cmake")
endif()
