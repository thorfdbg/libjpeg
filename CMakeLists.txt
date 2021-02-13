
cmake_minimum_required(VERSION 3.13...3.19)

project(jpeg VERSION 1.58 LANGUAGES CXX)

# Determine if project is built as a subproject (using add_subdirectory) or if it is the master project
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
  message(STATUS "Building as master project, CMake version: ${CMAKE_VERSION}")
endif ()

# The basic options to control what to build extra
option(LIBJPEG_BUILD_TOOL "Build JPEG tool application" ${MASTER_PROJECT})

# Provide BUILD_SHARED_LIBS as an option for GUI tools
option(BUILD_SHARED_LIBS "Will control if libjpeg is build as shared lib/DLL or as static library")

# Options to increase the warning level and warnings as errors
option(LIBJPEG_PEDANTIC_WARNINGS "Enable extra compilers warnings." OFF)
option(LIBJPEG_THREAT_WARNINGS_AS_ERRORS "Enable the option to treat warnings as errors." OFF)

# Options to increase additional debug checks
set(LIBJPEG_DEBUG_LEVEL 0 CACHE STRING "Defines the debug level {0 (off), 2, 3}")
set(LIBJPEG_CHECK_LEVEL 0 CACHE STRING "Defines the check level {0 (off), 2, 3}")

# Build with C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure the supported C++ compilers: gcc, clang and MSVC
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(
    -fno-exceptions
    -Wall
    -Wextra      # Alias for -W
    -Wpedantic   # Alias for -pedantic (also includes: -Wpointer-arith)
    -Wunused
    -Wcast-qual
    -Wwrite-strings
    -Woverloaded-virtual
    -Wsign-promo
    -Wno-shift-negative-value
    -Wno-class-memaccess
    -Wimplicit-fallthrough=0 # Disable implicit-fallthrough warnings
    -Wno-unused-parameter  # Disable warning about unused parameters
    -Wno-clobbered
    -Wno-unused-result
  )

  set(PEDANTIC_CXX_COMPILE_FLAGS
    -pedantic-errors
    -Wold-style-cast
    -Wfloat-equal
    -Wlogical-op
    -Wundef
    -Wshadow
    -Wcast-qual
    -Wformat=2
    -Wmissing-include-dirs
    -Wcast-align
    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Winvalid-pch
    -Woverloaded-virtual
    -Wnon-virtual-dtor
    -Wnoexcept
    -Wdouble-promotion
    -Wtrampolines
    -Wzero-as-null-pointer-constant
    -Wuseless-cast
    -Wvector-operation-performance
    -Wsized-deallocation
  )
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
      set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
        -Wshift-overflow=2
        -Wnull-dereference
        -Wduplicated-cond
      )
  endif()

  set(WERROR_FLAG -Werror)
endif()


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -fno-exceptions
    -Wall
    -Wextra      # Alias for -W
    -Wpedantic   # Alias for -pedantic (also includes: -Wpointer-arith)
    -Wcast-qual
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wno-unused-parameter  # Disable warning about unused parameters
  )

  set(PEDANTIC_CXX_COMPILE_FLAGS
      -Weverything
      -Wno-weak-vtables                       # Ignore, linker will remove the couple of extra vtables.
      -Wno-padded                             # Ignore, padding optimization is not needed.
      -Wno-global-constructors                # Ignore, by design CharLS uses types created at startup.
      -Wno-switch-enum                        # Ignore, cases are handled by default.
      -Wno-sign-conversion                    # Ignore, would just introduce ugly static_asserts.
      -Wno-exit-time-destructors              # Ignore, by design exit-time destructors are used.
      -Wno-missing-braces                     # Ignore, False warning in clang 5.0, fixed in 6.0.
      -Wno-c++98-compat-pedantic              # Ignore, target is C++11, ignore C++98 compatibility.
  )

  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0.2)
    set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
      -Wno-undefined-func-template            # Ignore, linker will complain if final template code is not available.
    )
  endif()

  set(WERROR_FLAG -Werror)
endif()


if(MSVC)
  set(PEDANTIC_CXX_COMPILE_FLAGS /W4)
  set(WERROR_FLAG /WX)

  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")

  # Remove option /GR (/GR is added by default by CMake). /GR is already the default
  # and this makes it possible to use /GR- without warnings.
  string(REGEX REPLACE " /GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  # All C++ source files are in utf-8 without signature (BOM), MSVC requires the utf-8 switch to read files as utf-8.
  add_compile_options("/utf-8")

  # Zc:__cplusplus: Will configure the MSVC compiler to use the correct value for the __cplusplus macro
  # (introduced with Visual Studio 2017 version 15.7)
  if(MSVC_VERSION GREATER_EQUAL 1914)
    add_compile_options("/Zc:__cplusplus")
  endif()

  # /Zc:throwingNew: Will configure the MSVC compiler that only the standard throwing operator new is used.
  add_compile_options("/Zc:throwingNew")
endif()


# When enabled apply the pedantic warnings options and warnings as errors globally.
if(LIBJPEG_PEDANTIC_WARNINGS)
  if(MSVC)
    # Remove existing warning level (W3 is added by default by CMake), duplicate level will generate cmd-line warnings.
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()

  add_compile_options(${PEDANTIC_CXX_COMPILE_FLAGS})
endif()
if(LIBJPEG_THREAT_WARNINGS_AS_ERRORS)
  add_compile_options(${WERROR_FLAG})
endif()


# Define the library "libjpeg"
add_library(jpeg "")

set_target_properties(jpeg PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(jpeg PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(jpeg PUBLIC DEBUG_LEVEL=${LIBJPEG_DEBUG_LEVEL} CHECK_LEVEL=${LIBJPEG_CHECK_LEVEL})

# When building as DLL (Windows) or shared libray, define JPEG_EXPORT
if(BUILD_SHARED_LIBS)
  target_compile_definitions(jpeg PRIVATE BUILD_LIB)
endif()

# Add all .cpp and .hpp sources files. Adding .hpp is not needed for the build but it helps IDEs to show the files.
target_sources(jpeg
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/boxes/alphabox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/box.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/checksumbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/colortrafobox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/databox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/dctbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/filetypebox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/floattonemappingbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/floattransformationbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/inversetonemappingbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/lineartransformationbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/matrixbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/mergingspecbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/namespace.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/nonlineartrafobox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/outputconversionbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/parametrictonemappingbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/refinementspecbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/superbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/tonemapperbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/aclosslessscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/acrefinementscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/acsequentialscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/decoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/encoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/entropyparser.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/image.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/jpeglsscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/lineinterleavedlsscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/losslessscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictivescan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictorbase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/rectanglerequest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/refinementscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/sampleinterleavedlsscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/sequentialscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/singlecomponentlsscan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/tables.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/actemplate.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/arithmetictemplate.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/arthdeco.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/blockrow.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/decodertemplate.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmancoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmandecoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmanstatistics.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmantemplate.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/qmcoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/quantizedrow.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/colortrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/colortransformerfactory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/floattrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/lslosslesstrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/multiplicationtrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/trivialtrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/ycbcrtrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/bitmapctrl.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blockbitmaprequester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blockbuffer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blocklineadapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/bufferctrl.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/hierarchicalbitmaprequester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/lineadapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linebitmaprequester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linebuffer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linelineadapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linemerger.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/residualblockhelper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/dct.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/deringing.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/idct.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/liftingdct.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/bitmaphook.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/hooks.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/imagebitmap.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/jpeg.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/jpgtypes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/parameters.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/tagitem.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/types.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/bitstream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/bytestream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/checksumadapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/decoderstream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/iostream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/memorystream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/randomaccessstream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/staticstream.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/actable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/adobemarker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/component.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/exifmarker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/frame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/huffmantable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/jfifmarker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/lscolortrafo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/quantization.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/quantizationtable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/restartintervalmarker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/scan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/scantypes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/thresholds.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/assert.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/ctype.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/errno.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/math.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/setjmp.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdarg.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stddef.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdio.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdlib.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/string.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/unistd.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/checksum.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/debug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/environment.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/line.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/numerics.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/priorityqueue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/rectangle.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/traits.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/downsampler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/interdownsampler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/downsamplerbase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/upsampler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/cositedupsampler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/upsamplerbase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/alphabox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/box.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/checksumbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/databox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/dctbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/filetypebox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/floattonemappingbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/floattransformationbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/inversetonemappingbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/lineartransformationbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/matrixbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/mergingspecbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/namespace.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/nonlineartrafobox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/outputconversionbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/parametrictonemappingbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/refinementspecbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/superbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boxes/tonemapperbox.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/aclosslessscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/acrefinementscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/acsequentialscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/decoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/encoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/entropyparser.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/image.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/jpeglsscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/lineinterleavedlsscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/losslessscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictivescan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/predictorbase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/rectanglerequest.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/refinementscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/sampleinterleavedlsscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/sequentialscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/singlecomponentlsscan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/codestream/tables.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/actemplate.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/arithmetictemplate.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/arthdeco.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/blockrow.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/decodertemplate.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmancoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmandecoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmanstatistics.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/huffmantemplate.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/qmcoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding/quantizedrow.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/colortrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/colortransformerfactory.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/floattrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/integertrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/lslosslesstrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/multiplicationtrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/trivialtrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/colortrafo/ycbcrtrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/config.h"
    "${CMAKE_CURRENT_LIST_DIR}/control/bitmapctrl.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blockbitmaprequester.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blockbuffer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/blocklineadapter.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/bufferctrl.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/hierarchicalbitmaprequester.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/lineadapter.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linebitmaprequester.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linebuffer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linelineadapter.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/linemerger.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/residualblockhelper.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/control/residualbuffer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/dct.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/deringing.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/idct.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/dct/liftingdct.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/bitmaphook.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/hooks.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/imagebitmap.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/jpeg.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/jpgtypes.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/parameters.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/tagitem.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/interface/types.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/bitstream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/bytestream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/checksumadapter.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/decoderstream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/iostream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/memorystream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/randomaccessstream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/io/staticstream.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/actable.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/adobemarker.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/component.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/exifmarker.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/frame.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/huffmantable.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/jfifmarker.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/lscolortrafo.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/quantization.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/quantizationtable.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/restartintervalmarker.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/scan.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/scantypes.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/marker/thresholds.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/assert.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/ctype.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/errno.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/math.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/setjmp.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdarg.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stddef.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdio.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/stdlib.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/string.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/std/unistd.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/checksum.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/debug.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/environment.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/line.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/numerics.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/priorityqueue.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/rectangle.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/tools/traits.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/downsampler.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/interdownsampler.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/downsamplerbase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/upsampler.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/cositedupsampler.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/upsampling/upsamplerbase.hpp"
)


if(LIBJPEG_BUILD_TOOL)
  add_subdirectory(cmd)
endif()
