add_library(arango_pregel STATIC
  AggregatorHandler.cpp
  AlgoRegistry.cpp
  Conductor.cpp
  GraphStore.cpp
  IncomingCache.cpp
  IndexHelpers.cpp
  OutgoingCache.cpp
  PregelFeature.cpp
  PregelMetrics.cpp
  Recovery.cpp
  Reports.cpp
  Reports.h
  Utils.cpp
  Worker.cpp
  WorkerConfig.cpp
  ${PROJECT_SOURCE_DIR}/arangod/RestHandler/RestControlPregelHandler.cpp
  ${PROJECT_SOURCE_DIR}/arangod/RestHandler/RestPregelHandler.cpp)

target_link_libraries(arango_pregel
	arango
  arango_agency
  arango_greenspun
  boost_boost
  boost_system)

target_include_directories(arango_pregel PRIVATE
  "${PROJECT_SOURCE_DIR}/arangod"
  "${PROJECT_SOURCE_DIR}/${ENTERPRISE_INCLUDE_DIR}")

target_compile_options(arango_pregel PRIVATE
	${CMAKE_DEPRECATE_OPTIONS})

add_subdirectory(Algos)
