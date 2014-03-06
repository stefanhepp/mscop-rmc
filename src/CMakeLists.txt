project(RMC)

add_executable(rmc RMC.cpp Problem.cpp ReadXML.cpp)

include_directories(/usr/include/libxml2)

target_link_libraries(rmc gecodeflatzinc gecodedriver gecodegist gecodesearch gecodeminimodel gecodeset
	                  gecodeint gecodekernel gecodesupport xml2)
