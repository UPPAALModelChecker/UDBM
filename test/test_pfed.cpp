#include "dbm/cost_type.h"
#include "dbm/pfed.h"

#include <doctest/doctest.h>

void any_contains(const dbm::pfed_t& pfed, int* valuation, size_t dim, CostType expected) {
    bool any_contains = false;
    for (const auto& pdbm : pfed) {
        if (pdbm_containsInt(pdbm, dim, valuation)) {
            CHECK_MESSAGE((pdbm_getCostOfValuation(pdbm, dim, valuation) == expected), "Expected cost " << expected << " but got " << pdbm_getCostOfValuation(pdbm, dim, valuation));
            any_contains = true;
        }
    }
    CHECK(any_contains);
}

TEST_SUITE("pfed")
{
    TEST_CASE("delay_constraint")
    {
        dbm::pfed_t pfed(2);
        pfed.setZero();
        pfed.up(4);
        pfed.constrain(1, 0, 2, false);
        pfed.constrain(0, 1, -1, false);

        REQUIRE((pfed.getInfimum() == 4));
    }

    TEST_CASE("reset")
    {
        dbm::pfed_t pfed(3);
        pfed.setZero();
        pfed.up(1);
        pfed.constrain(1, 0, 3, false);
        pfed.updateValue(1, 0);
        pfed.up(2);
        pfed.constrain(0, 2, -1, false);
        pfed.updateValue(2, 0);
        pfed.up(3);

        REQUIRE((pfed.size() == 2));
        REQUIRE((pfed.getInfimum() == 1));


        {

        }
        // Assert that some pdbm in pfed contains the valuation (1,1) and has cost 4
        {
            int valuation[3] = {0, 1, 1};
            any_contains(pfed, valuation, 3, 4);
        }

        {
            int valuation[3] = {0, 2, 2};
            any_contains(pfed, valuation, 3, 7);
        }

        {
            int valuation[3] = {0, 2, 0};
            any_contains(pfed, valuation, 3, 4);
        }
    }
}