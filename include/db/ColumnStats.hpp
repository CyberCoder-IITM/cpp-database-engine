#pragma once

#include <db/Query.hpp>
#include <vector>

namespace db {

    /**
     * @brief A fixed-width histogram over a single integer-valued column, used to estimate the cardinality of
     * selection predicates for query planning.
     */
    class ColumnStats {
        unsigned num_buckets;
        int range_min;
        int range_max;
        int bucket_width;
        std::vector<size_t> counts;
        size_t total;

    public:
        /**
         * @brief Construct a histogram covering [min, max] split into `buckets` equal-width buckets.
         * @param buckets The number of buckets to split the input range into.
         * @param min The minimum integer value this instance will process.
         * @param max The maximum integer value this instance will process.
         */
        ColumnStats(unsigned buckets, int min, int max);

        /**
         * @brief Add a value to the histogram. Values outside [min, max] are ignored.
         * @param v Value to add to the histogram.
         */
        void addValue(int v);

        /**
         * @brief Estimate the number of values that satisfy the given predicate, assuming values are uniformly
         * distributed within each bucket.
         * @param op Operator.
         * @param v Value.
         * @return Estimated cardinality of this operator and value.
         */
        size_t estimateCardinality(PredicateOp op, int v) const;
    };

} // namespace db
