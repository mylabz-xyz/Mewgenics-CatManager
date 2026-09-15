#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct CatData
{
    uint64_t id = 0;
    int64_t sqlKey = 0;

    std::string name;
    std::string sex;

    int age = 0;
    int level = 0;

    bool retired = false;
    bool dead = false;

    std::string room;


    // Génétique
    uint64_t parentAId = 0;
    uint64_t parentBId = 0;
    double coi = 0.0;


    // Relations calculées
    std::vector<uint64_t> children;


    // Présence actuelle
    bool inHouse = false;


        // temporaire analyse
    std::vector<uint8_t> rawData;
};