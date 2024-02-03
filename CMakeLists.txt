cmake_minimum_required(VERSION 3.8)
project(adfman)

set(SRC ${CMAKE_SOURCE_DIR}/src)

add_executable(adfman
	${SRC}/adf-man.c
	${SRC}/adflib/adf_bitm.c
	${SRC}/adflib/adf_cache.c
	${SRC}/adflib/adf_dev.c
	${SRC}/adflib/adf_dev_dump.c
	${SRC}/adflib/adf_dev_flop.c
	${SRC}/adflib/adf_dev_hd.c
	${SRC}/adflib/adf_dir.c
	${SRC}/adflib/adf_env.c
	${SRC}/adflib/adf_file_block.c
	${SRC}/adflib/adf_file.c
	${SRC}/adflib/adf_link.c
	${SRC}/adflib/adf_nativ.c
	${SRC}/adflib/adf_raw.c
	${SRC}/adflib/adf_salv.c
	${SRC}/adflib/adf_str.c
	${SRC}/adflib/adf_util.c
	${SRC}/adflib/adf_vol.c
	${SRC}/adflib/debug_util.c
)

target_include_directories(
	adfman PUBLIC
	${SRC}/adflib/
)

install(TARGETS adfman
    COMPONENT linapp
    RUNTIME DESTINATION "/bin/"
    LIBRARY DESTINATION "/lib/"
    DESTINATION "/bin/"
)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "KK") #required
INCLUDE(CPack)
