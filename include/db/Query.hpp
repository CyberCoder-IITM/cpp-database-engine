#pragma once

#include <db/DbFile.hpp>
#include <optional>
#include <vector>

namespace db {

    /**
     * @brief The operation of a predicate.
     * @details The supported numeric comparison operations are:
     *   EQ (equal),
     *   NE (not equal),
     *   LT (less than),
     *   LE (less than or equal),
     *   GT (greater than),
     *   GE (greater than or equal).
     */
    enum class PredicateOp { EQ, NE, LT, LE, GT, GE };

    /**
     * @brief A predicate to filter rows.
     * @details A predicate is a comparison between a field and a value.
     *   The field is specified by the field_name.
     *   The op is the operation to perform.
     *   The value is the value to compare the field with.
     */
    struct FilterPredicate {
        std::string field_name;
        PredicateOp op;
        field_t value;
    };

    /**
     * @brief A predicate to join two tables.
     * @details A join predicate is a comparison between a field of the left table and a field of the right table.
     */
    struct JoinPredicate {
        std::string left;
        PredicateOp op;
        std::string right;
    };

    /**
     * @brief The operation of an aggregate.
     * @details The supported aggregate operations are: sum, average, minimum, maximum, and count.
     */
    enum class AggregateOp { SUM, AVG, MIN, MAX, COUNT };

    /**
     * @brief An aggregate operation to group and summarize rows.
     * @details Groups rows by a field (optional) and summarizes the values of another field.
     */
    struct Aggregate {
        std::optional<std::string> group;
        AggregateOp op;
        std::string field;
    };

    /**
     * @brief Perform a projection operation.
     * @details Selects a subset of fields from the input table, in the order given by field_names, and writes
     * the resulting tuples to the output table.
     * @param in The input table.
     * @param out The output table.
     * @param field_names The fields to keep.
     */
    void projection(const DbFile &in, DbFile &out, const std::vector<std::string> &field_names);

    /**
     * @brief Perform a filter operation.
     * @details Selects rows that satisfy every predicate in pred (logical AND) and writes them to the output table.
     * @param in The input table.
     * @param out The output table.
     * @param pred The predicates to filter rows.
     */
    void filter(const DbFile &in, DbFile &out, const std::vector<FilterPredicate> &pred);

    /**
     * @brief Perform a join operation.
     * @details Combines rows from two tables that satisfy the join predicate and writes the result to the output
     * table.
     * @param left The left table.
     * @param right The right table.
     * @param out The output table.
     * @param pred The join predicate.
     * @note When performing an equality join, the join field of the right table is dropped from the output.
     */
    void join(const DbFile &left, const DbFile &right, DbFile &out, const JoinPredicate &pred);

    /**
     * @brief Perform an aggregate operation.
     * @details Groups rows by a field and summarizes the values of another field. If no group field is given, the
     * aggregate is performed over all rows and a single tuple is produced.
     * @param in The input table.
     * @param out The output table.
     * @param agg The aggregate operation.
     * @note The computed value has the same type as the aggregated field, except for AVG which is always a double.
     */
    void aggregate(const DbFile &in, DbFile &out, const Aggregate &agg);

} // namespace db
