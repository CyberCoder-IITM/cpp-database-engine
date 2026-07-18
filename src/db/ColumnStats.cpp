#include <db/ColumnStats.hpp>
#include <algorithm>
#include <numeric>
#include <stdexcept>

using namespace db;

ColumnStats::ColumnStats(unsigned buckets, int min, int max)
        : num_buckets(buckets), range_min(min), range_max(max), counts(buckets, 0), total(0) {
    if (buckets == 0 || min >= max) {
        throw std::invalid_argument("ColumnStats requires at least one bucket and min < max");
    }
    // Ceiling division so the buckets fully cover the inclusive range [min, max].
    const long long span = static_cast<long long>(max) - min + 1;
    bucket_width = static_cast<int>((span + buckets - 1) / buckets);
}

void ColumnStats::addValue(int v) {
    if (v < range_min || v > range_max) {
        return;
    }
    size_t index = static_cast<size_t>(v - range_min) / bucket_width;
    index = std::min(index, static_cast<size_t>(num_buckets - 1));
    ++counts[index];
    ++total;
}

size_t ColumnStats::estimateCardinality(PredicateOp op, int v) const {
    // Values entirely below or above the tracked range: every stored value is on one side of v.
    if (v < range_min) {
        switch (op) {
            case PredicateOp::GT:
            case PredicateOp::GE:
            case PredicateOp::NE:
                return total;
            default:
                return 0;
        }
    }
    if (v > range_max) {
        switch (op) {
            case PredicateOp::LT:
            case PredicateOp::LE:
            case PredicateOp::NE:
                return total;
            default:
                return 0;
        }
    }

    size_t index = static_cast<size_t>(v - range_min) / bucket_width;
    index = std::min(index, static_cast<size_t>(num_buckets - 1));

    const size_t height = counts[index];
    const size_t width = static_cast<size_t>(bucket_width);
    const int bucket_start = range_min + static_cast<int>(index) * bucket_width;
    const int bucket_end = bucket_start + bucket_width - 1;

    const size_t below = std::accumulate(counts.begin(), counts.begin() + static_cast<long>(index), size_t(0));
    const size_t above = std::accumulate(counts.begin() + static_cast<long>(index) + 1, counts.end(), size_t(0));

    switch (op) {
        case PredicateOp::EQ:
            return height / width;
        case PredicateOp::NE:
            return total - height / width;
        case PredicateOp::LT:
            return below + static_cast<size_t>(v - bucket_start) * height / width;
        case PredicateOp::LE:
            return below + static_cast<size_t>(v - bucket_start + 1) * height / width;
        case PredicateOp::GT:
            return above + static_cast<size_t>(bucket_end - v) * height / width;
        case PredicateOp::GE:
            return above + static_cast<size_t>(bucket_end - v + 1) * height / width;
    }
    throw std::logic_error("Unsupported predicate operator");
}
