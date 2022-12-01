/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "infimum.h"

#include "dbm/dbm_raw.hpp"
#include "dbm/mingraph.h"
#include "base/bitstring.h"
#include "debug/macros.h"

#include <algorithm>
#include <stdexcept>
#include <cstdlib>

/** b is the notation for supply/demand in the network flow literature */
static inline int32_t b(const int32_t* rates, cindex_t i) { return -rates[i]; }

/**
 * Tree structure:
 * Storing the information we need about nodes in the graph
 * spanning tree.
 */
struct Node
{
    Node* pred;        /**< Predecessor in the tree. */
    uint32_t depth;    /**< Depth from root (Always node0). */
    Node* thread;      /**< Next according to the preorder. */
    bool inbound;      /**< Is the arc pointing to me or away. */
    int32_t flow;      /**< Flow for the current solution. */
    int32_t potential; /**< Node potential for spanning tree solution. */
};

struct Tree : public std::vector<Node>
{
    using std::vector<Node>::vector;
    /** Computes the index of the given tree node */
    uint32_t index_of(const Node* n) const
    {
        assert(data() <= n);
        assert(n < data() + size());
        return static_cast<uint32_t>(n - data());
    }
    /** Computes the index of a predecessor of the given node index i. */
    [[nodiscard]] uint32_t pred_of(uint32_t i) const { return index_of((*this)[i].pred); }
    /** Computes the next according to the preorder the given node index i. */
    [[nodiscard]] uint32_t thread_of(uint32_t i) const { return index_of((*this)[i].thread); }
};

/*
 * Used to store the indices of the clock constraints
 * from the minimal constraint form.
 */
struct arc_t
{
    uint32_t i;
    uint32_t j;
};

#ifndef NDEBUG
static void printSimpleNodeInfo(const Tree& tree, const uint32_t i)
{
    std::cerr << "Index: " << i << ", pred: " << tree.pred_of(i) << ", depth: " << tree[i].depth
              << ", thread: " << tree.thread_of(i) << ",  inbound: " << tree[i].inbound << ", flow: " << tree[i].flow
              << ", potential: " << tree[i].potential;
}

static void printNodeInfo(const Tree& tree, const int32_t* rates, uint32_t i)
{
    printSimpleNodeInfo(tree, i);
    std::cerr << ", s/d: " << b(rates, i) << " ";
    std::cerr << std::endl;
}

static void printAllNodeInfo(const Tree& tree, const int32_t* rates)
{
    std::cerr << "Nodes:" << std::endl;
    for (uint32_t i = 0; i < tree.size(); ++i)
        printNodeInfo(tree, rates, i);
}
/*
static void printSimpleAllNodeInfo(const Nodes& tree, const uint32_t dim)
{
    std::cerr << "Nodes:" << std::endl;
    for (uint32_t i = 0; i < dim; i++)
    {
        printSimpleNodeInfo(tree, i);
        std::cerr << std::endl;
    }
}
*/

static bool checkTreeIntegrity(dbm::reader dbm, const int32_t* rates, const Tree& tree, const uint32_t dim)
{
    bool treeOK = true;
    // Check that all tree get their flow
    std::vector<uint32_t> sum(dim);
    uint32_t total = 0;
    sum[0] = 0;

    for (auto i = 1u; i < dim; i++) {
        sum[0] -= b(rates, i);
        total -= b(rates, i);
        sum[i] = b(rates, i);
    }
    for (auto i = 1u; i < dim; i++) {
        const auto pred_i = tree.pred_of(i);
        if (tree[i].inbound) {
            sum[i] += tree[i].flow;
            sum[pred_i] -= tree[i].flow;
        } else {
            sum[pred_i] += tree[i].flow;
            sum[i] -= tree[i].flow;
        }
    }
    for (auto i = 0u; i < dim; i++) {
        if (sum[i] != 0) {
            treeOK = false;
            std::cerr << "sum[" << i << "] is corrupted: " << sum[i]
                      << ", should be: " << (i == 0 ? total : b(rates, i)) << std::endl;
        }
    }

    // Check that the reduced costs are zero
    int32_t reduced_cost;
    for (auto i = 1u; i < dim; i++) {
        const auto pred_i = tree.pred_of(i);
        if (tree[i].potential != dbm_INFINITY || pred_i != 0) {
            if (tree[i].inbound)
                reduced_cost = dbm.at(pred_i, i) - tree[pred_i].potential + tree[i].potential;
            else
                reduced_cost = dbm.at(i, pred_i) + tree[pred_i].potential - tree[i].potential;
            if (reduced_cost != 0) {
                treeOK = false;
                std::cerr << "Reduced cost not zero (" << reduced_cost << ") i (pot): " << i << " ("
                          << tree[i].potential << "), pred(i) (pot): " << pred_i << " (" << tree[pred_i].potential
                          << "), inbound: " << tree[i].inbound << std::endl;
            }
        }
    }

    // Check depth integrity
    for (auto i = 1u; i < dim; i++) {
        const auto pred_i = tree.pred_of(i);
        if ((tree[pred_i].depth + 1) != tree[i].depth) {
            treeOK = false;
            std::cerr << "Depth of " << i << " is " << tree[i].depth << ", but depth of pred(i) " << pred_i << " is "
                      << tree[pred_i].depth << std::endl;
        }
    }

    // Check thread integrity
    std::vector<uint32_t> allVisited(bits2intsize(dim));
    base_setOneBit(allVisited.data(), 0);
    uint32_t j = tree.thread_of(0);

    for (auto i = 1u; i < dim; i++) {
        base_setOneBit(allVisited.data(), j);
        j = tree.thread_of(j);
    }
    if (j != 0) {
        treeOK = false;
        std::cerr << "Thread doesn't return to the root!! Ended with: " << j << std::endl;
        printAllNodeInfo(tree, rates);
    }
    // For each node the subtree should match the thread.
    for (auto i = 1u; i < dim; i++) {
        uint32_t size = bits2intsize(dim);
        auto thread = std::vector<uint32_t>(size, 0);
        auto subtree = std::vector<uint32_t>(size, 0);
        auto j = tree.thread_of(i);
        base_setOneBit(thread.data(), i);
        base_setOneBit(subtree.data(), i);
        // Getting the tread
        while (tree[j].depth > tree[i].depth) {
            base_setOneBit(thread.data(), j);
            j = tree.thread_of(j);
        }
        // getting the subtree
        for (auto j = 1u; j < dim; ++j) {
            uint32_t k = j;
            while (k != 0) {
                if (k == i) {
                    base_setOneBit(subtree.data(), j);
                }
                k = tree.pred_of(k);
            }
        }
        for (auto j = 0u; j < size; ++j) {
            if (thread[j] != subtree[j]) {
                treeOK = false;
                std::cerr << "Subtree of " << i << " doesn't match thread" << std::endl;
            }
        }
        for (auto i = 0u; i < dim; ++i) {
            if (ZERO == base_getOneBit(allVisited.data(), i)) {
                treeOK = false;
                std::cerr << "Node " << i << " not reach through thread!!" << std::endl;
            }
        }
    }

    return treeOK;
}

static inline bool nodeHasNoChildren(Node* node) { return (node->thread->depth > node->depth); }

static void printSolution(int32_t* valuation, const int32_t* rates, uint32_t dim)
{
    for (uint32_t i = 0; i < dim; i++) {
        std::cerr << "Val " << i << ": " << valuation[i] << " (rate: " << rates[i] << ")" << std::endl;
    }
}

static void printClockLowerBounds(dbm::reader dbm, uint32_t dim)
{
    for (uint32_t i = 1; i < dim; i++)
        std::cerr << "Lower bound " << i << ": " << -dbm.at(0, i) << std::endl;
}
/*
static void printAllConstraints(const raw_t *dbm, uint32_t dim, arc_t *arcs, uint32_t nbConstraints)
{
    for (uint32_t i = 0; i < nbConstraints; i++)
    {
        std::cerr << "Constraint " << i << ": x_" << arcs[i].i << " - x_" << arcs[i].j << " <= " <<
dbm.at(arcs[i].i, arcs[i].j) << std::endl;
    }
}

static void printAllRates(const int32_t *rates, uint32_t dim)
{
    for (uint32_t i = 0; i < dim; i++)
    {
        std::cerr << "Rate[" << i << "]: " << rates[i] << std::endl;
    }
}
*/
#endif
// END DEBUG SECTION

/*
 * Set up the initial solution by introducing artificial arcs as necessary.
 * Whenever dbm is without
 *  ---   x_0 - x_i <= c_0i and b(i) <  0, introduce x_0 - x_i <= 0;
 *  ---   x_i - x_0 <= c_i0 and b(i) >= 0, introduce x_i - x_0 <= dbm_INFINITY
 * To guarantee strongly feasible spanning trees - and thus termination - we
 * also need outbound arc from transshipment nodes!
 *
 * The initial solution includes computing the initial node potentials.
 *
 */
static void findInitialSpanningTreeSolution(dbm::reader dbm, const int32_t* rates, Tree& tree)
{
    assert(dbm.get_dim() == tree.size());
    uint32_t dim = tree.size();
    Node* last = tree.data() + dim;
    Node* node = &tree[0];
    node->pred = nullptr;
    node->depth = 0;
    node->thread = &tree[1];
    node->inbound = false;
    node->flow = -1;
    node->potential = 0;

    for (node++; node != last; node++) {
        uint32_t i = tree.index_of(node);
        node->pred = &tree[0];
        node->depth = 1;
        node->thread = &tree[(i + 1) % dim];
        node->flow = abs(rates[i]);

        if (b(rates, i) < 0) {  // -rate(i) < 0
            node->inbound = true;
            node->potential = -dbm.bound(0, i);
        } else {  // -rate(i) >= 0
            node->inbound = false;
            node->potential = dbm.bound(i, 0);
        }
    }
    // printAllNodeInfo(tree, rates, dim);
}

/*
 * Update the node potential so that alle reduced costs in the spanning
 * tree are zero. The subtree with the root is already up-to-date, so
 * it suffices to update the other subtree rooted at leave.
 *
 * Pre: leave should be on the path from arcs[enter].{i,j} to the root.
 */

static void updatePotentials(Node* leave, int32_t change)
{
    /* Update the entire subtree rooted at leave with a change
     * corresponding the reduced cost of the entering arc.  This is
     * enougn to guarantee that the reduced cost in the new spanning
     * tree remains zero. Node that the node from the entering arc
     * that is in the subtree is updated as well.
     *
     * The potential at leave is not updated, as the arc is being
     * removed anyway.
     */
    Node* z = leave;
    uint32_t d = leave->depth;
    do {
        z->potential += change;
        z = z->thread;
    } while (z->depth > d);
}

/**
 * Returns the last node according to preorder before \a exclude but
 * not before \a node.
 *
 * Node that this might be startNode itself.
 */
static Node* findLastNodeBefore(Node* node, const Node* exclude)
{
    Node* i;
    do {
        i = node;
        node = node->thread;
    } while (node != exclude);
    return i;
}

/**
 * Starting from \a node, returns the first node according to preorder
 * for which the preorder successor has a depth not higher than
 * \a depth.
 */
static Node* findLastNodeBefore(Node* node, const uint32_t depth)
{
    Node* i;
    do {
        i = node;
        node = node->thread;
    } while (node->depth > depth);
    return i;
}

/**
 * Returns the \a n'th predecessor of \a node. Returns \a node if \a n
 * is zero or negative.
 */
static Node* findNthPredecessor(Node* node, int32_t n)
{
    while (n > 0) {
        assert(node);
        node = node->pred;
        n--;
    }
    return node;
}

/**
 * Returns true if and only if n is on the path to the root from m.
 */
static inline bool isPredecessorOf(const Node* n, Node* m) { return n == findNthPredecessor(m, m->depth - n->depth); }

/**
 * Update all node information in the subtree not containing the root.
 * I.e., pred, depth, and thread.
 *
 */
static void updateNonRootSubtree(Node* rootNode, Node* nonRootNode, Node* leave, bool sourceInRootSubtree,
                                 uint32_t flow)
{
    /*
     * Update thread information NOT COMPLETELY SURE ABOUT CORRECTNESS
     * BUT CONFIDENT ... and even more after testing
     *
     * Maybe it is about time I describe the idea - to come, Gerd.
     */

    // Find node that threads to leave;
    Node* pointToLeave = findLastNodeBefore(leave->pred, leave);
    Node* lastOut = findLastNodeBefore(nonRootNode, nonRootNode->depth);
    Node* preorderOut = lastOut->thread;
    Node* i = nonRootNode;
    while (i != leave) {
        Node* prev = i;
        i = lastOut->thread = i->pred;

        /*
         * Determine the point where thread_MACRO(i) moves out of
         * i's subtrees and store the node in myPreorderOut.
         */
        lastOut = findLastNodeBefore(i, prev);

        if (i == preorderOut->pred) {
            lastOut->thread = preorderOut;
            // lastOut = findLastThreadedSubnode(nodes, preorderOut, 0);
            lastOut = findLastNodeBefore(preorderOut, i->depth);
            preorderOut = lastOut->thread;
        }
    }

    if (pointToLeave == rootNode) {
        /*
         * If rootNode points to the leave subtree in
         * preorder and is now changed to the root of
         * the leave subtree, then preorderOut should
         * remain unchanged.
         */

        rootNode->thread = nonRootNode;
        lastOut->thread = preorderOut;
    } else {
        lastOut->thread = rootNode->thread;
        rootNode->thread = nonRootNode;
        pointToLeave->thread = preorderOut;
    }

    ASSERT(preorderOut->depth <= leave->depth, std::cerr << ", pointToLeave: " << pointToLeave
                                                         << ", lastOut: " << lastOut << ", preorderOut: " << preorderOut
                                                         << ", depth(preorderOut): " << preorderOut->depth
                                                         << ". depth(leave): " << leave->depth);

    // Done updating thread information

    /*
     * Update the inbound, pred, and flow information
     * while tracing back the cycle. If (i,j) is still in the
     * tree and tree[i] had the information, then tree[j] will
     * get that information, as tree[k] will hold the information
     * about enter.
     */

    Node *tmppred1, *tmppred2, *newi;
    int32_t tmpflow1, tmpflow2;
    bool tmpinbound1, tmpinbound2;
    tmppred1 = rootNode;
    tmpflow1 = flow;
    tmpinbound1 = !sourceInRootSubtree;  // To handle the negation used later.

    newi = nonRootNode;
    do {
        i = newi;
        assert(i != nullptr);
        // These are i's new values
        tmppred2 = tmppred1;
        tmpflow2 = tmpflow1;
        tmpinbound2 = tmpinbound1;
        // Save i's old values
        newi = i->pred;
        tmppred1 = i;
        tmpflow1 = i->flow;
        tmpinbound1 = i->inbound;
        // Update i's values
        i->pred = tmppred2;
        i->flow = tmpflow2;
        i->inbound = !tmpinbound2;  // NOTE the negation of the direction.
    } while (i != leave);

    // Done updating pred, inbound, and flow information

    /*
     * Update depth information using the newly updated preorder
     * thread. We reuse preorderOut as found when updating thread.
     * Requires that pred,thread, and depth have been updated
     * correctly!
     */

    Node* stop = lastOut->thread;
    i = nonRootNode;
    do {
        i->depth = i->pred->depth + 1;
        i = i->thread;
    } while (i != stop);
}

/*
 * Update flow around the cycle introduced by enter, (k,l).
 * I.e., add flow to arcs in the direction of (k,l)
 * and subtract flow from arcs in the opposite direction
 * of (k,l).
 */
static void updateFlowInCycle(Node* k, Node* l, Node* root, int32_t flowToAugment)
{
    if (flowToAugment > 0) {
        while (k != root) {
            k->flow += k->inbound ? flowToAugment : -flowToAugment;
            ASSERT(k->flow >= 0, std::cerr << "flow(" << k << "): " << k->flow << std::endl);
            k = k->pred;
        }
        while (l != root) {
            l->flow += l->inbound ? -flowToAugment : flowToAugment;
            ASSERT(l->flow >= 0, std::cerr << "flow(" << l << "): " << l->flow << std::endl);
            l = l->pred;
        }
    }
}

/**
 * Update the spanning tree so the node information is updated
 * including flow and node potentials.
 */
static void updateSpanningTree(Node* k, Node* l, Node* leave, Node* root, int32_t costEnter)
{
    int32_t reducedCostEnter = costEnter - k->potential + l->potential;
    int32_t flowToAugment = leave->flow;
    updateFlowInCycle(k, l, root, flowToAugment);
    if (!isPredecessorOf(leave, k)) {
        updatePotentials(leave, -reducedCostEnter);
        updateNonRootSubtree(k, l, leave, true, flowToAugment);
    } else {
        updatePotentials(leave, reducedCostEnter);
        updateNonRootSubtree(l, k, leave, false, flowToAugment);
    }
}

/*
 * ENTERING ARC FUNCTION REQUIREMENTS
 *
 * Any entering arc function should return nullptr to indicate that no
 * eligible arc exists, thus, signifiying optimality of the current
 * solution. Otherwise an arc in [firstArc, lastArc) is returned.
 */

/**
 * Danzig's entering rule chooses the eligible arc with the
 * lowest cost.
 */
static auto enteringArcDanzig(const std::vector<arc_t>& arcs, const Tree& tree, dbm::reader dbm)
{
    auto best = arcs.end();
    int32_t lowest_reduced_cost = 0;
    for (auto it = arcs.begin(); it != arcs.end(); ++it) {
        /* Check if reduced cost is negative, i.e.  c_ij - pi(i) +
         * pi(j) < 0 for any arcs in arcs */
        int32_t reduced_cost = dbm.at(it->i, it->j) - tree[it->i].potential + tree[it->j].potential;

        if (reduced_cost < lowest_reduced_cost) {
            lowest_reduced_cost = reduced_cost;
            best = it;
        }
    }
    assert(lowest_reduced_cost < 0 || best == std::end(arcs));

    return best;
}

// static arc_t *enteringArcFirstEligible(
//     arc_t *firstArc, arc_t *lastArc,
//     Node *tree, const raw_t *dbm, uint32_t dim)
// {
//     while (firstArc != lastArc)
//     {
//         /* Check if reduced cost is negative, i.e.  c_ij - pi(i) +
//          * pi(j) < 0 for any arcs in arcs.
//          */
//         int32_t reduced_cost = dbm.at(firstArc->i, firstArc->j)
//             - potential(firstArc->i) + potential(firstArc->j);
//         if (reduced_cost < 0)
//         {
//             return firstArc;
//         }
//         firstArc++;
//     }
//     return nullptr;
// }

/**
 * Returns the common ancestor of nodes \a k and \a l with the biggest
 * depth.
 */
static Node* discoverCycleRoot(Node* k, Node* l)
{
    int32_t diff = k->depth - l->depth;
    k = findNthPredecessor(k, diff);
    l = findNthPredecessor(l, -diff);
    while (k != l) {
        k = k->pred;
        l = l->pred;
    }
    return k;
}

/*
 * @param (\a k, \a l) the entering arc
 * @param \a root is discoverCycleRoot(k, l);
 *
 * Find first blocking arc if we augment flow in the direction of
 * (k,l). I.e. the arc oppositely directed of (k,l) with the lowest
 * flow. In case of a tie, choose the last arc from root in the
 * direction of (k,l) to maintain a strongly feasible spanning tree.
 *
 * If no such arc exists it means that the solution is unbounded, which in
 * our case of zones cant happen as all priced zones have an infimum cost.
 *
 * Actually, the node in the spanning tree representation that mentions the
 * the arc is returned. This is to compensate for the fact that it could be
 * an artificial arc which is represented by nullptr.
 */
static Node* findLeavingArc(Node* k, Node* l, Node* root)
{
    int32_t smallestFlow = INT_MAX;
    /*
     * Node with the lowest depth of the leaving arc.
     */
    Node* smallestFlowNode = nullptr;

    /*
     * Move towards the common root to find the
     * arc in the opposite direction of (k,l) with
     * the smallest flow.
     */
    while (k != root) {
        if (!k->inbound)  // If not inbound then opposite of (i,j)
        {
            if (k->flow < smallestFlow)  // < since we need the last along (i,j)
            {
                smallestFlow = k->flow;
                smallestFlowNode = k;
            }
        }
        k = k->pred;
    }
    while (l != root) {
        if (l->inbound)  // If inbound then opposite of (i,j)
        {
            if (l->flow <= smallestFlow)  // <= since we need the last along (i,j)
            {
                smallestFlow = l->flow;
                smallestFlowNode = l;
            }
        }
        l = l->pred;
    }
    ASSERT(smallestFlow != INT_MAX, std::cerr << "No oppositely directed arcs" << std::endl);

    return smallestFlowNode;
}

/* Verify that we have no artificial arcs involved, we know
 * they have zero flow, but they distrub the node potentials.
 * Not in the sense that the solution violates the constraints,
 * but it is in an artificial corner point generated by dbm_INFINITY
 * Since we want a trace through corner point, we will change it.
 *
 * If it is not a transshipment node, we need to follow the
 * thread to update the potentials. We do it by computing
 */
static void testAndRemoveArtificialArcs(dbm::reader dbm, const int32_t* rates, Tree& tree)
{
    uint32_t dim = tree.size();
    for (uint32_t i = 1; i < dim; ++i) {
        const auto pred_i = tree.pred_of(i);
        if (tree[i].potential == dbm_INFINITY && pred_i == 0 && tree[i].flow == 0) {
            tree[i].inbound = true;

            Node* tmp = tree[i].thread;
            [[maybe_unused]] int32_t rateSum = b(rates, i);

            int32_t minPotential = dbm_INFINITY + dbm.at(0, i);

            /*
             * Find the highest potential we can decrease i with and
             * maintain positive potentials.
             */
            while (tmp->depth > tree[i].depth) {
                rateSum += b(rates, tree.index_of(tmp));
                if (tmp->potential < minPotential)
                    minPotential = tmp->potential;
                tmp = tmp->thread;
            }
            assert(rateSum == 0);

            /*
             * Update all potentials in the thread by subtracting
             * minPotential.
             *
             * Subtracting the same potetial maintains feasibility.
             */

            tmp = &tree[i];
            do {
                tmp->potential -= minPotential;
                tmp = tmp->thread;
            } while (tmp->depth > tree[i].depth);
        }
    }
}

static const auto is_non_negative = [](const int32_t& n) { return n >= 0; };

/*
 * Takes a priced zone and computes that infimum value of the dual
 * network flow problem. I.e. the cost of the offset and the costless
 * moves to the offset is not taken into account.
 */
static void infimumNetSimplex(dbm::reader dbm, const int32_t* rates, Tree& tree)
{
    uint32_t dim = tree.size();
    /* Find and store the minimal set of arcs. The netsimplex
     * algorithm is polynomial in the number of arcs.
     */
    std::vector<uint32_t> bitMatrix(bits2intsize(dim * dim));
    auto nbConstraints = dbm_analyzeForMinDBM(dbm, dim, bitMatrix.data());
    auto arcs = std::vector<arc_t>(nbConstraints);
    auto arc_it = arcs.begin();
    for (uint32_t i = 0; i < dim; i++) {
        for (uint32_t j = 0; j < dim; j++) {
            if (ONE == base_readOneBit(bitMatrix.data(), i * dim + j)) {
                arc_it->i = i;
                arc_it->j = j;
                ++arc_it;
            }
        }
    }
    assert(std::distance(arcs.begin(), arc_it) == (int32_t)nbConstraints);

    findInitialSpanningTreeSolution(dbm, rates, tree);

    ASSERT(checkTreeIntegrity(dbm, rates, tree, dim), printAllNodeInfo(tree, rates));

    /*
     * Run the network simplex algorithm on the dual problem of the
     * zones. If z is the infimium value in the dbm with rates, then
     * the network simplex algorithm computes -z. However, the value
     * of the node potential at the point of termination, matches
     * precisely the infimum point of the zone.
     */
    auto arc = enteringArcDanzig(arcs, tree, dbm);
    while (arc != std::end(arcs)) {
        Node* k = &tree[arc->i];
        Node* l = &tree[arc->j];

        /* Find common root in the cycle induced by introducing the
         * entering arc in the spanning tree.
         */
        Node* root = discoverCycleRoot(k, l);

        /* Leave will get the node that mentions the leaving arc.
         */
        Node* leave = findLeavingArc(k, l, root);

        updateSpanningTree(k, l, leave, root, dbm.at(arc->i, arc->j));

        ASSERT(checkTreeIntegrity(dbm, rates, tree, dim), printAllNodeInfo(tree, rates));
        // printAllNodeInfo(tree, rates, dim);
        arc = enteringArcDanzig(arcs, tree, dbm);
    }

    /*
     * Get rid of artificial arcs.
     */
    testAndRemoveArtificialArcs(dbm, rates, tree);
}

/*
 * Computes and returns the infimum over the zone given the cost rates.
 * At termination, valuation holds the clock valuation corresponding to
 * the infimum-achieving point in the zone.
 *
 */
int32_t pdbm_infimum(dbm::reader dbm, uint32_t dim, uint32_t offsetCost, const int32_t* rates)
{
    if (std::all_of(rates, rates + dim, is_non_negative)) {
        return offsetCost;
    }

    auto tree = Tree(dim);
    infimumNetSimplex(dbm, rates, tree);

    int32_t solution = offsetCost;
    for (uint32_t i = 0; i < dim; i++) {
        ASSERT(tree[i].potential >= 0, std::cerr << "Node: " << i << std::endl; printAllNodeInfo(tree, rates);
               printClockLowerBounds(dbm, dim));
        /*
         * Check if best solution has positive flow on artificial arc
         * if so, return minus infinity as the solution is unbounded.
         */
        if (tree[i].potential == dbm_INFINITY && tree.pred_of(i) == 0 && tree[i].flow > 0) {
            return -dbm_INFINITY;
        }
        solution += rates[i] * (tree[i].potential + dbm.at(0, i));
    }

    return solution;
}

void pdbm_infimum(dbm::reader dbm, uint32_t dim, uint32_t offsetCost, const int32_t* rates, int32_t* valuation)
{
    if (std::all_of(rates, rates + dim, is_non_negative)) {
        valuation[0] = 0;
        for (uint32_t i = 1; i < dim; ++i)
            valuation[i] = -dbm.at(0, i);
    } else {
        auto tree = Tree(dim);
        infimumNetSimplex(dbm, rates, tree);

        /* Assign the potentials to the best solution. */
        valuation[0] = 0;
        for (uint32_t i = 1; i < dim; i++) {
            ASSERT(tree[i].potential >= 0, std::cerr << "Node: " << i << std::endl; printAllNodeInfo(tree, rates);
                   printSolution(valuation, rates, dim); printClockLowerBounds(dbm, dim));
            /*
             * Check if best solution has positive flow on artificial arc
             * if so, infimum is not well defined as it is minus infinity
             * and we throw an exception.
             */
            if (tree[i].potential == dbm_INFINITY && tree.pred_of(i) == 0 && tree[i].flow > 0)
                throw std::domain_error("Infimum is downward unbound, thus, no well-defined infimum valuation.");

            valuation[i] = tree[i].potential;
        }
    }
}
