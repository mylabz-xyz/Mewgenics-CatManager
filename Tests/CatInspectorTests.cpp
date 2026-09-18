#include "TestUtils.h"

#include "../Native/Save/Analysis/CatInspector.h"

#include <iostream>

void RunCatInspectorTests()
{
    SaveData save{};

    // ------------------------------------------------------------
    // Synthetic pedigree
    //
    // Archer -> child of Grimnir
    //
    //            Gat
    //           /   \
    //      Sluggie  Miles
    //           \   /
    //          Juanita
    //
    // Sluggie and Miles share both parents.
    // ------------------------------------------------------------

    CatData archer =
        MakeCat(1, 625, "Archer");

    CatData grimnir =
        MakeCat(2, 613, "Grimnir");

    CatData gat =
        MakeCat(3, 697, "Gat");

    CatData juanita =
        MakeCat(4, 644, "Juanita");

    CatData sluggie =
        MakeCat(5, 717, "Sluggie");

    CatData miles =
        MakeCat(6, 718, "Miles");

    // Archer -> Grimnir
    archer.parentAId = grimnir.id;
    grimnir.children.push_back(archer.id);

    // Sluggie -> Gat + Juanita
    sluggie.parentAId = gat.id;
    sluggie.parentBId = juanita.id;

    // Miles -> Gat + Juanita
    miles.parentAId = gat.id;
    miles.parentBId = juanita.id;

    gat.children.push_back(sluggie.id);
    gat.children.push_back(miles.id);

    juanita.children.push_back(sluggie.id);
    juanita.children.push_back(miles.id);

    save.cats.emplace(archer.id, archer);
    save.cats.emplace(grimnir.id, grimnir);
    save.cats.emplace(gat.id, gat);
    save.cats.emplace(juanita.id, juanita);
    save.cats.emplace(sluggie.id, sluggie);
    save.cats.emplace(miles.id, miles);

    // ------------------------------------------------------------
    // Basic lookup
    // ------------------------------------------------------------

    Check(
        FindCat(save, archer.id) != nullptr,
        "FindCat should find Archer");

    Check(
        FindCat(save, 999999) == nullptr,
        "FindCat should return nullptr for an unknown ID");

    // ------------------------------------------------------------
    // Parent / child
    // ------------------------------------------------------------

    const CatData *archerParent =
        GetParentA(save, archer);

    Check(
        archerParent != nullptr,
        "Archer should have a parent");

    Check(
        archerParent &&
            archerParent->id == grimnir.id,
        "Archer parent should be Grimnir");

    const std::vector<const CatData *> grimnirChildren =
        GetChildren(save, grimnir);

    Check(
        grimnirChildren.size() == 1,
        "Grimnir should have one child");

    Check(
        !grimnirChildren.empty() &&
            grimnirChildren[0]->id == archer.id,
        "Grimnir child should be Archer");

    // ------------------------------------------------------------
    // Parent / child relationship classification
    // ------------------------------------------------------------

    const CatRelationship parentRelationship =
        AnalyzeRelationship(
            save,
            archer,
            grimnir,
            6);

    Check(
        parentRelationship.type ==
            CatRelationshipType::Parent,
        "Archer -> Grimnir should be classified as Parent");

    Check(
        parentRelationship.isParent,
        "Archer -> Grimnir should set isParent");

    const CatRelationship childRelationship =
        AnalyzeRelationship(
            save,
            grimnir,
            archer,
            6);

    Check(
        childRelationship.type ==
            CatRelationshipType::Child,
        "Grimnir -> Archer should be classified as Child");

    Check(
        childRelationship.isChild,
        "Grimnir -> Archer should set isChild");

    // ------------------------------------------------------------
    // Full siblings
    // ------------------------------------------------------------

    const CatRelationship siblingRelationship =
        AnalyzeRelationship(
            save,
            sluggie,
            miles,
            6);

    Check(
        siblingRelationship.type ==
            CatRelationshipType::FullSibling,
        "Sluggie and Miles should be FullSibling");

    Check(
        siblingRelationship.sharedParents.size() == 2,
        "Sluggie and Miles should share two parents");

    // ------------------------------------------------------------
    // Common ancestors
    // ------------------------------------------------------------

    const std::vector<CommonAncestor> commonAncestors =
        GetCommonAncestors(
            save,
            sluggie,
            miles,
            6);

    Check(
        commonAncestors.size() == 2,
        "Sluggie and Miles should have exactly two common ancestors");

    bool foundGat = false;
    bool foundJuanita = false;

    for (const CommonAncestor &ancestor : commonAncestors)
    {
        if (!ancestor.cat)
            continue;

        if (ancestor.cat->sqlKey == gat.sqlKey)
            foundGat = true;

        if (ancestor.cat->sqlKey == juanita.sqlKey)
            foundJuanita = true;
    }

    Check(
        foundGat,
        "Gat should be a common ancestor");

    Check(
        foundJuanita,
        "Juanita should be a common ancestor");

    // ------------------------------------------------------------
    // Half sibling
    // ------------------------------------------------------------

    CatData halfSibling =
        MakeCat(7, 719, "HalfSibling");

    halfSibling.parentAId = gat.id;
    halfSibling.parentBId = 0;

    gat.children.push_back(halfSibling.id);

    save.cats.emplace(
        halfSibling.id,
        halfSibling);

    const CatRelationship halfSiblingRelationship =
        AnalyzeRelationship(
            save,
            sluggie,
            halfSibling,
            6);

    Check(
        halfSiblingRelationship.type ==
            CatRelationshipType::HalfSibling,
        "Sluggie and HalfSibling should be HalfSibling");

    Check(
        halfSiblingRelationship.sharedParents.size() == 1,
        "Sluggie and HalfSibling should share exactly one parent");

    // ------------------------------------------------------------
    // Unrelated
    // ------------------------------------------------------------

    CatData unrelated =
        MakeCat(8, 720, "Unrelated");

    save.cats.emplace(
        unrelated.id,
        unrelated);

    const CatRelationship unrelatedRelationship =
        AnalyzeRelationship(
            save,
            sluggie,
            unrelated,
            6);

    Check(
        unrelatedRelationship.type ==
            CatRelationshipType::Unrelated,
        "Sluggie and Unrelated should be Unrelated");

    Check(
        unrelatedRelationship.sharedParents.empty(),
        "Unrelated cats should have no shared parents");

    Check(
        unrelatedRelationship.commonAncestors.empty(),
        "Unrelated cats should have no common ancestors");

    // ------------------------------------------------------------
    // Other relationship
    //
    // Two cats share an ancestor, but are not siblings.
    // ------------------------------------------------------------

    CatData otherParent =
        MakeCat(9, 721, "OtherParent");

    CatData other =
        MakeCat(10, 722, "Other");

    otherParent.parentAId = gat.id;
    other.parentBId = 0;

    other.parentAId = otherParent.id;
    other.parentBId = 0;

    otherParent.children.push_back(other.id);

    save.cats.emplace(
        otherParent.id,
        otherParent);

    save.cats.emplace(
        other.id,
        other);

    const CatRelationship otherRelationship =
        AnalyzeRelationship(
            save,
            sluggie,
            other,
            6);

    Check(
        otherRelationship.type ==
            CatRelationshipType::Other,
        "Sluggie and Other should be Other");

    Check(
        !otherRelationship.commonAncestors.empty(),
        "Other relationship should have a common ancestor");
        
    std::cout
        << "[PASS] CatInspectorTests"
        << '\n';
}