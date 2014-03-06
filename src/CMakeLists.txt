project(RMC)

add_executable(rmc RMC.cpp Problem.cpp ReadXML.cpp)

include_directories(/usr/include/libxml2)
include_directories(/opt/gecode/gecode-3.7.3)

link_directories(/opt/gecode/gecode-3.7.3)
#find_library(Gecode gecodeint PATHS /opt/gecode/gecode-3.7.3)

set(CMAKE_EXE_LINKER_FLAGS "-L/opt/gecode/gecode-3.7.3")

target_link_libraries(rmc gecodeflatzinc gecodedriver gecodegist gecodesearch gecodeminimodel gecodeset
	                  gecodeint gecodekernel gecodesupport xml2)
