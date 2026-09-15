cmake_minimum_required(VERSION 3.20)

project(CatManager)

add_library(
    CatManager SHARED
    CatManager.cpp
    Globals.cpp
    Mod.cpp
    Logger.cpp
    Hooks.cpp

    Save/SaveLocator.cpp
    Save/Repository/SaveParser.cpp

    sqlite3.c
)

target_compile_definitions(
    CatManager
    PRIVATE
    CATMANAGER_EXPORTS
)