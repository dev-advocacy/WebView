cmake_minimum_required(VERSION 3.20)
project(WebView2HttpClient CXX)

# -------------------------------------------------------------------------
# C++20 standard
# -------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------------
# cpprestsdk-stripped fork -- build from source via submodule
# No vcpkg dependency on cpprestsdk: we consume the fork directly.
# The fork's CMakeLists produces the 'cpprest' target.
# -------------------------------------------------------------------------
if(WIN32)
  set(PLATFORM_LIBS winhttp wininet)
  # cpprestsdk submodule path
  set(CPPREST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/cpprestsdk/Release)
  if(NOT EXISTS ${CPPREST_ROOT}/CMakeLists.txt)
    message(FATAL_ERROR "cpprestsdk submodule not found. Run: git submodule update --init --recursive")
  endif()
  set(BUILD_TESTS   OFF CACHE BOOL "" FORCE)
  set(BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
  set(CPPREST_EXCLUDE_BROTLI ON CACHE BOOL "" FORCE)
  add_subdirectory(${CPPREST_ROOT} cpprestsdk_build EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "WebView2HttpClient supports Windows only.")
endif()

# -------------------------------------------------------------------------
# DLL source files
# -------------------------------------------------------------------------
set(HTTPCLIENT_SOURCES
  dllmain.cpp
  WebView2HttpClient.cpp

  # Backend abstraction
  Backend/HttpBackendFactory.cpp
  Backend/WinHttpBackend.cpp
  Backend/WinINetBackend.cpp
  Backend/CppRestBackend.cpp

  # Cookie synchronization bridge
  Cookies/CookieStore.cpp
  Cookies/CookieBridge.cpp

  # Polly-style resilience policies
  Resilience/RetryPolicy.cpp
  Resilience/CircuitBreakerPolicy.cpp
  Resilience/TimeoutPolicy.cpp
  Resilience/RateLimiterPolicy.cpp
  Resilience/FallbackPolicy.cpp
  Resilience/PolicyExecutor.cpp

  # OpenTelemetry instrumentation
  Telemetry/HttpTelemetry.cpp
  Telemetry/PerformanceProbe.cpp

  # Performance benchmark (WinHTTP vs WinINet vs CppRest)
  Benchmark/HttpBenchmark.cpp
))

# -------------------------------------------------------------------------
# Shared library (DLL on Windows, .so on Linux)
# -------------------------------------------------------------------------
add_library(WebView2HttpClient SHARED ${HTTPCLIENT_SOURCES})

# Export macro: HTTPCLIENT_EXPORTS defined when building the DLL
target_compile_definitions(WebView2HttpClient PRIVATE HTTPCLIENT_EXPORTS)

target_compile_features(WebView2HttpClient PUBLIC cxx_std_20)

target_include_directories(WebView2HttpClient
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Backend>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Cookies>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Resilience>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Telemetry>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Benchmark>
    # WebView2::Cookie model
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../WebView2/Security>
)

# OpenTelemetry (via vcpkg)
find_package(opentelemetry-cpp CONFIG REQUIRED)

target_link_libraries(WebView2HttpClient
  PRIVATE
    cpprest
    opentelemetry-cpp::trace
    opentelemetry-cpp::metrics
    opentelemetry-cpp::logs
    opentelemetry-cpp::otlp_http_exporter
    ${PLATFORM_LIBS}
)

# -------------------------------------------------------------------------
# Install
# -------------------------------------------------------------------------
install(TARGETS WebView2HttpClient
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(FILES WebView2HttpClient.h DESTINATION include)