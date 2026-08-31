#include "../include/dbm/ParamPricedDBM.h"

#include <cstdint>
#include <vector>
#include <iostream>


int main()
{
    // the goal of what follows is to modelize the following automaton:  --> (0, +p1) ------> (1,+p2)
    //                                                                           \              /
    //                                                                      x>=3  \            /  x>=3
    //                                                                             \          /
    //                                                                              \        /
    //                                                                               \/    \/
    //                                                                                (2,+0)
    uint32_t nbLocations = 3;
    uint32_t initLocation = 0;
    uint32_t nbClocks = 1;
    uint32_t nbParam = 2;

    // x - x0 <= 3
    auto guard = PPDBM::DbmConstraint{0,1,PPDBM::DbmBound{-3,false}};
    auto guards = std::vector<PPDBM::DbmConstraint>{};
    guards.emplace_back(guard);

    // resets (none for the example)
    uint32_t clockToReset = 0;

    // edges
    auto defaultCost = std::vector<uint32_t>(nbParam + 1,0);
    auto edge01 = PPDBM::Edge{0,1,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge02 = PPDBM::Edge{0,2,clockToReset,guards,defaultCost};
    auto edge12 = PPDBM::Edge{1,2,clockToReset,guards,defaultCost};
    auto edges = std::vector<PPDBM::Edge>{edge01,edge02, edge12};

    // invariants (none for the example)
    auto inv = PPDBM::Matrix<PPDBM::DbmBound>(nbLocations,nbClocks + 1,PPDBM::INF_BOUND);

    // costs
    auto costs = PPDBM::Matrix<uint32_t>(nbLocations,nbParam+1,0);
    costs(0,1) = 1;
    costs(1,2) = 1;

    // automaton
    auto automaton = PPDBM::ParamPricedTimedAutomata{nbLocations,initLocation,nbClocks,nbParam,edges,inv,costs};

    // algo:
    auto RES = std::vector<PPDBM::Ppdbm>{};
    auto PASSED = std::vector<std::pair<uint32_t,PPDBM::Ppdbm>>{};
    auto WAITING = std::vector<std::pair<uint32_t,PPDBM::Ppdbm>>{std::make_pair(initLocation,PPDBM::Ppdbm{nbClocks + 1,nbParam,true})};
    auto [location, initPpdbm] = WAITING[0];
    for (auto ppdbm : initPpdbm.post_delta(automaton.getCostFunction(location), automaton.getInvariants(location))) {
        WAITING.emplace_back(location,ppdbm);
    }

    while (WAITING.size()>0) {
        auto symbolicState = WAITING.back();
        WAITING.pop_back();

        if (!symbolicState.second.getPC().isEmpty() && !PPDBM::Ppdbm::alreadyCoveredBy(symbolicState,PASSED)) {
            PASSED.emplace_back(symbolicState);
            auto successors = PPDBM::Ppdbm::post(symbolicState, automaton);
            for (auto successor : successors) {
                WAITING.emplace_back(successor);
            }
            if (symbolicState.first == 2) {
                RES.emplace_back(symbolicState.second);
            }
        }
    }


    // results displaying
    /*
    // RES = PPDBM::Ppdbm::pruneRedundant(std::move(RES));
    for (auto res : RES) {
        // res.getPC().minimize();
        res.display();
    }
    */

    RES = PPDBM::Ppdbm::pruneRedundant(std::move(RES));
    for (auto& res : RES) res.getPC().minimize();

    auto optimal = PPDBM::resolveOptimalPartition(RES);
    optimal = PPDBM::OptimalRegion::pruneRedundantRegions(std::move(optimal));

    std::cout << "\n\n========== OPTIMAL PARTITION ==========\n";
    for (auto& region : optimal) {
        PPDBM::Polyhedron p;
        for (auto& c : region.constraints) p.addParametricConstraint(c);
        p.minimize();

        std::cout << "cost = [ ";
        for (auto v : region.cost) std::cout << v << " ";
        std::cout << "]\nregion:\n";
        PPDBM::Polyhedron::printPolyhedron(p.getConstraints());
        std::cout << "---------------------------------------\n";
    }

    return 0;
}

