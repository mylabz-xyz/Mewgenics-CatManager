#include "../../Logger.h"
#include "../Decoder/CatDecoder.h"
#include "../Repository/SaveParser.h"
#include "../Pedigree/PedigreeParser.h"

extern "C"
{
#include "../../sqlite3.h"
}

#include <vector>

SaveData ParseSave(const char *path)
{
    SaveData save;

    sqlite3 *db = nullptr;

    Log("[CatManager] Opening sqlite");

    int rc = sqlite3_open(
        path,
        &db);

    if (rc != SQLITE_OK)
    {
        Log(
            "[CatManager] SQLite open failed rc=%d",
            rc);

        return save;
    }

    Log("[CatManager] SQLite opened");

    sqlite3_stmt *stmt = nullptr;

    rc = sqlite3_prepare_v2(
        db,
        "SELECT key, data FROM cats;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        Log(
            "[CatManager] Prepare failed rc=%d",
            rc);

        sqlite3_close(db);

        return save;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        sqlite3_int64 key =
            sqlite3_column_int64(stmt, 0);

        const unsigned char *blob =
            (const unsigned char *)sqlite3_column_blob(stmt, 1);

        int size =
            sqlite3_column_bytes(stmt, 1);

        if (!blob || size <= 0)
            continue;

        std::vector<unsigned char> wrapped(
            blob,
            blob + size);

        std::vector<unsigned char> dec;

        if (!DecompressCatBlob(
                wrapped,
                dec))
        {
            continue;
        }

        CatData cat =
            DecodeCat(dec);

        cat.rawData = dec;
        cat.sqlKey = key;

        save.sqlToCat[key] = cat.id;

        save.cats.emplace(
            cat.id,
            std::move(cat));
    }

    sqlite3_finalize(stmt);
    LoadPedigree(db, save);

    // Charger l'état de la maison (pièces et présence dans la maison vs extérieur)
    sqlite3_stmt *houseStmt = nullptr;
    rc = sqlite3_prepare_v2(
        db,
        "SELECT data FROM files WHERE key='house_state';",
        -1,
        &houseStmt,
        nullptr);

    if (rc == SQLITE_OK && sqlite3_step(houseStmt) == SQLITE_ROW)
    {
        const unsigned char *hBlob =
            (const unsigned char *)sqlite3_column_blob(houseStmt, 0);
        int hSize = sqlite3_column_bytes(houseStmt, 0);

        if (hBlob && hSize >= 8)
        {
            uint32_t count = 0;
            memcpy(&count, hBlob + 4, 4);

            size_t pos = 8;
            size_t inHouseCount = 0;
            size_t outsideCount = 0;

            for (uint32_t i = 0; i < count; i++)
            {
                if (pos + 16 > (size_t)hSize)
                    break;

                uint64_t catSqlKey = 0;
                uint64_t nameLen = 0;
                memcpy(&catSqlKey, hBlob + pos, 8);
                memcpy(&nameLen, hBlob + pos + 8, 8);
                pos += 16;

                std::string roomName;
                if (nameLen > 0 && pos + nameLen <= (size_t)hSize)
                {
                    roomName.assign((const char *)(hBlob + pos), nameLen);
                    pos += nameLen;
                }

                pos += 24; // données transform (x, y, z, rotation, etc.)

                auto sqlIt = save.sqlToCat.find(catSqlKey);
                if (sqlIt != save.sqlToCat.end())
                {
                    auto catIt = save.cats.find(sqlIt->second);
                    if (catIt != save.cats.end())
                    {
                        catIt->second.room = roomName;
                        catIt->second.inHouse = !roomName.empty();
                        if (catIt->second.inHouse)
                            inHouseCount++;
                        else
                            outsideCount++;
                    }
                }
            }

            Log("[HOUSE] State loaded: %zu in-house, %zu outside (total %u in house_state)",
                inHouseCount, outsideCount, count);
        }
    }
    if (houseStmt)
        sqlite3_finalize(houseStmt);

    // Mettre à jour les statuts dead et coi de chaque chat
    size_t livingCount = 0;
    size_t inHouseTotal = 0;
    size_t outsideTotal = 0;
    size_t deadCount = 0;

    for (auto &[id, cat] : save.cats)
    {
        bool isAlive = (save.activeCats.find(cat.sqlKey) != save.activeCats.end());
        cat.dead = !isAlive;

        if (isAlive)
        {
            livingCount++;
            if (cat.inHouse)
                inHouseTotal++;
            else
                outsideTotal++;
        }
        else
        {
            deadCount++;
        }

        auto pedIt = save.pedigree.find(cat.sqlKey);
        if (pedIt != save.pedigree.end())
        {
            cat.coi = pedIt->second.coi;
        }
    }

    Log("[STATUS] Cats summary: %zu living (%zu in-house, %zu outside), %zu deceased",
        livingCount, inHouseTotal, outsideTotal, deadCount);

    sqlite3_close(db);

    Log(
        "[CatManager] Cats parsed=%zu",
        save.cats.size());

    return save;
}