#include "../include/dbm/ParamPricedDBM.h"

#include <cstdint>
#include <vector>
#include <iostream>


int main()
{

    /*
    // EXAMPLE 1
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
    uint32_t target = 2;

    // x - x0 >= 3
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
    */


    /*
    // EXAMPLE 2
    // the goal of what follows is to modelize the following automaton:  --> (0, +p1) ------------> (1, 2) ------>  (2, 3) --------> (4, 0)
    //                                                                                  x <= 1        /\              /     x >= 3
    //                                                                                  y <- 0          \            /
    //                                                                                                   \          /
    //                                                                                                    \        /
    //                                                                                                     \     \/
    //                                                                                                     (3, +p2)
    //                                                                                                     |y <= 1|
    uint32_t nbLocations = 5;
    uint32_t initLocation = 0;
    uint32_t nbClocks = 2;
    uint32_t nbParam = 2;
    uint32_t target = 4;

    // x - x0 <= 1
    // x - x0 >= 3
    auto guard1 = PPDBM::DbmConstraint{1,0,PPDBM::DbmBound{1,false}};
    auto guard2 = PPDBM::DbmConstraint{0,1,PPDBM::DbmBound{-3,false}};
    auto guards1 = std::vector<PPDBM::DbmConstraint>{};
    auto guards2 = std::vector<PPDBM::DbmConstraint>{};
    guards1.emplace_back(guard1);
    guards2.emplace_back(guard2);

    // default reset
    uint32_t clockToReset = 0;

    // edges
    auto defaultCost = std::vector<uint32_t>(nbParam + 1,0);
    auto edge01 = PPDBM::Edge{0,1,2,guards1,defaultCost};
    auto edge12 = PPDBM::Edge{1,2,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge23 = PPDBM::Edge{2,3,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge31 = PPDBM::Edge{3,1,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge24 = PPDBM::Edge{2,4,clockToReset,guards2,defaultCost};
    auto edges = std::vector<PPDBM::Edge>{edge01,edge12, edge23, edge31, edge24};

    // invariants
    auto inv = PPDBM::Matrix<PPDBM::DbmBound>(nbLocations,nbClocks + 1,PPDBM::INF_BOUND);
    inv(3,2) = PPDBM::DbmBound{1,false};

    // costs
    auto costs = PPDBM::Matrix<uint32_t>(nbLocations,nbParam+1,0);
    costs(0,1) = 1;
    costs(1,0) = 2;
    costs(2,0) = 3;
    costs(3,2) = 1;

    // automaton
    auto automaton = PPDBM::ParamPricedTimedAutomata{nbLocations,initLocation,nbClocks,nbParam,edges,inv,costs};
    */


    /*
    // EXAMPLE 3                                                                          p1
    // the goal of what follows is to modelize the following automaton:  --> (0, 0) -----------> (1, 0)
    //                                                                              \            /\
    //                                                                               \__________/
    //                                                                                    p2
    uint32_t nbLocations = 2;
    uint32_t initLocation = 0;
    uint32_t nbClocks = 1;
    uint32_t nbParam = 2;
    uint32_t target = 1;

    // no guards
    auto guards = std::vector<PPDBM::DbmConstraint>{};

    // default reset
    uint32_t clockToReset = 0;

    // edges
    auto cost1 = std::vector<uint32_t>{0,1,0};
    auto cost2 = std::vector<uint32_t>{0,0,1};
    auto edge01_1 = PPDBM::Edge{0,1,clockToReset,guards,cost1};
    auto edge01_2 = PPDBM::Edge{0,1,clockToReset,guards,cost2};
    auto edges = std::vector<PPDBM::Edge>{edge01_1,edge01_2};

    // invariants
    auto inv = PPDBM::Matrix<PPDBM::DbmBound>(nbLocations,nbClocks + 1,PPDBM::INF_BOUND);

    // costs
    auto costs = PPDBM::Matrix<uint32_t>(nbLocations,nbParam+1,0);

    // automaton
    auto automaton = PPDBM::ParamPricedTimedAutomata{nbLocations,initLocation,nbClocks,nbParam,edges,inv,costs};
    */


    /*
    // EXAMPLE 4
    // the goal of what follows is to modelize the following automaton:  --> (0, +p1) ------------> (1, +p2) ------>  (2, 3) --------> (4, 0)
    //                                                                                  x <= 1        /\              /     x >= 3
    //                                                                                  y <- 0          \            /
    //                                                                                                   \          /
    //                                                                                                    \        /
    //                                                                                                     \     \/
    //                                                                                                     (3, 3)
    //                                                                                                     |y <= 1|
    uint32_t nbLocations = 5;
    uint32_t initLocation = 0;
    uint32_t nbClocks = 2;
    uint32_t nbParam = 2;
    uint32_t target = 4;

    // x - x0 <= 1
    // x - x0 >= 3
    auto guard1 = PPDBM::DbmConstraint{1,0,PPDBM::DbmBound{1,false}};
    auto guard2 = PPDBM::DbmConstraint{0,1,PPDBM::DbmBound{-3,false}};
    auto guards1 = std::vector<PPDBM::DbmConstraint>{};
    auto guards2 = std::vector<PPDBM::DbmConstraint>{};
    guards1.emplace_back(guard1);
    guards2.emplace_back(guard2);

    uint32_t clockToReset = 0;

    auto defaultCost = std::vector<uint32_t>(nbParam + 1,0);
    auto edge01 = PPDBM::Edge{0,1,2,guards1,defaultCost};       // reset y (clock 2)
    auto edge12 = PPDBM::Edge{1,2,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge23 = PPDBM::Edge{2,3,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge31 = PPDBM::Edge{3,1,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge24 = PPDBM::Edge{2,4,clockToReset,guards2,defaultCost};
    auto edges = std::vector<PPDBM::Edge>{edge01,edge12, edge23, edge31, edge24};

    auto inv = PPDBM::Matrix<PPDBM::DbmBound>(nbLocations,nbClocks + 1,PPDBM::INF_BOUND);
    inv(3,2) = PPDBM::DbmBound{1,false};

    // costs
    auto costs = PPDBM::Matrix<uint32_t>(nbLocations,nbParam+1,0);
    costs(0,1) = 1;
    costs(1,2) = 1;
    costs(2,0) = 3;
    costs(3,0) = 3;

    auto automaton = PPDBM::ParamPricedTimedAutomata{nbLocations,initLocation,nbClocks,nbParam,edges,inv,costs};
    */



    // EXAMPLE 5 (variante de EXAMPLE 4 avec des taux affines à 2 paramètres)
    // the goal of what follows is to modelize the following automaton:  --> (0, p1 + p2) ----------> (1, 2p1 + 3) ------> (2, p2 + 2) --------> (4, 0)
    //                                                                                       x <= 1        /\                /           x >= 3
    //                                                                                       y <- 0          \              /
    //                                                                                                        \            /
    //                                                                                                         \          /
    //                                                                                                          \       \/
    //                                                                                                          (3, p2 + 2)
    //                                                                                                            |y <= 1|
    uint32_t nbLocations = 5;
    uint32_t initLocation = 0;
    uint32_t nbClocks = 2;
    uint32_t nbParam = 2;
    uint32_t target = 4;

    auto guard1 = PPDBM::DbmConstraint{1,0,PPDBM::DbmBound{1,false}};   // x - x0 <= 1
    auto guard2 = PPDBM::DbmConstraint{0,1,PPDBM::DbmBound{-3,false}};  // x - x0 >= 3
    auto guards1 = std::vector<PPDBM::DbmConstraint>{guard1};
    auto guards2 = std::vector<PPDBM::DbmConstraint>{guard2};

    uint32_t clockToReset = 0;

    auto defaultCost = std::vector<uint32_t>(nbParam + 1,0);
    auto edge01 = PPDBM::Edge{0,1,2,guards1,defaultCost};       // reset y (clock 2)
    auto edge12 = PPDBM::Edge{1,2,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge23 = PPDBM::Edge{2,3,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge31 = PPDBM::Edge{3,1,clockToReset,std::vector<PPDBM::DbmConstraint>{},defaultCost};
    auto edge24 = PPDBM::Edge{2,4,clockToReset,guards2,defaultCost};
    auto edges = std::vector<PPDBM::Edge>{edge01,edge12, edge23, edge31, edge24};

    auto inv = PPDBM::Matrix<PPDBM::DbmBound>(nbLocations,nbClocks + 1,PPDBM::INF_BOUND);
    inv(3,2) = PPDBM::DbmBound{1,false};

    // costs(loc, k) = coefficient de p_k dans le taux de "loc" (k=0 -> terme constant)
    auto costs = PPDBM::Matrix<uint32_t>(nbLocations,nbParam+1,0);
    // loc0 : taux = p1 + p2
    costs(0,1) = 1;
    costs(0,2) = 1;
    // loc1 : taux = 2*p1 + 3
    costs(1,0) = 3;
    costs(1,1) = 2;
    // loc2 : taux = p2 + 2
    costs(2,0) = 2;
    costs(2,2) = 1;
    // loc3 : taux = p2 + 2 (identique à loc2, pour que la boucle 1->2->3->1 reste neutre en coût)
    costs(3,0) = 2;
    costs(3,2) = 1;

    auto automaton = PPDBM::ParamPricedTimedAutomata{nbLocations,initLocation,nbClocks,nbParam,edges,inv,costs};


    // algo:
    auto RES = std::vector<PPDBM::Ppdbm>{};
    auto PASSED = std::vector<std::pair<uint32_t,PPDBM::Ppdbm>>{};
    auto WAITING = std::vector<std::pair<uint32_t,PPDBM::Ppdbm>>{std::make_pair(initLocation,PPDBM::Ppdbm{nbClocks + 1,nbParam,true})};
    auto [location, initPpdbm] = WAITING[0];
    for (auto& ppdbm : initPpdbm.post_delta(automaton.getCostFunction(location), automaton.getInvariants(location), true)) {
        WAITING.emplace_back(location,ppdbm);
    }

    while (!WAITING.empty()) {
        auto symbolicState = WAITING.back();
        WAITING.pop_back();

        if (!symbolicState.second.getPC().isEmpty() && !PPDBM::Ppdbm::alreadyCoveredBy(symbolicState,PASSED)) {
            PASSED.emplace_back(symbolicState);
            auto successors = PPDBM::Ppdbm::post(symbolicState, automaton, target);
            for (auto& successor : successors) {
                WAITING.emplace_back(successor);
            }
            if (symbolicState.first == target) {
                RES.emplace_back(symbolicState.second);
            }
        }
    }


    // results displaying
    RES = PPDBM::Ppdbm::pruneRedundant(std::move(RES));
    for (auto& res : RES) res.getPC().minimize();

    auto candidates = PPDBM::buildCandidates(RES);
    auto optimal = PPDBM::resolveOptimalPartition(candidates);
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


// TODO: parser to generate / read files (accept the same files as cora to use the same benchmark)
// make more small examples (take the paper one)
// manipulate the search strategy (template and have fun) heuristic search: priority queue
// files

// input: <ParamPricedTimedautomata = (), target set of locations T, initialParameterConstraints (interval of integers, lower and upper bound)>

// output: Partition of N^nbParam, with for each part the corresponding minimum parametric cost function.

// write the paper
// clean CORA
// write a parser
// Do tests with xml files from CORA
// Create a benchmark (something that makes sense) (take an already existing example and add parameters manually)