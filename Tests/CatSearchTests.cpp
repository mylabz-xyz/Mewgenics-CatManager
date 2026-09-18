#include "TestUtils.h"

#include "../Native/Save/Analysis/CatInspector.h"

#include <iostream>
#include <vector>

void RunCatSearchTests()
{
    SaveData save;

    CatData searchAlive =
        MakeCat(
            11,
            723,
            "SearchTarget");

    CatData searchDead =
        MakeCat(
            12,
            724,
            "DeadSearchTarget");

    searchDead.dead = true;

    CatData searchOther =
        MakeCat(
            13,
            725,
            "AnotherCat");

    save.cats.emplace(
        searchAlive.id,
        searchAlive);

    save.cats.emplace(
        searchDead.id,
        searchDead);

    save.cats.emplace(
        searchOther.id,
        searchOther);

    // Partial search must be case-insensitive and ignore dead cats.
    const std::vector<CatSearchResult> searchResults =
        SearchCats(
            save,
            "search");

    Check(
        searchResults.size() == 1,
        "Search should exclude dead cats");

    Check(
        searchResults[0].cat->id == searchAlive.id,
        "Search should find the living matching cat");

    Check(
        !searchResults[0].exactMatch,
        "Partial search should not be exact");

    // Exact name matches should be reported as exact.
    const std::vector<CatSearchResult> exactResults =
        SearchCats(
            save,
            "SearchTarget");

    Check(
        exactResults.size() == 1,
        "Exact search should find the living cat");

    Check(
        exactResults[0].cat->id == searchAlive.id,
        "Exact search should return SearchTarget");

    Check(
        exactResults[0].exactMatch,
        "Exact search should be marked as exact");

    // Search must not depend on the capitalization used by the player.
    const std::vector<CatSearchResult> caseInsensitiveResults =
        SearchCats(
            save,
            "SEARCHTARGET");

    Check(
        caseInsensitiveResults.size() == 1,
        "Search should be case-insensitive");

    Check(
        caseInsensitiveResults[0].cat->id == searchAlive.id,
        "Case-insensitive search should find SearchTarget");

    const std::vector<CatSearchResult> missingResults =
        SearchCats(
            save,
            "DoesNotExist");

    Check(
        missingResults.empty(),
        "Unknown cat search should return no results");

    std::cout
        << "[PASS] CatSearchTests\n";
}