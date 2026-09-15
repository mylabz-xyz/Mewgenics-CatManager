#include "PedigreeParser.h"

#include "../Model/SaveData.h"
#include "../../Logger.h"

#include "../../sqlite3.h"

#include <cstring>
#include <cstdint>

void LoadPedigree(
    sqlite3 *db,
    SaveData &save)
{
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "SELECT data FROM files WHERE key='pedigree';",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        Log("[PED] failed loading pedigree");
        return;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        Log("[PED] no pedigree entry");
        return;
    }

    const unsigned char *blob =
        (const unsigned char *)sqlite3_column_blob(stmt, 0);

    int size =
        sqlite3_column_bytes(stmt, 0);

    Log("[PED] Pedigree blob size=%d bytes", size);

    if (!blob || size < 24)
    {
        sqlite3_finalize(stmt);
        return;
    }

    size_t totalBytes = (size_t)size;
    size_t offset = 0;

    // =========================================================================
    // Table 0 : Pedigree Nodes (absl::flat_hash_map<int64_t, PedigreeNode>)
    // Slot size = 32 bytes: [int64 id][int64 parentA][int64 parentB][double coi]
    // =========================================================================
    if (offset + 24 > totalBytes)
    {
        Log("[PED] Buffer too small for Table 0 header");
        sqlite3_finalize(stmt);
        return;
    }

    int64_t magic0 = 0;
    int64_t numElements0 = 0;
    int64_t capacityMask0 = 0;

    memcpy(&magic0, blob + offset, 8);
    memcpy(&numElements0, blob + offset + 8, 8);
    memcpy(&capacityMask0, blob + offset + 16, 8);

    Log("[PED] Table0 magic=%lld size=%lld capacityMask=%lld", magic0, numElements0, capacityMask0);

    if (magic0 != -11 || capacityMask0 <= 0)
    {
        Log("[PED] Invalid Table 0 magic=%lld mask=%lld", magic0, capacityMask0);
        sqlite3_finalize(stmt);
        return;
    }

    size_t cap0 = (size_t)capacityMask0;
    size_t ctrlStart0 = offset + 24;
    size_t ctrlSize0 = cap0 + 17;
    size_t slotsStart0 = ctrlStart0 + ctrlSize0;
    size_t tableSize0 = 24 + ctrlSize0 + cap0 * 32 + 8;

    if (offset + tableSize0 > totalBytes)
    {
        Log("[PED] Buffer too small for Table 0 (needs %zu, total %zu)", offset + tableSize0, totalBytes);
        sqlite3_finalize(stmt);
        return;
    }

    const uint8_t *ctrl0 = blob + ctrlStart0;

    for (size_t i = 0; i < cap0; i++)
    {
        if ((ctrl0[i] & 0x80) == 0)
        {
            size_t slotOffset = slotsStart0 + i * 32;

            int64_t id = 0;
            int64_t parentA = -1;
            int64_t parentB = -1;
            double coi = 0.0;

            memcpy(&id, blob + slotOffset, 8);
            memcpy(&parentA, blob + slotOffset + 8, 8);
            memcpy(&parentB, blob + slotOffset + 16, 8);
            memcpy(&coi, blob + slotOffset + 24, 8);

            PedigreeNode node;
            node.id = id;
            node.parentA = parentA;
            node.parentB = parentB;
            node.coi = coi;

            save.pedigree[id] = node;
        }
    }

    Log("[PED] Table0 valid nodes=%zu", save.pedigree.size());

    // Validation logs pour des nœuds connus
    int64_t testIds[] = {659, 698, 650, 4};
    for (int64_t tid : testIds)
    {
        auto it = save.pedigree.find(tid);
        if (it != save.pedigree.end())
        {
            Log("[PED] Node id=%lld parentA=%lld parentB=%lld coi=%f",
                it->second.id,
                it->second.parentA,
                it->second.parentB,
                it->second.coi);
        }
    }

    // =========================================================================
    // Table 1 : Relationship Matrix (absl::flat_hash_map<pair<int64, int64>, double>)
    // Slot size = 24 bytes: [int64 catA][int64 catB][double relationship]
    // =========================================================================
    size_t offset1 = offset + tableSize0;

    if (offset1 + 24 <= totalBytes)
    {
        int64_t magic1 = 0;
        int64_t numElements1 = 0;
        int64_t capacityMask1 = 0;

        memcpy(&magic1, blob + offset1, 8);
        memcpy(&numElements1, blob + offset1 + 8, 8);
        memcpy(&capacityMask1, blob + offset1 + 16, 8);

        Log("[PED] Table1 magic=%lld size=%lld capacityMask=%lld", magic1, numElements1, capacityMask1);

        if (magic1 == -11 && capacityMask1 > 0)
        {
            size_t cap1 = (size_t)capacityMask1;
            size_t ctrlStart1 = offset1 + 24;
            size_t ctrlSize1 = cap1 + 17;
            size_t slotsStart1 = ctrlStart1 + ctrlSize1;
            size_t tableSize1 = 24 + ctrlSize1 + cap1 * 24 + 8;

            if (offset1 + tableSize1 <= totalBytes)
            {
                const uint8_t *ctrl1 = blob + ctrlStart1;

                for (size_t i = 0; i < cap1; i++)
                {
                    if ((ctrl1[i] & 0x80) == 0)
                    {
                        size_t slotOffset = slotsStart1 + i * 24;

                        int64_t catA = 0;
                        int64_t catB = 0;
                        double rel = 0.0;

                        memcpy(&catA, blob + slotOffset, 8);
                        memcpy(&catB, blob + slotOffset + 8, 8);
                        memcpy(&rel, blob + slotOffset + 16, 8);

                        save.relationships.push_back({catA, catB, rel});
                    }
                }

                Log("[PED] Table1 valid entries=%zu", save.relationships.size());

                // =====================================================================
                // Table 2 : Active / Living Cats (absl::flat_hash_set<int64_t>)
                // Slot size = 8 bytes: [int64 catSqlKey]
                // =====================================================================
                size_t offset2 = offset1 + tableSize1;

                if (offset2 + 24 <= totalBytes)
                {
                    int64_t magic2 = 0;
                    int64_t numElements2 = 0;
                    int64_t capacityMask2 = 0;

                    memcpy(&magic2, blob + offset2, 8);
                    memcpy(&numElements2, blob + offset2 + 8, 8);
                    memcpy(&capacityMask2, blob + offset2 + 16, 8);

                    Log("[PED] Table2 magic=%lld size=%lld capacityMask=%lld", magic2, numElements2, capacityMask2);

                    if (magic2 == -11 && capacityMask2 > 0)
                    {
                        size_t cap2 = (size_t)capacityMask2;
                        size_t ctrlStart2 = offset2 + 24;
                        size_t ctrlSize2 = cap2 + 17;
                        size_t slotsStart2 = ctrlStart2 + ctrlSize2;
                        size_t tableSize2 = 24 + ctrlSize2 + cap2 * 8 + 8;

                        if (offset2 + tableSize2 <= totalBytes)
                        {
                            const uint8_t *ctrl2 = blob + ctrlStart2;

                            for (size_t i = 0; i < cap2; i++)
                            {
                                if ((ctrl2[i] & 0x80) == 0)
                                {
                                    size_t slotOffset = slotsStart2 + i * 8;

                                    int64_t activeSqlKey = 0;
                                    memcpy(&activeSqlKey, blob + slotOffset, 8);

                                    save.activeCats.insert(activeSqlKey);
                                }
                            }

                            Log("[PED] Table2 active cats=%zu", save.activeCats.size());
                        }
                        else
                        {
                            Log("[PED] Buffer too small for Table 2");
                        }
                    }
                }
            }
            else
            {
                Log("[PED] Buffer too small for Table 1");
            }
        }
    }

    sqlite3_finalize(stmt);
}