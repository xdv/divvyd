
cmake_minimum_required (VERSION 3.9.0)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Builds/CMake ${CMAKE_MODULE_PATH})
include (CMakeFuncs)
include (CheckCXXCompilerFlag)
include (ExternalProject)
if (target)
  message (WARNING
    "The target option is deprecated and will be removed in a future release")
  parse_target()
endif ()
project (divvyd)

#[===================================================================[
   convenience variables and sanity checks
#]===================================================================]

get_property (is_multiconfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (is_multiconfig STREQUAL "NOTFOUND")
  if (${CMAKE_GENERATOR} STREQUAL "Xcode" OR ${CMAKE_GENERATOR} MATCHES "^Visual Studio")
    set (is_multiconfig TRUE)
  endif ()
endif ()

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set (is_root_project ON)
else ()
  set (is_root_project OFF)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang") # both Clang and AppleClang
  set (is_clang TRUE)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (is_gcc TRUE)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
    message (FATAL_ERROR "This project requires GCC 5.1 or later")
  endif ()
endif ()
if (${CMAKE_GENERATOR} STREQUAL "Xcode")
  set (is_xcode TRUE)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set (is_linux TRUE)
else ()
  set (is_linux FALSE)
endif ()

if (NOT is_multiconfig AND NOT CMAKE_BUILD_TYPE)
  message (STATUS "Build type not specified - defaulting to Release")
  set (CMAKE_BUILD_TYPE Release CACHE STRING "build type" FORCE)
endif ()

# check for in-source build and fail
if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message (FATAL_ERROR "Builds (in-source) are not allowed in "
    "${CMAKE_CURRENT_SOURCE_DIR}. Please remove CMakeCache.txt and the CMakeFiles "
    "directory from ${CMAKE_CURRENT_SOURCE_DIR} and try building in a separate directory.")
endif ()

if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio" AND
    NOT ("${CMAKE_GENERATOR}" MATCHES .*Win64.*))
  message (FATAL_ERROR
    "Visual Studio 32-bit build is not supported. Use -G\"${CMAKE_GENERATOR} Win64\"")
endif ()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message (FATAL_ERROR "Divvyd requires a 64 bit target architecture.\n"
    "The most likely cause of this warning is trying to build divvyd with a 32-bit OS.")
endif ()

#[===================================================================[
   read version from source
#]===================================================================]

file (STRINGS src/divvy/protocol/impl/BuildInfo.cpp BUILD_INFO)
foreach (line_ ${BUILD_INFO})
  if (line_ MATCHES "versionString[ ]*=[ ]*\"(.+)\"")
    set (divvyd_version ${CMAKE_MATCH_1})
  endif ()
endforeach ()
if (divvyd_version)
  message (STATUS "divvyd version: ${divvyd_version}")
else ()
  message (FATAL_ERROR "unable to determine divvyd version")
endif ()

#[===================================================================[
   declare user options/settings
#]===================================================================]

option (assert "Enables asserts, even in release builds" OFF)
option (unity "Creates a build based on unity sources. This is the default" ON)
if (is_gcc OR is_clang)
  option (coverage "Generates coverage info." OFF)
  option (profile "Add profiling flags" OFF)
else ()
  set (profile OFF CACHE BOOL "gcc/clang only" FORCE)
  set (coverage OFF CACHE BOOL "gcc/clang only" FORCE)
endif ()
if (is_linux)
  option (BUILD_SHARED_LIBS "build shared divvy libraries" OFF)
  option (static "link protobuf, openssl, libc++, and boost statically" ON)
  option (perf "Enables flags that assist with perf recording" OFF)
  option (use_gold "enables detection of gold (binutils) linker" ON)
else ()
  # we are not ready to allow shared-libs on windows because it would require
  # export declarations. On macos it's more feasible, but static openssl
  # produces odd linker errors, thus we disable shared lib builds for now.
  set (BUILD_SHARED_LIBS OFF CACHE BOOL "build shared divvy libraries - OFF for win/macos" FORCE)
  set (static ON CACHE BOOL "static link, linux only. ON for WIN/macos" FORCE)
  set (perf OFF CACHE BOOL "perf flags, linux only" FORCE)
  set (use_gold OFF CACHE BOOL "gold linker, linux only" FORCE)
endif ()
option (jemalloc "Enables jemalloc for heap profiling" OFF)
option (werr "treat warnings as errors" OFF)

# this one is a string and therefore can't be an option
set (san "" CACHE STRING "On gcc & clang, add sanitizer instrumentation")
set_property (CACHE san PROPERTY STRINGS ";undefined;memory;address;thread")
if (san)
  string (TOLOWER ${san} san)
  set (SAN_FLAG "-fsanitize=${san}")
  set (SAN_LIB "")
  if (is_gcc)
    if (san STREQUAL "address")
      set (SAN_LIB "asan")
    elseif (san STREQUAL "thread")
      set (SAN_LIB "tsan")
    elseif (san STREQUAL "memory")
      set (SAN_LIB "msan")
    elseif (san STREQUAL "undefined")
      set (SAN_LIB "ubsan")
    endif ()
  endif ()
  set (_saved_CRL ${CMAKE_REQUIRED_LIBRARIES})
  set (CMAKE_REQUIRED_LIBRARIES "${SAN_FLAG};${SAN_LIB}")
  check_cxx_compiler_flag (${SAN_FLAG} COMPILER_SUPPORTS_SAN)
  set (CMAKE_REQUIRED_LIBRARIES ${_saved_CRL})
  if (NOT COMPILER_SUPPORTS_SAN)
    message (FATAL_ERROR "${san} sanitizer does not seem to be supported by your compiler")
  endif ()
endif ()

# the remaining options are obscure and rarely used
option (beast_no_unit_test_inline
  "Prevents unit test definitions from being inserted into global table"
  OFF)
option (beast_force_debug
  "Force BEAST_DEBUG regardless of DEBUG settings"
  OFF)
# NOTE - THIS OPTION CURRENTLY DOES NOT COMPILE :
# TODO: fix or remove
option (verify_nodeobject_keys
  "This verifies that the hash of node objects matches the payload. \
  This check is expensive - use with caution."
  OFF)
option (dump_leaks_on_exit
  "Displays heap blocks and counted objects which were not disposed of\
  during exit. Only implemented for windows builds."
  ON)
option (single_io_service_thread
  "Restricts the number of threads calling io_service::run to one. \
  This can be useful when debugging."
  OFF)
option (boost_show_deprecated
  "Allow boost to fail on deprecated usage. Only useful if you're trying\
  to find deprecated calls."
  OFF)
# beast_check_mem_leaks can't be an option() because we want to support an
# "undefined" which means use default behavior
# option (beast_check_mem_leaks
#   "Force beast mem leak checking. Default is on for DEBUG builds. Only implemented on WIN32"
if (WIN32)
  option (beast_disable_autolink "Disables autolinking of system libraries on WIN32" OFF)
else ()
  set (beast_disable_autolink OFF CACHE BOOL "WIN32 only" FORCE)
endif ()
if (coverage)
  message (STATUS "coverage build requested - forcing Debug build")
  set (CMAKE_BUILD_TYPE Debug)
endif ()

#[===================================================================[
   setup project-wide compiler settings
#]===================================================================]

#[=========================================================[
   TODO some/most of these common settings belong in a
   toolchain file, especially the ABI-impacting ones
#]=========================================================]
add_library (common INTERFACE)
add_library (Divvy::common ALIAS common)
# add a single global dependency on this interface lib
link_libraries (Divvy::common)
set_target_properties (common
  PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
target_compile_features (common INTERFACE cxx_std_14)
target_compile_definitions (common
  INTERFACE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
    $<$<AND:$<BOOL:profile>,$<NOT:$<BOOL:assert>>>:NDEBUG>)

if (MSVC)
  # remove existing exception flag since we set it to -EHa
  string (REGEX REPLACE "[-/]EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  # also remove dynamic runtime
  foreach (var_
      CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    string (REGEX REPLACE "[-/]MD[d]*" " " ${var_} "${${var_}}")
  endforeach ()

  target_compile_options (common
    INTERFACE
      -bigobj            # Increase object file max size
      -fp:precise        # Floating point behavior
      -Gd                # __cdecl calling convention
      -Gm-               # Minimal rebuild: disabled
      -Gy-               # Function level linking: disabled
      -MP                # Multiprocessor compilation
      -openmp-           # pragma omp: disabled
      -errorReport:none  # No error reporting to Internet
      -nologo            # Suppress login banner
      -wd4018            # Disable signed/unsigned comparison warnings
      -wd4244            # Disable float to int possible loss of data warnings
      -wd4267            # Disable size_t to T possible loss of data warnings
      -wd4800            # Disable C4800(int to bool performance)
      -wd4503            # Decorated name length exceeded, name was truncated
      $<$<COMPILE_LANGUAGE:CXX>:
        -EHa
        -GR
      >
      $<$<CONFIG:Release>:-Ox>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:
        -GS
        -Zc:forScope
      >
      # static runtime
      $<$<CONFIG:Debug>:-MTd>
      $<$<NOT:$<CONFIG:Debug>>:-MT>
      $<$<BOOL:${werr}>:-WX>
      )
  target_compile_definitions (common
    INTERFACE
      _WIN32_WINNT=0x6000
      _SCL_SECURE_NO_WARNINGS
      _CRT_SECURE_NO_WARNINGS
      WIN32_CONSOLE
      NOMINMAX
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:_CRTDBG_MAP_ALLOC>)
  target_link_libraries (common
    INTERFACE
      -errorreport:none
      -machine:X64)
else ()
  # HACK : because these need to come first, before any warning demotion
  string (APPEND CMAKE_CXX_FLAGS " -Wall -Wdeprecated")
  # not MSVC
  target_compile_options (common
    INTERFACE
      $<$<BOOL:${werr}>:-Werror>
      $<$<COMPILE_LANGUAGE:CXX>:
        -frtti
        -Wnon-virtual-dtor
      >
      -Wno-sign-compare
      -Wno-char-subscripts
      -Wno-format
      -Wno-unused-local-typedefs
      $<$<BOOL:${is_gcc}>:
        -Wno-unused-but-set-variable
        -Wno-deprecated
      >
      $<$<NOT:$<CONFIG:Debug>>:-fno-strict-aliasing>
      # tweak gcc optimization for debug
      $<$<AND:$<BOOL:is_gcc>,$<CONFIG:Debug>>:-O0>
      # Add debug symbols to release config
      $<$<CONFIG:Release>:-g>)
  target_link_libraries (common
    INTERFACE
      -rdynamic
      $<$<AND:$<BOOL:${static}>,$<NOT:$<BOOL:${APPLE}>>>:-static-libstdc++>)
endif ()

if (use_gold AND is_gcc)
  # use gold linker if available
  execute_process (
    COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=gold -Wl,--version
    ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    #[=========================================================[
       NOTE: THE gold linker inserts -rpath as DT_RUNPATH by
       default intead of DT_RPATH, so you might have slightly
       unexpected runtime ld behavior if you were expecting
       DT_RPATH.  Specify --disable-new-dtags to gold if you do
       not want the default DT_RUNPATH behavior. This rpath
       treatment as well as static/dynamic selection means that
       gold does not currently have ideal default behavior when
       we are using jemalloc. Thus for simplicity we don't use
       it when jemalloc is requested. An alternative to
       disabling would be to figure out all the settings
       required to make gold play nicely with jemalloc.
    #]=========================================================]
  if (("${LD_VERSION}" MATCHES "GNU gold") AND (NOT jemalloc))
    target_link_libraries (common
      INTERFACE
        -fuse-ld=gold
        -Wl,--no-as-needed
        #[=========================================================[
           see https://bugs.launchpad.net/ubuntu/+source/eglibc/+bug/1253638/comments/5
           DT_RUNPATH does not work great for transitive
           dependencies (of which boost has a few) - so just
           switch to DT_RPATH if doing dynamic linking with gold
        #]=========================================================]
        $<$<NOT:$<BOOL:${static}>>:-Wl,--disable-new-dtags>)
  endif ()
  unset (LD_VERSION)
endif ()

if (is_clang)
  # use lld linker if available
  execute_process (
    COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=lld -Wl,--version
    ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
  if ("${LD_VERSION}" MATCHES "LLD")
    target_link_libraries (common INTERFACE -fuse-ld=lld)
  endif ()
  unset (LD_VERSION)
endif()

if (assert)
  foreach (var_
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    STRING (REGEX REPLACE "[-/]DNDEBUG" "" ${var_} "${${var_}}")
  endforeach ()
endif ()

#[===================================================================[
   divvyd compile options/settings via an interface library
#]===================================================================]

add_library (opts INTERFACE)
add_library (Divvy::opts ALIAS opts)
target_compile_definitions (opts
  INTERFACE
    BOOST_NO_AUTO_PTR
    BOOST_ASIO_HAS_STD_ARRAY
    BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS
    $<$<BOOL:${boost_show_deprecated}>:
      BOOST_ASIO_NO_DEPRECATED
      BOOST_FILESYSTEM_NO_DEPRECATED
    >
    $<$<NOT:$<BOOL:${boost_show_deprecated}>>:
      BOOST_COROUTINES_NO_DEPRECATION_WARNING
      BOOST_BEAST_ALLOW_DEPRECATED
      BOOST_FILESYSTEM_DEPRECATED
    >
    BEAST_CHECK_MEMORY_LEAKS=$<BOOL:${beast_check_mem_leaks}>
    DIVVY_DUMP_LEAKS_ON_EXIT=$<BOOL:${dump_leaks_on_exit}>
    $<$<BOOL:${beast_no_unit_test_inline}>:BEAST_NO_UNIT_TEST_INLINE=1>
    $<$<BOOL:${beast_force_debug}>:BEAST_FORCE_DEBUG=1>
    $<$<BOOL:${beast_disable_autolink}>:BEAST_DONT_AUTOLINK_TO_WIN32_LIBRARIES=1>
    $<$<BOOL:${single_io_service_thread}>:DIVVY_SINGLE_IO_SERVICE_THREAD=1>
    # doesn't currently compile ? :
    $<$<BOOL:${verify_nodeobject_keys}>:DIVVY_VERIFY_NODEOBJECT_KEYS=1>)
target_compile_options (opts
  INTERFACE
    $<$<AND:$<BOOL:${is_gcc}>,$<COMPILE_LANGUAGE:CXX>>:-Wsuggest-override>
    $<$<BOOL:${perf}>:-fno-omit-frame-pointer>
    $<$<BOOL:${coverage}>:-fprofile-arcs -ftest-coverage>
    $<$<BOOL:${profile}>:-pg>
    $<$<AND:$<BOOL:${is_gcc}>,$<BOOL:${profile}>>:-p>)

target_link_libraries (opts
  INTERFACE
    $<$<BOOL:${coverage}>:-fprofile-arcs -ftest-coverage>
    $<$<BOOL:${profile}>:-pg>
    $<$<AND:$<BOOL:${is_gcc}>,$<BOOL:${profile}>>:-p>)

if (jemalloc)
  find_package (jemalloc REQUIRED)
  target_compile_definitions (opts INTERFACE PROFILE_JEMALLOC)
  target_include_directories (opts SYSTEM INTERFACE ${JEMALLOC_INCLUDE_DIRS})
  target_link_libraries (opts INTERFACE ${JEMALLOC_LIBRARIES})
  get_filename_component (JEMALLOC_LIB_PATH ${JEMALLOC_LIBRARIES} DIRECTORY)
  ## TODO see if we can use the BUILD_RPATH target property (is it transitive?)
  set (CMAKE_BUILD_RPATH ${CMAKE_BUILD_RPATH} ${JEMALLOC_LIB_PATH})
endif ()

if (san)
  target_compile_options (opts
    INTERFACE
      ${SAN_FLAG}
      -fno-omit-frame-pointer)
  target_compile_definitions (opts
    INTERFACE
      $<$<STREQUAL:${san},address>:SANITIZER=ASAN>
      $<$<STREQUAL:${san},thread>:SANITIZER=TSAN>
      $<$<STREQUAL:${san},memory>:SANITIZER=MSAN>
      $<$<STREQUAL:${san},undefined>:SANITIZER=UBSAN>)
  target_link_libraries (opts INTERFACE ${SAN_FLAG} ${SAN_LIB})
endif ()

#[===================================================================[
   divvyd transitive library deps via an interface library
#]===================================================================]

add_library (divvy_syslibs INTERFACE)
add_library (Divvy::syslibs ALIAS divvy_syslibs)
target_link_libraries (divvy_syslibs
  INTERFACE
    $<$<BOOL:${MSVC}>:
      legacy_stdio_definitions.lib
      Shlwapi
      kernel32
      user32
      gdi32
      winspool
      comdlg32
      advapi32
      shell32
      ole32
      oleaut32
      uuid
      odbc32
      odbccp32
      crypt32
    >
    $<$<NOT:$<BOOL:${MSVC}>>:dl>
    $<$<NOT:$<OR:$<BOOL:${MSVC}>,$<BOOL:${APPLE}>>>:rt>)

if (NOT MSVC)
  set (THREADS_PREFER_PTHREAD_FLAG ON)
  find_package (Threads)
  target_link_libraries (divvy_syslibs INTERFACE Threads::Threads)
endif ()

add_library (divvy_libs INTERFACE)
add_library (Divvy::libs ALIAS divvy_libs)
target_link_libraries (divvy_libs INTERFACE Divvy::syslibs)

#[===================================================================[
   NIH dep: boost
#]===================================================================]

if ((NOT DEFINED BOOST_ROOT) AND (DEFINED ENV{BOOST_ROOT}))
  set (BOOST_ROOT $ENV{BOOST_ROOT})
endif ()
file (TO_CMAKE_PATH "${BOOST_ROOT}" BOOST_ROOT)
if (WIN32 OR CYGWIN)
  # Workaround for MSVC having two boost versions - x86 and x64 on same PC in stage folders
  if (DEFINED BOOST_ROOT)
    if (IS_DIRECTORY ${BOOST_ROOT}/stage64/lib)
      set (BOOST_LIBRARYDIR ${BOOST_ROOT}/stage64/lib)
    else ()
      set (BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib)
    endif ()
  endif ()
endif ()
message (STATUS "BOOST_ROOT: ${BOOST_ROOT}")
message (STATUS "BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}")

# uncomment the following as needed to debug FindBoost issues:
#set (Boost_DEBUG ON)

#[=========================================================[
   boost dynamic libraries don't trivially support @rpath
   linking right now (cmake's default), so just force
   static linking for macos, or if requested on linux by flag
#]=========================================================]
if (static)
  set (Boost_USE_STATIC_LIBS ON)
endif ()
set (Boost_USE_MULTITHREADED ON)
if (static AND NOT APPLE)
  set (Boost_USE_STATIC_RUNTIME ON)
else ()
  set (Boost_USE_STATIC_RUNTIME OFF)
endif ()
find_package (Boost 1.67 REQUIRED
  COMPONENTS
    chrono
    context
    coroutine
    date_time
    filesystem
    program_options
    regex
    serialization
    system
    thread)

add_library (divvy_boost INTERFACE)
add_library (Divvy::boost ALIAS divvy_boost)
if (is_xcode)
  target_include_directories (divvy_boost BEFORE INTERFACE ${Boost_INCLUDE_DIRS})
  target_compile_options (divvy_boost INTERFACE --system-header-prefix="boost/")
else ()
  target_include_directories (divvy_boost SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries (divvy_boost
  INTERFACE
    Boost::boost
    Boost::chrono
    Boost::coroutine
    Boost::date_time
    Boost::filesystem
    Boost::program_options
    Boost::regex
    Boost::serialization
    Boost::system
    Boost::thread)

#[===================================================================[
   NIH dep: openssl
#]===================================================================]

if (APPLE AND NOT DEFINED ENV{OPENSSL_ROOT_DIR})
  find_program (HOMEBREW brew)
  if (NOT HOMEBREW STREQUAL "HOMEBREW-NOTFOUND")
    execute_process (COMMAND ${HOMEBREW} --prefix openssl
      OUTPUT_VARIABLE OPENSSL_ROOT_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif ()
endif ()

if ((NOT DEFINED OPENSSL_ROOT) AND (DEFINED ENV{OPENSSL_ROOT}))
  set (OPENSSL_ROOT $ENV{OPENSSL_ROOT})
endif ()
file (TO_CMAKE_PATH "${OPENSSL_ROOT}" OPENSSL_ROOT)

if (static)
  set (OPENSSL_USE_STATIC_LIBS ON)
endif ()
set (OPENSSL_MSVC_STATIC_RT ON)
set (_ssl_min_ver 1.0.2)
# HACK for travis
if ("$ENV{CI}" STREQUAL "true" AND "$ENV{TRAVIS}" STREQUAL "true")
  set (_ssl_min_ver 1.0.1)
endif ()
find_package (OpenSSL ${_ssl_min_ver} REQUIRED)
target_link_libraries (divvy_libs
  INTERFACE
    OpenSSL::SSL
    OpenSSL::Crypto)
# disable SSLv2...this can also be done when building/configuring OpenSSL
set_target_properties(OpenSSL::SSL PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS OPENSSL_NO_SSL2)
#[=========================================================[
   https://gitlab.kitware.com/cmake/cmake/issues/16885
   depending on how openssl is built, it might depend
   on zlib. In fact, the openssl find package should
   figure this out for us, but it does not currently...
   so let's add zlib ourselves to the lib list
   TODO: investigate linking to static zlib for static
   build option
#]=========================================================]
find_package (ZLIB)
if (TARGET ZLIB::ZLIB)
  #target_link_libraries (OpenSSL::Crypto INTERFACE ZLIB::ZLIB)
  set_target_properties(OpenSSL::Crypto PROPERTIES
    INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
endif ()

#[===================================================================[
   NIH dep: secp256k1
#]===================================================================]

add_library (secp256k1 STATIC
  src/secp256k1/src/secp256k1.c)
target_compile_definitions (secp256k1
  PRIVATE
    USE_NUM_NONE
    USE_FIELD_10X26
    USE_FIELD_INV_BUILTIN
    USE_SCALAR_8X32
    USE_SCALAR_INV_BUILTIN)
target_include_directories (secp256k1
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1)
target_compile_options (secp256k1
  PRIVATE
    $<$<BOOL:${MSVC}>:-wd4319>
    $<$<NOT:$<BOOL:${MSVC}>>:
      -Wno-deprecated-declarations
      -Wno-unused-function
    >
    $<$<BOOL:${is_gcc}>:-Wno-nonnull-compare>)
add_library (NIH::secp256k1 ALIAS secp256k1)
target_link_libraries (divvy_libs INTERFACE NIH::secp256k1)
#[===========================[
   headers installation
#]===========================]
install (
  FILES
    src/secp256k1/include/secp256k1.h
  DESTINATION include/secp256k1/include)

#[===================================================================[
   NIH dep: ed25519-donna
#]===================================================================]

add_library (ed25519-donna STATIC
  src/ed25519-donna/ed25519.c)
target_include_directories (ed25519-donna
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ed25519-donna)
#[=========================================================[
   NOTE for macos:
   https://github.com/floodyberry/ed25519-donna/issues/29
   our source for ed25519-donna-portable.h has been
   patched to workaround this.
#]=========================================================]
target_link_libraries (ed25519-donna PUBLIC OpenSSL::SSL)
add_library (NIH::ed25519-donna ALIAS ed25519-donna)
target_link_libraries (divvy_libs INTERFACE NIH::ed25519-donna)
#[===========================[
   headers installation
#]===========================]
install (
  FILES
    src/ed25519-donna/ed25519.h
  DESTINATION include/ed25519-donna)

#[===================================================================[
   NIH dep: lz4
#]===================================================================]

add_library (lz4 STATIC
  src/lz4/lib/lz4.c
  src/lz4/lib/lz4hc.c
  src/lz4/lib/lz4frame.c
  src/lz4/lib/xxhash.c)
target_compile_definitions (lz4
  PRIVATE XXH_NAMESPACE=LZ4_)
add_library (NIH::lz4 ALIAS lz4)
target_link_libraries (divvy_libs INTERFACE NIH::lz4)
## pseudo-install our files so that dependent builds can find them
file (MAKE_DIRECTORY
    ${CMAKE_BINARY_DIR}/lz4/include
    ${CMAKE_BINARY_DIR}/lz4/lib)
target_include_directories (lz4 PUBLIC ${CMAKE_BINARY_DIR}/lz4/include)
add_custom_command (TARGET lz4 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/lib/lz4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/lib/lz4frame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/lib/lz4hc.h
    ${CMAKE_BINARY_DIR}/lz4/include
  COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:lz4> ${CMAKE_BINARY_DIR}/lz4/lib)
exclude_if_included (lz4)

#[===================================================================[
   NIH dep: libarchive (via external project)
#]===================================================================]
set (la_root_suffix)
if (MSVC)
  set(la_root_suffix "_static")
endif ()
string(REGEX REPLACE "[ \\/%]+" "_" gen_for_path ${CMAKE_GENERATOR})
string(TOLOWER ${gen_for_path} gen_for_path)
# hack: trying to shorten paths for windows CI (hits 260 MAXPATH easily)
string(REPLACE "visual_studio" "vs" gen_for_path ${gen_for_path})
ExternalProject_Add (libarchive
  PREFIX ${CMAKE_SOURCE_DIR}/.nih_c/${gen_for_path}/${CMAKE_CXX_COMPILER_ID}_${CMAKE_CXX_COMPILER_VERSION}
  # TODO: switch back to official repo once they allow ENABLE_WERROR option in
  # mainline -- see https://github.com/libarchive/libarchive/pull/1033
  #GIT_REPOSITORY https://github.com/libarchive/libarchive.git
  #GIT_TAG v3.3.1
  GIT_REPOSITORY https://github.com/mellery451/libarchive.git
  GIT_TAG e78e48ea4e102fef7f379bc8f10afbfbf25633a6
  CMAKE_ARGS
    # passing the compiler seems to be needed for windows CI, sadly
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:-DCMAKE_VERBOSE_MAKEFILE=ON>
    -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/lz4
    -DCMAKE_DEBUG_POSTFIX=_d
    -DCMAKE_BUILD_TYPE=$<IF:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>,Debug,Release>
    -DENABLE_LZ4=ON
    -ULZ4_*
    -DENABLE_WERROR=OFF
    -DENABLE_TAR=OFF
    -DENABLE_TAR_SHARED=OFF
    -DENABLE_INSTALL=ON
    -DENABLE_NETTLE=OFF
    -DENABLE_OPENSSL=OFF
    -DENABLE_LZO=OFF
    -DENABLE_LZMA=OFF
    -DENABLE_ZLIB=OFF
    -DENABLE_BZip2=OFF
    -DENABLE_LIBXML2=OFF
    -DENABLE_EXPAT=OFF
    -DENABLE_PCREPOSIX=OFF
    -DENABLE_LibGCC=OFF
    -DENABLE_CNG=OFF
    -DENABLE_CPIO=OFF
    -DENABLE_CPIO_SHARED=OFF
    -DENABLE_CAT=OFF
    -DENABLE_CAT_SHARED=OFF
    -DENABLE_XATTR=OFF
    -DENABLE_ACL=OFF
    -DENABLE_ICONV=OFF
    -DENABLE_TEST=OFF
    -DENABLE_COVERAGE=OFF
    $<$<BOOL:${MSVC}>:
      "-DCMAKE_C_FLAGS=-GR -Gd -fp:precise -FS -MP"
      "-DCMAKE_C_FLAGS_DEBUG=-MTd"
      "-DCMAKE_C_FLAGS_RELEASE=-MT"
      "-DCMAKE_C_FLAGS_RELWITHDEBINFO=-MT"
      "-DCMAKE_C_FLAGS_MINSIZEREL=-MT"
    >
  LOG_BUILD ON
  LOG_CONFIGURE ON
  BUILD_COMMAND
    ${CMAKE_COMMAND}
    --build .
    --config $<CONFIG>
    --target archive_static
    $<$<BOOL:${is_multiconfig}>:
      COMMAND
        ${CMAKE_COMMAND} -E copy
       <BINARY_DIR>/libarchive/$<CONFIG>/${CMAKE_STATIC_LIBRARY_PREFIX}archive${la_root_suffix}$<$<OR:$<CONFIG:Debug>,$<CONFIG:DebugClassic>>:_d>${CMAKE_STATIC_LIBRARY_SUFFIX}
        <BINARY_DIR>/libarchive
      >
  TEST_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS lz4
  BUILD_BYPRODUCTS
    <BINARY_DIR>/libarchive/${CMAKE_STATIC_LIBRARY_PREFIX}archive${la_root_suffix}${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/libarchive/${CMAKE_STATIC_LIBRARY_PREFIX}archive${la_root_suffix}_d${CMAKE_STATIC_LIBRARY_SUFFIX}
)
ExternalProject_Get_Property (libarchive BINARY_DIR)
ExternalProject_Get_Property (libarchive SOURCE_DIR)
add_library (archive_lib STATIC IMPORTED GLOBAL)
file (MAKE_DIRECTORY ${SOURCE_DIR}/libarchive)
set_target_properties (archive_lib PROPERTIES
  IMPORTED_LOCATION_DEBUG
    ${BINARY_DIR}/libarchive/${CMAKE_STATIC_LIBRARY_PREFIX}archive${la_root_suffix}_d${CMAKE_STATIC_LIBRARY_SUFFIX}
  IMPORTED_LOCATION_RELEASE
    ${BINARY_DIR}/libarchive/${CMAKE_STATIC_LIBRARY_PREFIX}archive${la_root_suffix}${CMAKE_STATIC_LIBRARY_SUFFIX}
  INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/libarchive
  INTERFACE_COMPILE_DEFINITIONS LIBARCHIVE_STATIC)
add_dependencies (archive_lib libarchive)
target_link_libraries (archive_lib INTERFACE NIH::lz4)
target_link_libraries (divvy_libs INTERFACE archive_lib)

#[===================================================================[
   NIH dep: sqlite
#]===================================================================]

add_library (sqlite3 STATIC
  src/sqlite/sqlite/sqlite3.c)
#[=========================================================[
   When compiled with SQLITE_THREADSAFE=1, SQLite operates
   in serialized mode. In this mode, SQLite can be safely
   used by multiple threads with no restriction.

   VFALCO NOTE: This implies a global mutex!

   When compiled with SQLITE_THREADSAFE=2, SQLite can be
   used in a multithreaded program so long as no two
   threads attempt to use the same database connection at
   the same time.

   VFALCO NOTE: This is the preferred threading model.
#]=========================================================]
target_compile_definitions (sqlite3
  PRIVATE
    SQLITE_THREADSAFE=1
    HAVE_USLEEP=1)
target_compile_options (sqlite3
  PRIVATE
    $<$<BOOL:${MSVC}>:
      -wd4100
      -wd4127
      -wd4232
      -wd4244
      -wd4701
      -wd4706
      -wd4996
    >
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-array-bounds>)
add_library (NIH::sqlite3 ALIAS sqlite3)
target_link_libraries (sqlite3 PUBLIC $<$<NOT:$<BOOL:${MSVC}>>:dl>)
target_link_libraries (divvy_libs INTERFACE NIH::sqlite3)
file (MAKE_DIRECTORY
    ${CMAKE_BINARY_DIR}/sqlite3/include
    ${CMAKE_BINARY_DIR}/sqlite3/lib)
target_include_directories (sqlite3
  PRIVATE
    src/sqlite
    src/sqlite/sqlite
  INTERFACE
    ${CMAKE_BINARY_DIR}/sqlite3/include)
add_custom_command (TARGET sqlite3 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite/sqlite ${CMAKE_BINARY_DIR}/sqlite3/include
  COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:sqlite3> ${CMAKE_BINARY_DIR}/sqlite3/lib
  BYPRODUCTS ${CMAKE_BINARY_DIR}/sqlite3/include/sqlite3.h)
exclude_if_included (sqlite3)

#[===================================================================[
   NIH dep: soci
#]===================================================================]

add_library (soci STATIC
  #[=========================================================[
     this unity file is our interpretation of
     what sources constitute soci-lib.
  #]=========================================================]
  src/divvy/unity/soci.cpp
  # this header is here to create dependency on the sqlite post-build step
  ${CMAKE_BINARY_DIR}/sqlite3/include/sqlite3.h)
target_compile_definitions (soci PUBLIC SOCI_HAVE_CXX_C11=1)
target_include_directories (soci
  PUBLIC
    src/soci/src
    src/soci/include
    #[=========================================================[
       HACK for ninja..which doesn't properly see
       the POST_BUILD dependency on sqlite (TODO: diagnose)
       so we must include the original source dir
    #]=========================================================]
    $<$<STREQUAL:${CMAKE_GENERATOR},Ninja>:
      ${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite
    >
  PRIVATE
    src
    src/soci/src/core
    src/soci/include/private)
target_link_libraries (soci
  NIH::sqlite3
  Divvy::boost)
add_library (NIH::soci ALIAS soci)
target_link_libraries (divvy_libs INTERFACE NIH::soci)
exclude_if_included (soci)

#[===================================================================[
   NIH dep: snappy
#]===================================================================]

add_library (snappy STATIC
  src/snappy/snappy/snappy.cc
  src/snappy/snappy/snappy-sinksource.cc
  src/snappy/snappy/snappy-stubs-internal.cc)
target_compile_options (snappy
  PRIVATE
    $<$<NOT:$<BOOL:${MSVC}>>:
      -Wno-deprecated
      -Wno-unused-function
    >
    $<$<BOOL:${is_gcc}>:-Wno-nonnull-compare>)
target_include_directories (snappy
  PUBLIC
    src/snappy/snappy
    src/snappy/config)
add_library (NIH::snappy ALIAS snappy)
target_link_libraries (divvy_libs INTERFACE NIH::snappy)
exclude_if_included (snappy)

#[===================================================================[
   NIH dep: rocksdb
#]===================================================================]

if (NOT MSVC)
  add_library (rocksdb STATIC
    #[=========================================================[
       this unity file is our interpretation of what sources
       constitute rocksdb
    #]=========================================================]
    src/divvy/unity/rocksdb.cpp)
  target_compile_options (rocksdb
    PRIVATE
      $<$<BOOL:${is_gcc}>:-w>
    PUBLIC
      $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:CMAKE_CXX_COMPILER_VERSION,7>>:
        -Wno-inconsistent-missing-override
        -Wno-uninitialized
      >
      $<$<BOOL:${is_clang}>:--system-header-prefix=rocksdb2>)
  target_include_directories (rocksdb
    PRIVATE
      src
      src/rocksdb2
    PUBLIC
      src/rocksdb2/include)
  target_compile_definitions (rocksdb
    PUBLIC
      SNAPPY
      DIVVY_ROCKSDB_AVAILABLE=1
      $<$<BOOL:${WIN32}>:ROCKSDB_PLATFORM_WINDOWS>
      $<$<NOT:$<BOOL:${WIN32}>>:
        ROCKSDB_PLATFORM_POSIX
        ROCKSDB_LIB_IO_POSIX
      >
      $<$<BOOL:${APPLE}>:OS_MACOSX=1>
      $<$<PLATFORM_ID:Linux>:OS_LINUX>
    )
  target_link_libraries (rocksdb NIH::snappy)
else ()
  add_library (rocksdb INTERFACE)
  target_compile_definitions (rocksdb INTERFACE DIVVY_ROCKSDB_AVAILABLE=0)
endif ()
add_library (NIH::rocksdb ALIAS rocksdb)
target_link_libraries (divvy_libs INTERFACE NIH::rocksdb)
exclude_if_included (rocksdb)

#[===================================================================[
   NIH dep: nudb

   NuDB is header-only, thus is an INTERFACE lib in CMake.
   TODO: move this into NuDB repo, add proper targets and
   export/install
#]===================================================================]

add_library (nudb INTERFACE)
target_include_directories (nudb INTERFACE src/nudb/include)
target_link_libraries (nudb
  INTERFACE
    Boost::thread
    Boost::system)
add_library (NIH::nudb ALIAS nudb)
target_link_libraries (divvy_libs INTERFACE NIH::nudb)

#[===================================================================[
   import protobuf (lib and compiler) and create a lib
   from our proto message definitions. There is fallback
   logic for windows that builds some version of the protobuf
   core library from source and downloads the protoc compiler
   if needed
#]===================================================================]

if (static)
  set (Protobuf_USE_STATIC_LIBS ON)
endif ()
find_package (Protobuf)
if (MSVC AND NOT TARGET protobuf::protoc)
  find_program (my_protoc NAMES protoc)
  if (NOT my_protoc)
    message (WARNING "No protoc found -- downloading now")
    file (DOWNLOAD
      "https://divvy.github.io/Downloads/protoc/2.5.1/protoc.exe"
      ${CMAKE_BINARY_DIR}/tools/protoc.exe
      STATUS status
      TIMEOUT 30)
    list (GET status 0 status_code)
    list (GET status 1 status_string)
    if (NOT status_code EQUAL 0)
      message (FATAL_ERROR "Failed to download protoc: ${status_string}")
    endif ()
    set (my_protoc ${CMAKE_BINARY_DIR}/tools/protoc.exe)
  endif ()
  add_executable (protobuf::protoc IMPORTED)
  set_target_properties (protobuf::protoc PROPERTIES
    IMPORTED_LOCATION "${my_protoc}")
endif ()

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/proto_gen)
set (save_CBD ${CMAKE_CURRENT_BINARY_DIR})
set (CMAKE_CURRENT_BINARY_DIR ${CMAKE_BINARY_DIR}/proto_gen)
protobuf_generate_cpp (
  PROTO_SRCS
  PROTO_HDRS
  src/divvy/proto/divvy.proto)
set (CMAKE_CURRENT_BINARY_DIR ${save_CBD})

add_library (pbufs STATIC ${PROTO_SRCS} ${PROTO_HDRS})
if ((NOT TARGET protobuf::libprotobuf) AND (NOT Protobuf_LIBRARIES))
  message (STATUS "protobuf lib not found - adding unity source for lib")
  target_sources (pbufs PRIVATE src/divvy/unity/protobuf.cpp)
  target_include_directories (pbufs
    PUBLIC
      src/protobuf/src
      src/protobuf/vsprojects
      src/divvy/proto)
  target_compile_definitions (pbufs
    PRIVATE
      _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
endif ()
target_include_directories (pbufs PRIVATE src)
target_include_directories (pbufs
  SYSTEM PUBLIC
    ${CMAKE_BINARY_DIR}/proto_gen)
if (NOT MSVC)
  if (TARGET protobuf::libprotobuf)
    target_link_libraries (pbufs protobuf::libprotobuf)
  else ()
    target_link_libraries (pbufs ${Protobuf_LIBRARIES})
    target_include_directories (pbufs PUBLIC ${Protobuf_INCLUDE_DIR})
  endif ()
endif ()
target_compile_options (pbufs
  PUBLIC
    $<$<BOOL:${is_xcode}>:
      --system-header-prefix="google/protobuf"
      -Wno-deprecated-dynamic-exception-spec
    >
    $<$<BOOL:${MSVC}>:
      -wd4251
      -wd4146
    >
  PRIVATE
    $<$<BOOL:${MSVC}>:
      -wd4018
      -wd4244
      -wd4800
      -wd4996
    >)
add_library (Divvy::pbufs ALIAS pbufs)
target_link_libraries (divvy_libs INTERFACE Divvy::pbufs)
exclude_if_included (pbufs)

#[===================================================================[
   xdvl_core
   core functionality, useable by some client software perhaps
#]===================================================================]

file (GLOB_RECURSE rb_headers
  src/divvy/beast/*.h
  src/divvy/beast/*.hpp)

add_library (xdvl_core
  ${rb_headers} ## headers added here for benefit of IDEs
  #[===============================[
      beast/legacy FILES:
      TODO: review these sources for removal or replacement
  #]===============================]
  $<$<BOOL:${unity}>:
    src/divvy/beast/core/core.unity.cpp
    src/divvy/beast/unity/beast_hash_unity.cpp
    src/divvy/beast/unity/beast_insight_unity.cpp
    src/divvy/beast/unity/beast_net_unity.cpp
    src/divvy/beast/unity/beast_utility_unity.cpp
  >
  $<$<NOT:$<BOOL:${unity}>>:
    src/divvy/beast/core/CurrentThreadName.cpp
    src/divvy/beast/core/SemanticVersion.cpp
    src/divvy/beast/core/WaitableEvent.cpp
    src/divvy/beast/hash/impl/xxhash.cpp
    src/divvy/beast/insight/impl/Collector.cpp
    src/divvy/beast/insight/impl/Groups.cpp
    src/divvy/beast/insight/impl/Hook.cpp
    src/divvy/beast/insight/impl/Metric.cpp
    src/divvy/beast/insight/impl/NullCollector.cpp
    src/divvy/beast/insight/impl/StatsDCollector.cpp
    src/divvy/beast/net/impl/IPAddressConversion.cpp
    src/divvy/beast/net/impl/IPAddressV4.cpp
    src/divvy/beast/net/impl/IPAddressV6.cpp
    src/divvy/beast/net/impl/IPEndpoint.cpp
    src/divvy/beast/utility/src/beast_Debug.cpp
    src/divvy/beast/utility/src/beast_Journal.cpp
    src/divvy/beast/utility/src/beast_PropertyStream.cpp
  >
  # END beast/legacy

  #[===============================[
     core sources
  #]===============================]
  $<$<BOOL:${unity}>:
    src/divvy/unity/basics1.cpp
    src/divvy/unity/json.cpp
    src/divvy/unity/protocol.cpp
    src/divvy/unity/crypto.cpp
  >
  $<$<NOT:$<BOOL:${unity}>>:
    #[===============================[
       nounity, main sources:
         subdir: basics (partial)
    #]===============================]
    src/divvy/basics/impl/base64.cpp
    src/divvy/basics/impl/contract.cpp
    src/divvy/basics/impl/CountedObject.cpp
    src/divvy/basics/impl/Log.cpp
    src/divvy/basics/impl/strHex.cpp
    src/divvy/basics/impl/StringUtilities.cpp
    #[===============================[
       nounity, main sources:
         subdir: json
    #]===============================]
    src/divvy/json/impl/JsonPropertyStream.cpp
    src/divvy/json/impl/Object.cpp
    src/divvy/json/impl/Output.cpp
    src/divvy/json/impl/Writer.cpp
    src/divvy/json/impl/json_reader.cpp
    src/divvy/json/impl/json_value.cpp
    src/divvy/json/impl/json_valueiterator.cpp
    src/divvy/json/impl/json_writer.cpp
    src/divvy/json/impl/to_string.cpp
    #[===============================[
       nounity, main sources:
         subdir: protocol
    #]===============================]
    src/divvy/protocol/impl/AccountID.cpp
    src/divvy/protocol/impl/Book.cpp
    src/divvy/protocol/impl/BuildInfo.cpp
    src/divvy/protocol/impl/ErrorCodes.cpp
    src/divvy/protocol/impl/Feature.cpp
    src/divvy/protocol/impl/HashPrefix.cpp
    src/divvy/protocol/impl/IOUAmount.cpp
    src/divvy/protocol/impl/Indexes.cpp
    src/divvy/protocol/impl/InnerObjectFormats.cpp
    src/divvy/protocol/impl/Issue.cpp
    src/divvy/protocol/impl/Keylet.cpp
    src/divvy/protocol/impl/LedgerFormats.cpp
    src/divvy/protocol/impl/PublicKey.cpp
    src/divvy/protocol/impl/Quality.cpp
    src/divvy/protocol/impl/Rate2.cpp
    src/divvy/protocol/impl/SField.cpp
    src/divvy/protocol/impl/SOTemplate.cpp
    src/divvy/protocol/impl/STAccount.cpp
    src/divvy/protocol/impl/STAmount.cpp
    src/divvy/protocol/impl/STArray.cpp
    src/divvy/protocol/impl/STBase.cpp
    src/divvy/protocol/impl/STBlob.cpp
    src/divvy/protocol/impl/STInteger.cpp
    src/divvy/protocol/impl/STLedgerEntry.cpp
    src/divvy/protocol/impl/STObject.cpp
    src/divvy/protocol/impl/STParsedJSON.cpp
    src/divvy/protocol/impl/STPathSet.cpp
    src/divvy/protocol/impl/STTx.cpp
    src/divvy/protocol/impl/STValidation.cpp
    src/divvy/protocol/impl/STVar.cpp
    src/divvy/protocol/impl/STVector256.cpp
    src/divvy/protocol/impl/SecretKey.cpp
    src/divvy/protocol/impl/Seed.cpp
    src/divvy/protocol/impl/Serializer.cpp
    src/divvy/protocol/impl/Sign.cpp
    src/divvy/protocol/impl/TER.cpp
    src/divvy/protocol/impl/TxFormats.cpp
    src/divvy/protocol/impl/UintTypes.cpp
    src/divvy/protocol/impl/digest.cpp
    src/divvy/protocol/impl/tokens.cpp
    #[===============================[
       nounity, main sources:
         subdir: crypto
    #]===============================]
    src/divvy/crypto/impl/GenerateDeterministicKey.cpp
    src/divvy/crypto/impl/KeyType.cpp
    src/divvy/crypto/impl/RFC1751.cpp
    src/divvy/crypto/impl/csprng.cpp
    src/divvy/crypto/impl/ec_key.cpp
    src/divvy/crypto/impl/openssl.cpp
  >)
add_library (Divvy::xdvl_core ALIAS xdvl_core)
target_include_directories (xdvl_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/divvy>
    # this one is for beast/legacy files:
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/beast/extras>
    $<INSTALL_INTERFACE:include>)
target_compile_options (xdvl_core
  PUBLIC
    $<$<BOOL:${is_gcc}>:-Wno-maybe-uninitialized>)
target_link_libraries (xdvl_core
  PUBLIC
    OpenSSL::Crypto
    NIH::secp256k1
    NIH::ed25519-donna
    Divvy::syslibs
    Divvy::boost
    Divvy::opts)
#[=================================[
   main/core headers installation
#]=================================]
install (
  FILES
    src/divvy/basics/base64.h
    src/divvy/basics/Blob.h
    src/divvy/basics/Buffer.h
    src/divvy/basics/CountedObject.h
    src/divvy/basics/LocalValue.h
    src/divvy/basics/Log.h
    src/divvy/basics/Slice.h
    src/divvy/basics/StringUtilities.h
    src/divvy/basics/ToString.h
    src/divvy/basics/UnorderedContainers.h
    src/divvy/basics/base_uint.h
    src/divvy/basics/chrono.h
    src/divvy/basics/contract.h
    src/divvy/basics/date.h
    src/divvy/basics/hardened_hash.h
    src/divvy/basics/strHex.h
  DESTINATION include/divvy/basics)
install (
  FILES
    src/divvy/crypto/GenerateDeterministicKey.h
    src/divvy/crypto/KeyType.h
    src/divvy/crypto/RFC1751.h
    src/divvy/crypto/csprng.h
  DESTINATION include/divvy/crypto)
install (
  FILES
    src/divvy/crypto/impl/ec_key.h
    src/divvy/crypto/impl/openssl.h
  DESTINATION include/divvy/crypto/impl)
install (
  FILES
    src/divvy/json/JsonPropertyStream.h
    src/divvy/json/Object.h
    src/divvy/json/Output.h
    src/divvy/json/Writer.h
    src/divvy/json/json_forwards.h
    src/divvy/json/json_reader.h
    src/divvy/json/json_value.h
    src/divvy/json/json_writer.h
    src/divvy/json/to_string.h
  DESTINATION include/divvy/json)
install (
  FILES
    src/divvy/json/impl/json_assert.h
  DESTINATION include/divvy/json/impl)
install (
  FILES
    src/divvy/protocol/AccountID.h
    src/divvy/protocol/AmountConversions.h
    src/divvy/protocol/Book.h
    src/divvy/protocol/BuildInfo.h
    src/divvy/protocol/ErrorCodes.h
    src/divvy/protocol/Feature.h
    src/divvy/protocol/HashPrefix.h
    src/divvy/protocol/IOUAmount.h
    src/divvy/protocol/Indexes.h
    src/divvy/protocol/InnerObjectFormats.h
    src/divvy/protocol/Issue.h
    src/divvy/protocol/JsonFields.h
    src/divvy/protocol/Keylet.h
    src/divvy/protocol/KnownFormats.h
    src/divvy/protocol/LedgerFormats.h
    src/divvy/protocol/Protocol.h
    src/divvy/protocol/PublicKey.h
    src/divvy/protocol/Quality.h
    src/divvy/protocol/Rate.h
    src/divvy/protocol/SField.h
    src/divvy/protocol/SOTemplate.h
    src/divvy/protocol/STAccount.h
    src/divvy/protocol/STAmount.h
    src/divvy/protocol/STArray.h
    src/divvy/protocol/STBase.h
    src/divvy/protocol/STBitString.h
    src/divvy/protocol/STBlob.h
    src/divvy/protocol/STExchange.h
    src/divvy/protocol/STInteger.h
    src/divvy/protocol/STLedgerEntry.h
    src/divvy/protocol/STObject.h
    src/divvy/protocol/STParsedJSON.h
    src/divvy/protocol/STPathSet.h
    src/divvy/protocol/STTx.h
    src/divvy/protocol/STValidation.h
    src/divvy/protocol/STVector256.h
    src/divvy/protocol/SecretKey.h
    src/divvy/protocol/Seed.h
    src/divvy/protocol/Serializer.h
    src/divvy/protocol/Sign.h
    src/divvy/protocol/SystemParameters.h
    src/divvy/protocol/TER.h
    src/divvy/protocol/TxFlags.h
    src/divvy/protocol/TxFormats.h
    src/divvy/protocol/UintTypes.h
    src/divvy/protocol/XDVAmount.h
    src/divvy/protocol/digest.h
    src/divvy/protocol/tokens.h
  DESTINATION include/divvy/protocol)
install (
  FILES
    src/divvy/protocol/impl/STVar.h
    src/divvy/protocol/impl/secp256k1.h
  DESTINATION include/divvy/protocol/impl)

#[===================================[
   beast/legacy headers installation
#]===================================]
install (
  FILES
    src/divvy/beast/clock/abstract_clock.h
    src/divvy/beast/clock/basic_seconds_clock.h
    src/divvy/beast/clock/manual_clock.h
  DESTINATION include/divvy/beast/clock)
install (
  FILES
    src/divvy/beast/core/CompilerConfig.h
    src/divvy/beast/core/Config.h
    src/divvy/beast/core/ConfigCheck.h
    src/divvy/beast/core/LexicalCast.h
    src/divvy/beast/core/List.h
    src/divvy/beast/core/PlatformConfig.h
    src/divvy/beast/core/SemanticVersion.h
    src/divvy/beast/core/StandardConfig.h
  DESTINATION include/divvy/beast/core)
install (
  FILES
    src/divvy/beast/crypto/detail/mac_facade.h
    src/divvy/beast/crypto/detail/ripemd_context.h
    src/divvy/beast/crypto/detail/sha2_context.h
    src/divvy/beast/crypto/ripemd.h
    src/divvy/beast/crypto/secure_erase.h
    src/divvy/beast/crypto/sha2.h
  DESTINATION include/divvy/beast/crypto)
install (
  FILES
    src/divvy/beast/hash/endian.h
    src/divvy/beast/hash/hash_append.h
    src/divvy/beast/hash/meta.h
    src/divvy/beast/hash/uhash.h
    src/divvy/beast/hash/xxhasher.h
  DESTINATION include/divvy/beast/hash)
install (
  FILES src/divvy/beast/hash/impl/xxhash.h
  DESTINATION include/divvy/beast/hash/impl)
install (
  FILES
    src/divvy/beast/rfc2616.h
    src/divvy/beast/type_name.h
    src/divvy/beast/unit_test.h
    src/divvy/beast/xor_shift_engine.h
  DESTINATION include/divvy/beast)
install (
  FILES
    src/divvy/beast/utility/Journal.h
    src/divvy/beast/utility/PropertyStream.h
    src/divvy/beast/utility/Zero.h
    src/divvy/beast/utility/rngfill.h
  DESTINATION include/divvy/beast/utility)
# WARNING!! -- horrible levelization ahead
# (these files should be isolated or moved...but
#  unfortunately unit_test.h above creates this dependency)
install (
  FILES
    src/beast/extras/beast/unit_test/amount.hpp
    src/beast/extras/beast/unit_test/dstream.hpp
    src/beast/extras/beast/unit_test/global_suites.hpp
    src/beast/extras/beast/unit_test/match.hpp
    src/beast/extras/beast/unit_test/recorder.hpp
    src/beast/extras/beast/unit_test/reporter.hpp
    src/beast/extras/beast/unit_test/results.hpp
    src/beast/extras/beast/unit_test/runner.hpp
    src/beast/extras/beast/unit_test/suite.hpp
    src/beast/extras/beast/unit_test/suite_info.hpp
    src/beast/extras/beast/unit_test/suite_list.hpp
    src/beast/extras/beast/unit_test/thread.hpp
  DESTINATION include/beast/unit_test)
install (
  FILES
    src/beast/extras/beast/unit_test/detail/const_container.hpp
  DESTINATION include/beast/unit_test/detail)

#[===================================================================[
   divvyd executable
#]===================================================================]

#[=========================================================[
   this one header is added as source just to keep older
   versions of cmake happy. cmake 3.10+ allows
   add_executable with no sources
#]=========================================================]
add_executable (divvyd src/divvy/app/main/Application.h)
target_sources (divvyd PRIVATE
  $<$<BOOL:${unity}>:
    #[===============================[
       unity, main sources
    #]===============================]
    src/divvy/unity/app_consensus.cpp
    src/divvy/unity/app_ledger.cpp
    src/divvy/unity/app_ledger_impl.cpp
    src/divvy/unity/app_main1.cpp
    src/divvy/unity/app_main2.cpp
    src/divvy/unity/app_misc.cpp
    src/divvy/unity/app_misc_impl.cpp
    src/divvy/unity/app_paths.cpp
    src/divvy/unity/app_tx.cpp
    src/divvy/unity/conditions.cpp
    src/divvy/unity/consensus.cpp
    src/divvy/unity/core.cpp
    src/divvy/unity/basics2.cpp
    src/divvy/unity/ledger.cpp
    src/divvy/unity/net.cpp
    src/divvy/unity/nodestore.cpp
    src/divvy/unity/overlay1.cpp
    src/divvy/unity/overlay2.cpp
    src/divvy/unity/peerfinder.cpp
    src/divvy/unity/resource.cpp
    src/divvy/unity/rpcx1.cpp
    src/divvy/unity/rpcx2.cpp
    src/divvy/unity/shamap.cpp
    src/divvy/unity/server.cpp
    src/divvy/unity/soci_divvy.cpp
    #[===============================[
       unity, test sources
    #]===============================]
    src/test/unity/app_test_unity1.cpp
    src/test/unity/app_test_unity2.cpp
    src/test/unity/basics_test_unity.cpp
    src/test/unity/beast_test_unity1.cpp
    src/test/unity/beast_test_unity2.cpp
    src/test/unity/conditions_test_unity.cpp
    src/test/unity/consensus_test_unity.cpp
    src/test/unity/core_test_unity.cpp
    src/test/unity/crypto_test_unity.cpp
    src/test/unity/json_test_unity.cpp
    src/test/unity/ledger_test_unity.cpp
    src/test/unity/nodestore_test_unity.cpp
    src/test/unity/overlay_test_unity.cpp
    src/test/unity/peerfinder_test_unity.cpp
    src/test/unity/protocol_test_unity.cpp
    src/test/unity/resource_test_unity.cpp
    src/test/unity/rpc_test_unity.cpp
    src/test/unity/server_test_unity.cpp
    src/test/unity/server_status_test_unity.cpp
    src/test/unity/shamap_test_unity.cpp
    src/test/unity/jtx_unity1.cpp
    src/test/unity/jtx_unity2.cpp
    src/test/unity/csf_unity.cpp
  >
  $<$<NOT:$<BOOL:${unity}>>:
    #[===============================[
       nounity, main sources:
         subdir: app
    #]===============================]
    src/divvy/app/consensus/RCLConsensus.cpp
    src/divvy/app/consensus/RCLCxPeerPos.cpp
    src/divvy/app/consensus/RCLValidations.cpp
    src/divvy/app/ledger/AcceptedLedger.cpp
    src/divvy/app/ledger/AcceptedLedgerTx.cpp
    src/divvy/app/ledger/AccountStateSF.cpp
    src/divvy/app/ledger/BookListeners.cpp
    src/divvy/app/ledger/ConsensusTransSetSF.cpp
    src/divvy/app/ledger/Ledger.cpp
    src/divvy/app/ledger/LedgerHistory.cpp
    src/divvy/app/ledger/OrderBookDB.cpp
    src/divvy/app/ledger/TransactionStateSF.cpp
    src/divvy/app/ledger/impl/BuildLedger.cpp
    src/divvy/app/ledger/impl/InboundLedger.cpp
    src/divvy/app/ledger/impl/InboundLedgers.cpp
    src/divvy/app/ledger/impl/InboundTransactions.cpp
    src/divvy/app/ledger/impl/LedgerCleaner.cpp
    src/divvy/app/ledger/impl/LedgerMaster.cpp
    src/divvy/app/ledger/impl/LedgerReplay.cpp
    src/divvy/app/ledger/impl/LedgerToJson.cpp
    src/divvy/app/ledger/impl/LocalTxs.cpp
    src/divvy/app/ledger/impl/OpenLedger.cpp
    src/divvy/app/ledger/impl/TransactionAcquire.cpp
    src/divvy/app/ledger/impl/TransactionMaster.cpp
    src/divvy/app/main/Application.cpp
    src/divvy/app/main/BasicApp.cpp
    src/divvy/app/main/CollectorManager.cpp
    src/divvy/app/main/DBInit.cpp
    src/divvy/app/main/LoadManager.cpp
    src/divvy/app/main/Main.cpp
    src/divvy/app/main/NodeIdentity.cpp
    src/divvy/app/main/NodeStoreScheduler.cpp
    src/divvy/app/misc/CanonicalTXSet.cpp
    src/divvy/app/misc/FeeVoteImpl.cpp
    src/divvy/app/misc/HashRouter.cpp
    src/divvy/app/misc/NetworkOPs.cpp
    src/divvy/app/misc/SHAMapStoreImp.cpp
    src/divvy/app/misc/impl/AccountTxPaging.cpp
    src/divvy/app/misc/impl/AmendmentTable.cpp
    src/divvy/app/misc/impl/LoadFeeTrack.cpp
    src/divvy/app/misc/impl/Manifest.cpp
    src/divvy/app/misc/impl/Transaction.cpp
    src/divvy/app/misc/impl/TxQ.cpp
    src/divvy/app/misc/impl/ValidatorKeys.cpp
    src/divvy/app/misc/impl/ValidatorList.cpp
    src/divvy/app/misc/impl/ValidatorSite.cpp
    src/divvy/app/paths/AccountCurrencies.cpp
    src/divvy/app/paths/Credit.cpp
    src/divvy/app/paths/Flow.cpp
    src/divvy/app/paths/Node.cpp
    src/divvy/app/paths/PathRequest.cpp
    src/divvy/app/paths/PathRequests.cpp
    src/divvy/app/paths/PathState.cpp
    src/divvy/app/paths/Pathfinder.cpp
    src/divvy/app/paths/DivvyCalc.cpp
    src/divvy/app/paths/DivvyLineCache.cpp
    src/divvy/app/paths/DivvyState.cpp
    src/divvy/app/paths/cursor/AdvanceNode.cpp
    src/divvy/app/paths/cursor/DeliverNodeForward.cpp
    src/divvy/app/paths/cursor/DeliverNodeReverse.cpp
    src/divvy/app/paths/cursor/EffectiveRate.cpp
    src/divvy/app/paths/cursor/ForwardLiquidity.cpp
    src/divvy/app/paths/cursor/ForwardLiquidityForAccount.cpp
    src/divvy/app/paths/cursor/Liquidity.cpp
    src/divvy/app/paths/cursor/NextIncrement.cpp
    src/divvy/app/paths/cursor/ReverseLiquidity.cpp
    src/divvy/app/paths/cursor/ReverseLiquidityForAccount.cpp
    src/divvy/app/paths/cursor/DivvyLiquidity.cpp
    src/divvy/app/paths/impl/BookStep.cpp
    src/divvy/app/paths/impl/DirectStep.cpp
    src/divvy/app/paths/impl/PaySteps.cpp
    src/divvy/app/paths/impl/XDVEndpointStep.cpp
    src/divvy/app/tx/impl/ApplyContext.cpp
    src/divvy/app/tx/impl/BookTip.cpp
    src/divvy/app/tx/impl/CancelCheck.cpp
    src/divvy/app/tx/impl/CancelOffer.cpp
    src/divvy/app/tx/impl/CancelTicket.cpp
    src/divvy/app/tx/impl/CashCheck.cpp
    src/divvy/app/tx/impl/Change.cpp
    src/divvy/app/tx/impl/CreateCheck.cpp
    src/divvy/app/tx/impl/CreateOffer.cpp
    src/divvy/app/tx/impl/CreateTicket.cpp
    src/divvy/app/tx/impl/DepositPreauth.cpp
    src/divvy/app/tx/impl/Escrow.cpp
    src/divvy/app/tx/impl/InvariantCheck.cpp
    src/divvy/app/tx/impl/OfferStream.cpp
    src/divvy/app/tx/impl/PayChan.cpp
    src/divvy/app/tx/impl/Payment.cpp
    src/divvy/app/tx/impl/SetAccount.cpp
    src/divvy/app/tx/impl/SetRegularKey.cpp
    src/divvy/app/tx/impl/SetSignerList.cpp
    src/divvy/app/tx/impl/SetTrust.cpp
    src/divvy/app/tx/impl/SignerEntries.cpp
    src/divvy/app/tx/impl/Taker.cpp
    src/divvy/app/tx/impl/Transactor.cpp
    src/divvy/app/tx/impl/apply.cpp
    src/divvy/app/tx/impl/applySteps.cpp
    #[===============================[
       nounity, main sources:
         subdir: basics (partial)
    #]===============================]
    src/divvy/basics/impl/Archive.cpp
    src/divvy/basics/impl/BasicConfig.cpp
    src/divvy/basics/impl/PerfLogImp.cpp
    src/divvy/basics/impl/ResolverAsio.cpp
    src/divvy/basics/impl/Sustain.cpp
    src/divvy/basics/impl/UptimeClock.cpp
    src/divvy/basics/impl/make_SSLContext.cpp
    src/divvy/basics/impl/mulDiv.cpp
    #[===============================[
       nounity, main sources:
         subdir: conditions
    #]===============================]
    src/divvy/conditions/impl/Condition.cpp
    src/divvy/conditions/impl/Fulfillment.cpp
    src/divvy/conditions/impl/error.cpp
    #[===============================[
       nounity, main sources:
         subdir: core
    #]===============================]
    src/divvy/core/impl/Config.cpp
    src/divvy/core/impl/DatabaseCon.cpp
    src/divvy/core/impl/DummySociDynamicBackend.cpp
    src/divvy/core/impl/Job.cpp
    src/divvy/core/impl/JobQueue.cpp
    src/divvy/core/impl/LoadEvent.cpp
    src/divvy/core/impl/LoadMonitor.cpp
    src/divvy/core/impl/SNTPClock.cpp
    src/divvy/core/impl/SociDB.cpp
    src/divvy/core/impl/Stoppable.cpp
    src/divvy/core/impl/TerminateHandler.cpp
    src/divvy/core/impl/TimeKeeper.cpp
    src/divvy/core/impl/Workers.cpp
    #[===============================[
       nounity, main sources:
         subdir: consensus
    #]===============================]
    src/divvy/consensus/Consensus.cpp
    #[===============================[
       nounity, main sources:
         subdir: ledger
    #]===============================]
    src/divvy/ledger/impl/ApplyStateTable.cpp
    src/divvy/ledger/impl/ApplyView.cpp
    src/divvy/ledger/impl/ApplyViewBase.cpp
    src/divvy/ledger/impl/ApplyViewImpl.cpp
    src/divvy/ledger/impl/BookDirs.cpp
    src/divvy/ledger/impl/CachedSLEs.cpp
    src/divvy/ledger/impl/CachedView.cpp
    src/divvy/ledger/impl/CashDiff.cpp
    src/divvy/ledger/impl/Directory.cpp
    src/divvy/ledger/impl/OpenView.cpp
    src/divvy/ledger/impl/PaymentSandbox.cpp
    src/divvy/ledger/impl/RawStateTable.cpp
    src/divvy/ledger/impl/ReadView.cpp
    src/divvy/ledger/impl/TxMeta.cpp
    src/divvy/ledger/impl/View.cpp
    #[===============================[
       nounity, main sources:
         subdir: net
    #]===============================]
    src/divvy/net/impl/HTTPClient.cpp
    src/divvy/net/impl/InfoSub.cpp
    src/divvy/net/impl/RPCCall.cpp
    src/divvy/net/impl/RPCErr.cpp
    src/divvy/net/impl/RPCSub.cpp
    src/divvy/net/impl/RegisterSSLCerts.cpp
    src/divvy/net/impl/SSLHTTPDownloader.cpp
    #[===============================[
       nounity, main sources:
         subdir: nodestore
    #]===============================]
    src/divvy/nodestore/backend/MemoryFactory.cpp
    src/divvy/nodestore/backend/NuDBFactory.cpp
    src/divvy/nodestore/backend/NullFactory.cpp
    src/divvy/nodestore/backend/RocksDBFactory.cpp
    src/divvy/nodestore/backend/RocksDBQuickFactory.cpp
    src/divvy/nodestore/impl/BatchWriter.cpp
    src/divvy/nodestore/impl/Database.cpp
    src/divvy/nodestore/impl/DatabaseNodeImp.cpp
    src/divvy/nodestore/impl/DatabaseRotatingImp.cpp
    src/divvy/nodestore/impl/DatabaseShardImp.cpp
    src/divvy/nodestore/impl/DecodedBlob.cpp
    src/divvy/nodestore/impl/DummyScheduler.cpp
    src/divvy/nodestore/impl/EncodedBlob.cpp
    src/divvy/nodestore/impl/ManagerImp.cpp
    src/divvy/nodestore/impl/NodeObject.cpp
    src/divvy/nodestore/impl/Shard.cpp
    #[===============================[
       nounity, main sources:
         subdir: overlay
    #]===============================]
    src/divvy/overlay/impl/Cluster.cpp
    src/divvy/overlay/impl/ConnectAttempt.cpp
    src/divvy/overlay/impl/Message.cpp
    src/divvy/overlay/impl/OverlayImpl.cpp
    src/divvy/overlay/impl/PeerImp.cpp
    src/divvy/overlay/impl/PeerSet.cpp
    src/divvy/overlay/impl/TMHello.cpp
    src/divvy/overlay/impl/TrafficCount.cpp
    #[===============================[
       nounity, main sources:
         subdir: peerfinder
    #]===============================]
    src/divvy/peerfinder/impl/Bootcache.cpp
    src/divvy/peerfinder/impl/Endpoint.cpp
    src/divvy/peerfinder/impl/PeerfinderConfig.cpp
    src/divvy/peerfinder/impl/PeerfinderManager.cpp
    src/divvy/peerfinder/impl/SlotImp.cpp
    src/divvy/peerfinder/impl/SourceStrings.cpp
    #[===============================[
       nounity, main sources:
         subdir: resource
    #]===============================]
    src/divvy/resource/impl/Charge.cpp
    src/divvy/resource/impl/Consumer.cpp
    src/divvy/resource/impl/Fees.cpp
    src/divvy/resource/impl/ResourceManager.cpp
    #[===============================[
       nounity, main sources:
         subdir: rpc
    #]===============================]
    src/divvy/rpc/handlers/AccountChannels.cpp
    src/divvy/rpc/handlers/AccountCurrenciesHandler.cpp
    src/divvy/rpc/handlers/AccountInfo.cpp
    src/divvy/rpc/handlers/AccountLines.cpp
    src/divvy/rpc/handlers/AccountObjects.cpp
    src/divvy/rpc/handlers/AccountOffers.cpp
    src/divvy/rpc/handlers/AccountTx.cpp
    src/divvy/rpc/handlers/AccountTxOld.cpp
    src/divvy/rpc/handlers/AccountTxSwitch.cpp
    src/divvy/rpc/handlers/BlackList.cpp
    src/divvy/rpc/handlers/BookOffers.cpp
    src/divvy/rpc/handlers/CanDelete.cpp
    src/divvy/rpc/handlers/Connect.cpp
    src/divvy/rpc/handlers/ConsensusInfo.cpp
    src/divvy/rpc/handlers/DepositAuthorized.cpp
    src/divvy/rpc/handlers/DownloadShard.cpp
    src/divvy/rpc/handlers/Feature1.cpp
    src/divvy/rpc/handlers/Fee1.cpp
    src/divvy/rpc/handlers/FetchInfo.cpp
    src/divvy/rpc/handlers/GatewayBalances.cpp
    src/divvy/rpc/handlers/GetCounts.cpp
    src/divvy/rpc/handlers/LedgerAccept.cpp
    src/divvy/rpc/handlers/LedgerCleanerHandler.cpp
    src/divvy/rpc/handlers/LedgerClosed.cpp
    src/divvy/rpc/handlers/LedgerCurrent.cpp
    src/divvy/rpc/handlers/LedgerData.cpp
    src/divvy/rpc/handlers/LedgerEntry.cpp
    src/divvy/rpc/handlers/LedgerHandler.cpp
    src/divvy/rpc/handlers/LedgerHeader.cpp
    src/divvy/rpc/handlers/LedgerRequest.cpp
    src/divvy/rpc/handlers/LogLevel.cpp
    src/divvy/rpc/handlers/LogRotate.cpp
    src/divvy/rpc/handlers/NoDivvyCheck.cpp
    src/divvy/rpc/handlers/OwnerInfo.cpp
    src/divvy/rpc/handlers/PathFind.cpp
    src/divvy/rpc/handlers/PayChanClaim.cpp
    src/divvy/rpc/handlers/Peers.cpp
    src/divvy/rpc/handlers/Ping.cpp
    src/divvy/rpc/handlers/Print.cpp
    src/divvy/rpc/handlers/Random.cpp
    src/divvy/rpc/handlers/DivvyPathFind.cpp
    src/divvy/rpc/handlers/ServerInfo.cpp
    src/divvy/rpc/handlers/ServerState.cpp
    src/divvy/rpc/handlers/SignFor.cpp
    src/divvy/rpc/handlers/SignHandler.cpp
    src/divvy/rpc/handlers/Stop.cpp
    src/divvy/rpc/handlers/Submit.cpp
    src/divvy/rpc/handlers/SubmitMultiSigned.cpp
    src/divvy/rpc/handlers/Subscribe.cpp
    src/divvy/rpc/handlers/TransactionEntry.cpp
    src/divvy/rpc/handlers/Tx.cpp
    src/divvy/rpc/handlers/TxHistory.cpp
    src/divvy/rpc/handlers/UnlList.cpp
    src/divvy/rpc/handlers/Unsubscribe.cpp
    src/divvy/rpc/handlers/ValidationCreate.cpp
    src/divvy/rpc/handlers/ValidationSeed.cpp
    src/divvy/rpc/handlers/ValidatorListSites.cpp
    src/divvy/rpc/handlers/Validators.cpp
    src/divvy/rpc/handlers/WalletPropose.cpp
    src/divvy/rpc/impl/Handler.cpp
    src/divvy/rpc/impl/LegacyPathFind.cpp
    src/divvy/rpc/impl/RPCHandler.cpp
    src/divvy/rpc/impl/RPCHelpers.cpp
    src/divvy/rpc/impl/Role.cpp
    src/divvy/rpc/impl/ServerHandlerImp.cpp
    src/divvy/rpc/impl/ShardArchiveHandler.cpp
    src/divvy/rpc/impl/Status.cpp
    src/divvy/rpc/impl/TransactionSign.cpp
    #[===============================[
       nounity, main sources:
         subdir: server
    #]===============================]
    src/divvy/server/impl/JSONRPCUtil.cpp
    src/divvy/server/impl/Port.cpp
    #[===============================[
       nounity, main sources:
         subdir: shamap
    #]===============================]
    src/divvy/shamap/impl/SHAMap.cpp
    src/divvy/shamap/impl/SHAMapDelta.cpp
    src/divvy/shamap/impl/SHAMapItem.cpp
    src/divvy/shamap/impl/SHAMapMissingNode.cpp
    src/divvy/shamap/impl/SHAMapNodeID.cpp
    src/divvy/shamap/impl/SHAMapSync.cpp
    src/divvy/shamap/impl/SHAMapTreeNode.cpp
    #[===============================[
       nounity, test sources:
         subdir: app
    #]===============================]
    src/test/app/AccountTxPaging_test.cpp
    src/test/app/AmendmentTable_test.cpp
    src/test/app/Check_test.cpp
    src/test/app/CrossingLimits_test.cpp
    src/test/app/DeliverMin_test.cpp
    src/test/app/DepositAuth_test.cpp
    src/test/app/Discrepancy_test.cpp
    src/test/app/Escrow_test.cpp
    src/test/app/Flow_test.cpp
    src/test/app/Freeze_test.cpp
    src/test/app/HashRouter_test.cpp
    src/test/app/LedgerHistory_test.cpp
    src/test/app/LedgerLoad_test.cpp
    src/test/app/LedgerReplay_test.cpp
    src/test/app/LoadFeeTrack_test.cpp
    src/test/app/Manifest_test.cpp
    src/test/app/MultiSign_test.cpp
    src/test/app/OfferStream_test.cpp
    src/test/app/Offer_test.cpp
    src/test/app/OversizeMeta_test.cpp
    src/test/app/Path_test.cpp
    src/test/app/PayChan_test.cpp
    src/test/app/PayStrand_test.cpp
    src/test/app/PseudoTx_test.cpp
    src/test/app/RCLValidations_test.cpp
    src/test/app/Regression_test.cpp
    src/test/app/SHAMapStore_test.cpp
    src/test/app/SetAuth_test.cpp
    src/test/app/SetRegularKey_test.cpp
    src/test/app/SetTrust_test.cpp
    src/test/app/Taker_test.cpp
    src/test/app/Ticket_test.cpp
    src/test/app/Transaction_ordering_test.cpp
    src/test/app/TrustAndBalance_test.cpp
    src/test/app/TxQ_test.cpp
    src/test/app/ValidatorKeys_test.cpp
    src/test/app/ValidatorList_test.cpp
    src/test/app/ValidatorSite_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: basics
    #]===============================]
    src/test/basics/Buffer_test.cpp
    src/test/basics/DetectCrash_test.cpp
    src/test/basics/KeyCache_test.cpp
    src/test/basics/PerfLog_test.cpp
    src/test/basics/RangeSet_test.cpp
    src/test/basics/Slice_test.cpp
    src/test/basics/StringUtilities_test.cpp
    src/test/basics/TaggedCache_test.cpp
    src/test/basics/base64_test.cpp
    src/test/basics/base_uint_test.cpp
    src/test/basics/contract_test.cpp
    src/test/basics/hardened_hash_test.cpp
    src/test/basics/mulDiv_test.cpp
    src/test/basics/qalloc_test.cpp
    src/test/basics/tagged_integer_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: beast
    #]===============================]
    src/test/beast/IPEndpoint_test.cpp
    src/test/beast/LexicalCast_test.cpp
    src/test/beast/SemanticVersion_test.cpp
    src/test/beast/aged_associative_container_test.cpp
    src/test/beast/beast_CurrentThreadName_test.cpp
    src/test/beast/beast_Debug_test.cpp
    src/test/beast/beast_Journal_test.cpp
    src/test/beast/beast_PropertyStream_test.cpp
    src/test/beast/beast_Zero_test.cpp
    src/test/beast/beast_abstract_clock_test.cpp
    src/test/beast/beast_asio_error_test.cpp
    src/test/beast/beast_basic_seconds_clock_test.cpp
    src/test/beast/beast_io_latency_probe_test.cpp
    src/test/beast/beast_weak_fn_test.cpp
    src/test/beast/define_print.cpp
    #[===============================[
       nounity, test sources:
         subdir: conditions
    #]===============================]
    src/test/conditions/PreimageSha256_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: consensus
    #]===============================]
    src/test/consensus/ByzantineFailureSim_test.cpp
    src/test/consensus/Consensus_test.cpp
    src/test/consensus/DistributedValidatorsSim_test.cpp
    src/test/consensus/LedgerTiming_test.cpp
    src/test/consensus/LedgerTrie_test.cpp
    src/test/consensus/ScaleFreeSim_test.cpp
    src/test/consensus/Validations_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: core
    #]===============================]
    src/test/core/ClosureCounter_test.cpp
    src/test/core/Config_test.cpp
    src/test/core/Coroutine_test.cpp
    src/test/core/CryptoPRNG_test.cpp
    src/test/core/JobQueue_test.cpp
    src/test/core/SociDB_test.cpp
    src/test/core/Stoppable_test.cpp
    src/test/core/TerminateHandler_test.cpp
    src/test/core/Workers_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: crypto
    #]===============================]
    src/test/crypto/Openssl_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: csf
    #]===============================]
    src/test/csf/BasicNetwork_test.cpp
    src/test/csf/Digraph_test.cpp
    src/test/csf/Histogram_test.cpp
    src/test/csf/Scheduler_test.cpp
    src/test/csf/impl/Sim.cpp
    src/test/csf/impl/ledgers.cpp
    #[===============================[
       nounity, test sources:
         subdir: json
    #]===============================]
    src/test/json/Object_test.cpp
    src/test/json/Output_test.cpp
    src/test/json/Writer_test.cpp
    src/test/json/json_value_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: jtx
    #]===============================]
    src/test/jtx/Env_test.cpp
    src/test/jtx/WSClient_test.cpp
    src/test/jtx/impl/Account.cpp
    src/test/jtx/impl/Env.cpp
    src/test/jtx/impl/JSONRPCClient.cpp
    src/test/jtx/impl/ManualTimeKeeper.cpp
    src/test/jtx/impl/WSClient.cpp
    src/test/jtx/impl/amount.cpp
    src/test/jtx/impl/balance.cpp
    src/test/jtx/impl/delivermin.cpp
    src/test/jtx/impl/deposit.cpp
    src/test/jtx/impl/envconfig.cpp
    src/test/jtx/impl/fee.cpp
    src/test/jtx/impl/flags.cpp
    src/test/jtx/impl/jtx_json.cpp
    src/test/jtx/impl/memo.cpp
    src/test/jtx/impl/multisign.cpp
    src/test/jtx/impl/offer.cpp
    src/test/jtx/impl/owners.cpp
    src/test/jtx/impl/paths.cpp
    src/test/jtx/impl/pay.cpp
    src/test/jtx/impl/quality2.cpp
    src/test/jtx/impl/rate.cpp
    src/test/jtx/impl/regkey.cpp
    src/test/jtx/impl/sendmax.cpp
    src/test/jtx/impl/seq.cpp
    src/test/jtx/impl/sig.cpp
    src/test/jtx/impl/tag.cpp
    src/test/jtx/impl/ticket.cpp
    src/test/jtx/impl/trust.cpp
    src/test/jtx/impl/txflags.cpp
    src/test/jtx/impl/utility.cpp
    #[===============================[
       nounity, test sources:
         subdir: ledger
    #]===============================]
    src/test/ledger/BookDirs_test.cpp
    src/test/ledger/CashDiff_test.cpp
    src/test/ledger/Directory_test.cpp
    src/test/ledger/Invariants_test.cpp
    src/test/ledger/PaymentSandbox_test.cpp
    src/test/ledger/PendingSaves_test.cpp
    src/test/ledger/SHAMapV2_test.cpp
    src/test/ledger/SkipList_test.cpp
    src/test/ledger/View_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: nodestore
    #]===============================]
    src/test/nodestore/Backend_test.cpp
    src/test/nodestore/Basics_test.cpp
    src/test/nodestore/Database_test.cpp
    src/test/nodestore/Timing_test.cpp
    src/test/nodestore/import_test.cpp
    src/test/nodestore/varint_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: overlay
    #]===============================]
    src/test/overlay/TMHello_test.cpp
    src/test/overlay/cluster_test.cpp
    src/test/overlay/short_read_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: peerfinder
    #]===============================]
    src/test/peerfinder/Livecache_test.cpp
    src/test/peerfinder/PeerFinder_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: protocol
    #]===============================]
    src/test/protocol/BuildInfo_test.cpp
    src/test/protocol/IOUAmount_test.cpp
    src/test/protocol/InnerObjectFormats_test.cpp
    src/test/protocol/Issue_test.cpp
    src/test/protocol/PublicKey_test.cpp
    src/test/protocol/Quality_test.cpp
    src/test/protocol/STAccount_test.cpp
    src/test/protocol/STAmount_test.cpp
    src/test/protocol/STObject_test.cpp
    src/test/protocol/STTx_test.cpp
    src/test/protocol/STValidation_test.cpp
    src/test/protocol/SecretKey_test.cpp
    src/test/protocol/Seed_test.cpp
    src/test/protocol/TER_test.cpp
    src/test/protocol/XDVAmount_test.cpp
    src/test/protocol/digest_test.cpp
    src/test/protocol/types_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: resource
    #]===============================]
    src/test/resource/Logic_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: rpc
    #]===============================]
    src/test/rpc/AccountCurrencies_test.cpp
    src/test/rpc/AccountInfo_test.cpp
    src/test/rpc/AccountLinesRPC_test.cpp
    src/test/rpc/AccountObjects_test.cpp
    src/test/rpc/AccountOffers_test.cpp
    src/test/rpc/AccountSet_test.cpp
    src/test/rpc/AccountTx_test.cpp
    src/test/rpc/AmendmentBlocked_test.cpp
    src/test/rpc/Book_test.cpp
    src/test/rpc/DepositAuthorized_test.cpp
    src/test/rpc/Feature_test.cpp
    src/test/rpc/GatewayBalances_test.cpp
    src/test/rpc/GetCounts_test.cpp
    src/test/rpc/JSONRPC_test.cpp
    src/test/rpc/KeyGeneration_test.cpp
    src/test/rpc/LedgerClosed_test.cpp
    src/test/rpc/LedgerData_test.cpp
    src/test/rpc/LedgerRPC_test.cpp
    src/test/rpc/LedgerRequestRPC_test.cpp
    src/test/rpc/NoDivvyCheck_test.cpp
    src/test/rpc/NoDivvy_test.cpp
    src/test/rpc/OwnerInfo_test.cpp
    src/test/rpc/Peers_test.cpp
    src/test/rpc/RPCOverload_test.cpp
    src/test/rpc/RobustTransaction_test.cpp
    src/test/rpc/ServerInfo_test.cpp
    src/test/rpc/Status_test.cpp
    src/test/rpc/Subscribe_test.cpp
    src/test/rpc/TransactionEntry_test.cpp
    src/test/rpc/TransactionHistory_test.cpp
    src/test/rpc/ValidatorRPC_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: server
    #]===============================]
    src/test/server/ServerStatus_test.cpp
    src/test/server/Server_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: shamap
    #]===============================]
    src/test/shamap/FetchPack_test.cpp
    src/test/shamap/SHAMapSync_test.cpp
    src/test/shamap/SHAMap_test.cpp
    #[===============================[
       nounity, test sources:
         subdir: unit_test
    #]===============================]
    src/test/unit_test/multi_runner.cpp
    >)
target_link_libraries (divvyd
  Divvy::opts
  Divvy::libs
  Divvy::boost
  Divvy::xdvl_core)
exclude_if_included (divvyd)

#[===================================================================[
   install stuff
#]===================================================================]

install (
  TARGETS
    secp256k1
    ed25519-donna
    common
    opts
    divvy_syslibs
    divvy_boost
    xdvl_core
  EXPORT DivvyExports
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)
install (EXPORT DivvyExports
  FILE DivvyTargets.cmake
  NAMESPACE Divvy::
  DESTINATION lib/cmake/divvy)
include (CMakePackageConfigHelpers)
write_basic_package_version_file (
  DivvyConfigVersion.cmake
  VERSION ${divvyd_version}
  COMPATIBILITY SameMajorVersion)

if (is_root_project)
  install (TARGETS divvyd RUNTIME DESTINATION bin)
  set_target_properties(divvyd PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
  install (
    FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/Builds/CMake/DivvyConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/DivvyConfigVersion.cmake
    DESTINATION lib/cmake/divvy)
  # sample configs should not overwrite existing files
  # install if-not-exists workaround as suggested by
  # https://cmake.org/Bug/view.php?id=12646
  install(CODE "
    macro (copy_if_not_exists SRC DEST NEWNAME)
      if (NOT EXISTS \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/\${DEST}/\${NEWNAME}\")
        file (INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/\${DEST}\" FILES \"\${SRC}\" RENAME \"\${NEWNAME}\")
      else ()
        message (\"-- Skipping : \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/\${DEST}/\${NEWNAME}\")
      endif ()
    endmacro()
    copy_if_not_exists(\"${CMAKE_CURRENT_SOURCE_DIR}/cfg/divvyd-example.cfg\" etc divvyd.cfg)
    copy_if_not_exists(\"${CMAKE_CURRENT_SOURCE_DIR}/cfg/validators-example.txt\" etc validators.txt)
  ")
endif ()

#[===================================================================[
   multiconfig misc
#]===================================================================]

if (is_multiconfig)
  # Divvyd headers. Only useful for IDEs.
  file (GLOB_RECURSE divvyd_headers src/*.h src/*.hpp *.md)
  target_sources (divvyd PRIVATE ${divvyd_headers})
  set_property (
    SOURCE ${divvyd_headers}
    APPEND
    PROPERTY HEADER_FILE_ONLY true)
  if (MSVC)
    set_property(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      PROPERTY VS_STARTUP_PROJECT divvyd)
  endif ()
endif ()

if (is_multiconfig)
  group_sources(src)
  group_sources(docs)
  group_sources(Builds)
endif ()

#[===================================================================[
   docs target (optional)
#]===================================================================]

find_package (Doxygen)
if (TARGET Doxygen::doxygen)
  set (doc_srcs docs/source.dox)
  file (GLOB_RECURSE other_docs docs/*.md)
  list (APPEND doc_srcs "${other_docs}")
  # read the source config and make a modified one
  # that points the output files to our build directory
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/docs/source.dox" dox_content)
  string (REGEX REPLACE "[\t ]*OUTPUT_DIRECTORY[\t ]*=(.*)"
    "OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}\n\\1"
    new_config "${dox_content}")
  file (WRITE "${CMAKE_BINARY_DIR}/source.dox" "${new_config}")
  add_custom_target (docs
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/source.dox"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/html_doc/index.html"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs"
    SOURCES "${doc_srcs}")
  if (is_multiconfig)
    set_property (
      SOURCE ${doc_srcs}
      APPEND
      PROPERTY HEADER_FILE_ONLY
      true)
  endif ()
else ()
  message (STATUS "doxygen executable not found -- skipping docs target")
endif ()
