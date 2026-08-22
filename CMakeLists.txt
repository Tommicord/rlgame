cmake_minimum_required(VERSION 4.2.0)

project(rlgame LANGUAGES C CXX)

include(cmake/BuildCompiler.cmake)
include(cmake/BuildDependencies.cmake)
include(cmake/BuildLibUtils.cmake)
include(cmake/BuildLib.cmake)
include(cmake/BuildEmbed.cmake)
include(cmake/BuildTesting.cmake)