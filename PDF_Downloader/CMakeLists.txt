# CMakeList.txt : CMake project for PDF_Downlaoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("PDF_Downloader")

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(CURL CONFIG REQUIRED)
find_package(OpenXLSX CONFIG REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add source to this project's executable.
add_executable (PDF_Downloader 
	"src/main.cpp" 
	"src/EzPDFDownloader.cpp" 
	"src/ExcelWorker.cpp" 
	"header/EzPDFDownloader.h" 
	"header/ExcelWorker.h" 
	"header/PDF_Downloader.h"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/header)

target_link_libraries(PDF_Downloader 
	PRIVATE
	Qt6::Widgets
	Qt6::Concurrent
	OpenXLSX::OpenXLSX 
	CURL::libcurl
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PDF_Downloader PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
