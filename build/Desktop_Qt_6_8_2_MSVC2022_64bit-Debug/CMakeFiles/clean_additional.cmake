# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "APO-Lab-App_autogen"
  "CMakeFiles\\APO-Lab-App_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\APO-Lab-App_autogen.dir\\ParseCache.txt"
  )
endif()
