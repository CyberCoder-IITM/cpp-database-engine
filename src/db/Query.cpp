#include <db/Query.hpp>
#include <db/Tuple.hpp>
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>

using namespace db;

namespace {

    bool satisfies(const field_t &lhs, PredicateOp op, const field_t &rhs) {
        switch (op) {
            case PredicateOp::EQ: return lhs == rhs;
            case PredicateOp::NE: return lhs != rhs;
            case PredicateOp::LT: return lhs < rhs;
            case PredicateOp::LE: return lhs <= rhs;
            case PredicateOp::GT: return lhs > rhs;
            case PredicateOp::GE: return lhs >= rhs;
        }
        throw std::logic_error("Unsupported predicate operator");
    }

    // Accumulates the running state needed to compute any of the five aggregate operations for one group
    // (or for the whole table, when there is no GROUP BY field).
    struct AggState {
        size_t count = 0;
        double sum = 0.0;
        bool saw_double = false;
        std::optional<field_t> min;
        std::optional<field_t> max;

        void accumulate(const field_t &v) {
            ++count;
            if (std::holds_alternative<int>(v)) {
                sum += std::get<int>(v);
            } else if (std::holds_alternative<double>(v)) {
                sum += std::get<double>(v);
                saw_double = true;
            }
            if (!min || v < *min) {
                min = v;
            }
            if (!max || v > *max) {
                max = v;
            }
        }

        field_t result(AggregateOp op) const {
            switch (op) {
                case AggregateOp::COUNT:
                    return static_cast<int>(count);
                case AggregateOp::SUM:
                    return saw_double ? field_t(sum) : field_t(static_cast<int>(sum));
                case AggregateOp::AVG:
                    return sum / static_cast<double>(count);
                case AggregateOp::MIN:
                    return *min;
                case AggregateOp::MAX:
                    return *max;
            }
            throw std::logic_error("Unsupported aggregate operator");
        }
    };

} // namespace

void db::projection(const DbFile &in, DbFile &out, const std::vector<std::string> &field_names) {
    const TupleDesc &in_td = in.getTupleDesc();
    std::vector<size_t> indices;
    indices.reserve(field_names.size());
    for (const auto &name : field_names) {
        indices.push_back(in_td.index_of(name));
    }

    for (const auto &t : in) {
        std::vector<field_t> fields;
        fields.reserve(indices.size());
        for (size_t idx : indices) {
            fields.push_back(t.get_field(idx));
        }
        out.insertTuple(Tuple(fields));
    }
}

void db::filter(const DbFile &in, DbFile &out, const std::vector<FilterPredicate> &pred) {
    const TupleDesc &in_td = in.getTupleDesc();
    std::vector<size_t> indices;
    indices.reserve(pred.size());
    for (const auto &p : pred) {
        indices.push_back(in_td.index_of(p.field_name));
    }

    for (const auto &t : in) {
        bool ok = true;
        for (size_t i = 0; i < pred.size(); ++i) {
            if (!satisfies(t.get_field(indices[i]), pred[i].op, pred[i].value)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            out.insertTuple(t);
        }
    }
}

void db::join(const DbFile &left, const DbFile &right, DbFile &out, const JoinPredicate &pred) {
    const TupleDesc &left_td = left.getTupleDesc();
    const TupleDesc &right_td = right.getTupleDesc();
    const size_t left_idx = left_td.index_of(pred.left);
    const size_t right_idx = right_td.index_of(pred.right);
    const bool drop_right_field = pred.op == PredicateOp::EQ;

    // Simple nested-loop join: the buffer pool caches pages for us, so this stays within its budget as
    // long as one of the two tables is small relative to the pool.
    for (const auto &lt : left) {
        const field_t &lval = lt.get_field(left_idx);
        for (const auto &rt : right) {
            if (!satisfies(lval, pred.op, rt.get_field(right_idx))) {
                continue;
            }

            std::vector<field_t> fields;
            fields.reserve(left_td.size() + right_td.size());
            for (size_t i = 0; i < left_td.size(); ++i) {
                fields.push_back(lt.get_field(i));
            }
            for (size_t i = 0; i < right_td.size(); ++i) {
                if (drop_right_field && i == right_idx) {
                    continue;
                }
                fields.push_back(rt.get_field(i));
            }
            out.insertTuple(Tuple(fields));
        }
    }
}

void db::aggregate(const DbFile &in, DbFile &out, const Aggregate &agg) {
    const TupleDesc &td = in.getTupleDesc();
    const size_t field_idx = td.index_of(agg.field);

    if (agg.group) {
        const size_t group_idx = td.index_of(*agg.group);
        // std::map (not unordered_map) because field_t has no std::hash specialization, but it does
        // support operator< via std::variant, which is all std::map needs.
        std::map<field_t, AggState> groups;
        for (const auto &t : in) {
            groups[t.get_field(group_idx)].accumulate(t.get_field(field_idx));
        }
        for (const auto &[key, state] : groups) {
            out.insertTuple(Tuple({key, state.result(agg.op)}));
        }
    } else {
        AggState state;
        for (const auto &t : in) {
            state.accumulate(t.get_field(field_idx));
        }
        out.insertTuple(Tuple({state.result(agg.op)}));
    }
}
