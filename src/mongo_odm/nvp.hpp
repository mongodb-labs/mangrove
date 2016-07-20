// Copyright 2016 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <mongo_odm/config/prelude.hpp>

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <bsoncxx/types.hpp>

#include <mongo_odm/macros.hpp>
#include <mongo_odm/util.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <typename NvpT, typename T>
class NvpCRTP;

template <typename NvpT>
class SortExpr;

// Forward declarations for Expression Types
template <typename NvpT, typename U>
class ComparisonExpr;

template <typename NvpT, typename U>
class ComparisonValueExpr;

template <typename NvpT>
class ModExpr;

template <typename NvpT, typename U>
class UpdateExpr;

template <typename NvpT, typename U>
class UpdateValueExpr;

template <typename NvpT>
class UnsetExpr;

template <typename NvpT>
class CurrentDateExpr;

template <typename NvpT, typename U>
class AddToSetUpdateExpr;

template <typename NvpT, typename U, typename Sort = int>
class PushUpdateExpr;

struct current_date_t {};
const current_date_t current_date;

// Forward declarations for Expression type trait structs
template <typename>
struct is_sort_expression;
template <typename>
struct is_query_expression;
template <typename>
struct is_update_expression;

/**
 * An object that represents a name-value pair of a member in an object.
 * It is templated on the class of the member and its type.
 */
template <typename Base, typename T>
class Nvp : public NvpCRTP<Nvp<Base, T>, T> {
   public:
    using type = T;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;

    /**
     * Create a name-value pair from a member pointer and a name.
     * @param  t    A pointer to the member
     * @param  name The name of the member
     */
    constexpr Nvp(T Base::*t, const char* name) : t(t), name(name) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr UpdateExpr<Nvp<Base, T>, no_opt_type> operator=(const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
    * Creates an expression that sets a date value to the current date.
    * This is only enabled for std::chrono::time/duration values, as well as b_date.
    */
    template <typename U = no_opt_type>
    constexpr typename std::enable_if<is_date<U>::value, CurrentDateExpr<Nvp<Base, T>>>::type
    operator=(const current_date_t& cd) const {
        (void)cd;
        return {*this, true};
    }

    /**
     * Creates an expression that sets a date value to the current date.
     * This is only enabled for std::chrono::time/duration values, as well as b_date.
     */
    template <typename U = no_opt_type>
    constexpr typename std::enable_if<std::is_same<bsoncxx::types::b_timestamp, U>::value,
                                      CurrentDateExpr<Nvp<Base, T>>>::type
    operator=(const current_date_t& cd) const {
        (void)cd;
        return {*this, false};
    }

    std::string get_name() const {
        return name;
    }

    T Base::*t;
    const char* name;
};

/**
 * Class that represents a name-value pair for a field of an object that is a member of another
 * object.
 * The name of this name-value pair will be "<parent-name>.field_name".
 * The parenet can be either an Nvp, or another NvpChild.
 * @tparam Base The type of the object that contains this field
 * @tparam T    The type of the field
 * @tparam Parent The type of the parent name-value pair, either an Nvp<...> or NvpChild<...>.
 */
template <typename Base, typename T, typename Parent>
class NvpChild : public NvpCRTP<NvpChild<Base, T, Parent>, T> {
   public:
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;
    using type = T;

    constexpr NvpChild(T Base::*t, const char* name, const Parent& parent)
        : t(t), name(name), parent(parent) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr UpdateExpr<NvpChild<Base, T, Parent>, T> operator=(const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
    * Creates an expression that sets a date value to the current date.
    * This is only enabled for std::chrono::time/duration values, as well as b_date.
    */
    template <typename U = no_opt_type>
    constexpr
        typename std::enable_if<is_date<U>::value, CurrentDateExpr<NvpChild<Base, T, Parent>>>::type
        operator=(const current_date_t& cd) const {
        (void)cd;
        return {*this, true};
    }

    /**
     * Creates an expression that sets a date value to the current date.
     * This is only enabled for std::chrono::time/duration values, as well as b_date.
     */
    template <typename U = no_opt_type>
    constexpr typename std::enable_if<std::is_same<bsoncxx::types::b_timestamp, U>::value,
                                      CurrentDateExpr<NvpChild<Base, T, Parent>>>::type
    operator=(const current_date_t& cd) const {
        (void)cd;
        return {*this, false};
    }

    /**
     * Returns the qualified name of this field in dot notation, i.e. "parent.child".
     * @return A string containing the name of this field in dot notation.
     */
    std::string get_name() const {
        std::string full_name;
        full_name += (parent.get_name() + ".");
        full_name += this->name;
        return full_name;
    }

    T Base::*t;
    const char* name;
    const Parent& parent;
};

/**
 * Represents a field that does not have a name, i.e. an array element.
 * This is used in $elemMatch expressions on scalar arrays, where the elements can be compared but
 * don't have fields of their own.
 * @tparam T    The type of the field.
 */
template <typename T>
class FreeNvp : public NvpCRTP<FreeNvp<T>, T> {
   public:
    // In case this corresponds to an optional type, store the underlying type.
    using no_opt_type = remove_optional_t<T>;
    using type = T;

    // TODO This shouldn't have a name, but it needs to work with Expressions when building queries.
    std::string get_name() const {
        return "";
    }
};

/**
 * A CRTP base class that contains member functions for name-value pairs.
 * These functions are identical between Nvp<...> and NvpChild<...>, but their return types are
 * templated on the Nvp's types, so they are defined here using CRTP.
 * @tparam NvpT The type of the name-value pair
 * @tparam T    The type of the field referred to by the name-value pair.
 */
template <typename NvpT, typename T>
class NvpCRTP {
   private:
    // Type trait that checks if the given iterable class contains the same value type as either
    // a) this Nvp's type, or
    // b) this Nvp's value type, if this Nvp is also an iterable.
    template <typename Iterable, typename Default = void>
    using enable_if_matching_iterable_t = typename std::enable_if<
        is_iterable_not_string<Iterable>::value &&
            std::is_convertible<iterable_value<Iterable>,
                                iterable_value<remove_optional_t<T>>>::value,
        Default>::type;

   public:
    using type = T;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;

    // keeps track of pointer to base class, for convenience.
    const NvpT* self;

    constexpr NvpCRTP() : self(static_cast<const NvpT*>(this)) {
    }

    /**
     * Chains two name-value pairs to access a sub-field, i.e. a field with the name "parent.child".
     * @tparam U    The type of the child NVP
     * @param child An NVP that corresponds to a sub-field of this NVP. Its base class must be the
     * same as this field's current type.
     * @return      An NVP with the same base class and type as the subfield, but with a link to a
     * parent so that its name is qualified in dot notation.
     */
    template <typename U>
    constexpr NvpChild<T, U, NvpT> operator->*(const Nvp<T, U>& child) const {
        return {child.t, child.name, *static_cast<const NvpT*>(this)};
    }

    /**
     * Creates a sort expression that sorts documents by this field.
     * @param ascending Whether to sort by ascending order (+1 in MongoDB syntax).
     *                  If false, sorts by descending order (-1 in MongoDB syntax).
     * @return a SortExpr that reprsents the sort expression {field: +/-1}.
     */
    constexpr SortExpr<NvpT> sort(bool ascending) const {
        return {*static_cast<const NvpT*>(this), ascending};
    }

    /**
     * Creates an expression that checks whether the value of this field matches any value in the
     * given iterable.
     * @tparam Iterable A type that works in range-based for loops, and yields objects convertible
     * to the type of this name-value pair.
     */
    template <typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr ComparisonExpr<NvpT, Iterable> in(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$in"};
    }

    /**
     * Creates an expression that checks whether the value of this field matches none of the values
     * in the given iterable.
     * @tparam Iterable A type that works in range-based for loops, and yields objects convertible
     * to the type of this name-value pair.
     */
    template <typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr ComparisonExpr<NvpT, Iterable> nin(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$nin"};
    }

    /**
     * Creates an expression that checks the existence of a certain field.
     * This is only enabled for fields that are optional types.
     * {field: {$exists: <bool>}}
     * @param exists    If false, checks that the given field does *not* exist in a document.
     *                  True by default.
     */
    template <typename U = T, typename = typename std::enable_if<is_optional<U>::value>::type>
    constexpr ComparisonExpr<NvpT, bool> exists(const bool& exists) const {
        return {*static_cast<const NvpT*>(this), exists, "$exists"};
    }

    /**
     * Creates a ModExpr that represents a query with the $mod operator.
     * Such a query essentially checks that "nvp_value % divisor == remainder"
     * @param divisor   The divisor for the modulus operation
     * @param remainder   The remainder after dividing a value by `divisor`
     * @returns A ModExpr representing this query.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<std::is_arithmetic<U>::value>::type>
    constexpr ModExpr<NvpT> mod(const int& divisor, const int& remainder) const {
        return {*static_cast<const NvpT*>(this), divisor, remainder};
    }

    /**
     * Creates a comparison expression that represents a query with a $regex operator.
     * Such a query only works for string fields.
     * @param regex     The regex to check against.
     * @param options   Options to pass to the regex.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_string<U>::value>::type>
    constexpr ComparisonValueExpr<NvpT, bsoncxx::types::b_regex> regex(const char* regex,
                                                                       const char* options) const {
        return {*static_cast<const NvpT*>(this), bsoncxx::types::b_regex(regex, options), "$regex"};
    }

    /* Array Query operators */

    /**
     * Creates a query with the $all operator that compares values in this field's array to values
     * in another array.
     * This is only enabled if the current field is an iterable itself.
     * @param  iter     An iterable containing elements of the same type as this field's elements.
     * @returns         A comparison expression with the $all oeprator.
     */
    template <typename Iterable, typename = enable_if_matching_iterable_t<Iterable>,
              typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr ComparisonExpr<NvpT, Iterable> all(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$all"};
    }

    /**
     * Creates a query with the $elemMatch operator that finds elements in this field that match the
     * given queries. This can include "FreeExpr" expressions, that don't contain a field name,
 * In the case of a scalar array. e.g. "arr: {$elemMatch: {$gt: 4, .....}}"
     * This is only enabled if the current field is an iterable itself.
     * TODO type-checking that the given queries correspond to children of the current array's
     * elements.
     *
     * @tparam Expr     A query expression object.
     * @param queries   Queries to comapre values against.
     * @returns         A comparison expression with the $elemMatch operator.
     */
    template <typename Expr,
              typename = typename std::enable_if<is_query_expression<Expr>::value>::type,
              typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr ComparisonExpr<NvpT, Expr> elem_match(const Expr& queries) const {
        return {*static_cast<const NvpT*>(this), queries, "$elemMatch"};
    }

    /**
     * Constructs a nameless name-value-pair that corresponds to an element in a scalar array, if
     * this field is an array. This is used to create expressions with $elemMatch.
     * This is only enabled if this current field is an array.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr FreeNvp<iterable_value<no_opt_type>> element() const {
        return {};
    }

    /**
     * Creates an array query expression with the $size operator.
     * This is only enabled if this current field is an array.
     * @param n The size the array should be.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr ComparisonExpr<NvpT, std::int64_t> size(const std::int64_t& n) const {
        return {*static_cast<const NvpT*>(this), n, "$size"};
    }

    /* bitwise queries, enabled only for integral types. */

    /**
     * Creates a query that uses the $bitsAllSet operator to check a numerical field with a bitmask.
     * $bitsAllSet checks that every bit in the bitmask is set in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAllSet operator
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename Mask, typename = typename std::enable_if<
                                 std::is_integral<Mask>::value ||
                                 std::is_same<Mask, bsoncxx::types::b_binary>::value>::type>
    constexpr ComparisonExpr<NvpT, Mask> bits_all_set(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAllSet"};
    }

    /**
     * Creates a query that uses the $bitsAllSet operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename... Args>
    constexpr ComparisonValueExpr<NvpT, std::int64_t> bits_all_set(std::int64_t pos1,
                                                                   std::int64_t pos2,
                                                                   Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAllSet"};
    }

    /**
     * Creates a query that uses the $bitsAnySet operator to check a numerical field with a bitmask.
     * $bitsAnySet checks that a least one bit in the bitmask is set in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAnySet operator
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename Mask, typename = typename std::enable_if<
                                 std::is_integral<Mask>::value ||
                                 std::is_same<Mask, bsoncxx::types::b_binary>::value>::type>
    constexpr ComparisonExpr<NvpT, Mask> bits_any_set(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAnySet"};
    }

    /**
     * Creates a query that uses the $bitsAnySet operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename... Args>
    constexpr ComparisonValueExpr<NvpT, std::int64_t> bits_any_set(std::int64_t pos1,
                                                                   std::int64_t pos2,
                                                                   Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAnySet"};
    }

    /**
     * Creates a query that uses the $bitsAllClear operator to check a numerical field with a
     * bitmask.
     * $bitsAllClear checks that every bit in the bitmask is cleared in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAllClear operator
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename Mask, typename = typename std::enable_if<
                                 std::is_integral<Mask>::value ||
                                 std::is_same<Mask, bsoncxx::types::b_binary>::value>::type>
    constexpr ComparisonValueExpr<NvpT, Mask> bits_all_clear(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAllClear"};
    }

    /**
     * Creates a query that uses the $bitsAllClear operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename... Args>
    constexpr ComparisonValueExpr<NvpT, std::int64_t> bits_all_clear(std::int64_t pos1,
                                                                     std::int64_t pos2,
                                                                     Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAllClear"};
    }

    /**
     * Creates a query that uses the $bitsAnyClear operator to check a numerical field with a
     * bitmask.
     * $bitsAnyClear checks that a least one bit in the bitmask is cleared in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAnyClear operator
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename Mask, typename = typename std::enable_if<
                                 std::is_integral<Mask>::value ||
                                 std::is_same<Mask, bsoncxx::types::b_binary>::value>::type>
    constexpr ComparisonExpr<NvpT, Mask> bits_any_clear(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAnyClear"};
    }

    /**
     * Creates a query that uses the $bitsAnyClear operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A ComparisonExpr representing this query
     */
    template <typename U = no_opt_type, typename = typename std::enable_if<
                                            std::is_integral<U>::value ||
                                            std::is_same<U, bsoncxx::types::b_binary>::value>::type,
              typename... Args>
    constexpr ComparisonValueExpr<NvpT, std::int64_t> bits_any_clear(std::int64_t pos1,
                                                                     std::int64_t pos2,
                                                                     Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAnyClear"};
    }

    constexpr UpdateExpr<NvpT, no_opt_type> set_on_insert(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$setOnInsert"};
    }

    /* Update operators */

    /**
     * Creates an expression that unsets the current field. The field must be of optional type.
     * @returns An UnsetExpr that unsets the current field.
     */
    template <typename U = T, typename = typename std::enable_if<is_optional<U>::value>::type>
    constexpr UnsetExpr<NvpT> unset() const {
        return {*static_cast<const NvpT*>(this)};
    }

    /**
     * Creates an expression that uses the $min operator to only update a field if the new value is
     * lower than the current value.
     * @param val   The (tentative) new value.
     * @returns     An UpdateExpression with the $min operator.
     */
    constexpr UpdateExpr<NvpT, no_opt_type> min(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$min"};
    }

    /**
     * Creates an expression that uses the $max operator to only update a field if the new value is
     * greater than the current value.
     * @param val   The (tentative) new value.
     * @returns     An UpdateExpression with the $max operator.
     */
    constexpr UpdateExpr<NvpT, no_opt_type> max(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$max"};
    }

    /**
     * Creates an expression that sets a date value to the current date.
     * This is only enabled for std::chrono::time/duration values, as well as b_date.
     */
    template <typename U = no_opt_type>
    constexpr typename std::enable_if<is_date<U>::value, CurrentDateExpr<NvpT>>::type current_date()
        const {
        return {*static_cast<const NvpT*>(this), true};
    }

    /**
     * Creats an expression that sets a b_timestamp value to the current date.
     * This is only enabled for b_timestamp values. (Use of which is discouraged by users.)
     */
    template <typename U = no_opt_type>
    constexpr typename std::enable_if<std::is_same<bsoncxx::types::b_timestamp, U>::value,
                                      CurrentDateExpr<NvpT>>::type
    current_date() const {
        return {*static_cast<const NvpT*>(this), false};
    }

    /* Array update operators */
    /**
     * Creates an update expression with the $pop operator.
     * This is only enabled if the current field is an array type.
     * @param last  If true, removes the element from the end of the array.
     *              If false, from the start.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr UpdateValueExpr<NvpT, int> pop(bool last) const {
        return {*static_cast<const NvpT*>(this), last ? 1 : -1, "$pop"};
    }

    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given value exactly.
     * This is only enabled if the current field is an array type.
     * @param val   The value to remove. This must match the type *contained* by this array field.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr UpdateExpr<NvpT, iterable_value<no_opt_type>> pull(
        const iterable_value<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, "$pull"};
    }
    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given query.
     * This query can contain free expressions, similarly to the $elemMatch operator.
     * This is only enabled if the current field is an array type.
     * @tparam Expr The type of the given query, must satisfy is_query_expression.
     * @param  expr A query expression against which to compare
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type,
              typename Expr>
    constexpr
        typename std::enable_if<is_query_expression<Expr>::value, UpdateExpr<NvpT, Expr>>::type
        pull(const Expr& expr) const {
        return {*static_cast<const NvpT*>(this), expr, "$pull"};
    }

    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given value exactly.
     * This is only enabled if the current field is an array type.
     * @param iter   An iterable containing the values to remove.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr UpdateExpr<NvpT, Iterable> pull_all(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$pullAll"};
    }

    /**
     * Creates an update expression with the $addToSet operator,
     * that adds a single value to an array, if it is unique.
     * This is only enabled if the current field is an array type.
     * @param val   The value to add.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr AddToSetUpdateExpr<NvpT, iterable_value<no_opt_type>> add_to_set(
        const iterable_value<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, false};
    }

    /**
     * Creates an update expression with the $addToSet operator and the $each modifier,
     * that adds a list of value to an array, only keeping the unique values.
     * This is only enabled if the current field is an array.
     * @param iter   The list of values to add.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr AddToSetUpdateExpr<NvpT, Iterable> add_to_set(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, true};
    }

    /**
     * Creates an update epxression with the $push operator, that adds a single value to an array.
     * This is only enabled if the current field is an array.
     * @param val   The value to add.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type>
    constexpr PushUpdateExpr<NvpT, iterable_value<no_opt_type>> push(
        const iterable_value<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, false};
    }

    /**
     * Creates an update epxression with the $push operator and the $each modifier,
     * that adds a list of value to an array. Further modifiers can be given as arguments to this
     * function, or by modifying the resulting PushUpdateExpr object.
     * This is only enabled if the current field is an array.

     * @tparam Iterable An iterable that contains the same value type as this field's array.
     * @tparam Sort     The type of the given sort expression, if one is given.
     * @param  iter     The iterable containing values to add.
     * @param  slice    An optional argument containing the value of the $slice modifier.
     * @param  sort    An optional argument containing an expression for the $sort modifier.
     * @param  potision    An optional argument containing the value of the $position modifier.
     */
    template <typename U = no_opt_type,
              typename = typename std::enable_if<is_iterable_not_string<U>::value>::type,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>,
              typename Sort = int,
              typename = typename std::enable_if<is_sort_expression<Sort>::value ||
                                                 std::is_same<int, Sort>::value>::type>
    constexpr PushUpdateExpr<NvpT, Iterable, Sort> push(
        const Iterable& iter, bsoncxx::stdx::optional<std::int32_t> slice = bsoncxx::stdx::nullopt,
        const bsoncxx::stdx::optional<Sort>& sort = bsoncxx::stdx::nullopt,
        bsoncxx::stdx::optional<std::uint32_t> position = bsoncxx::stdx::nullopt) const {
        return {*static_cast<const NvpT*>(this), iter, true, slice, sort, position};
    }
};

/**
 * A type trait struct that inherits from std::true_type
 * if the given type parameter is a name-value pair,
 * and from std::false_type otherwise.
 */
template <typename>
struct is_nvp_type : public std::false_type {};

template <typename Base, typename T>
struct is_nvp_type<Nvp<Base, T>> : public std::true_type {};

template <typename Base, typename T, typename Parent>
struct is_nvp_type<NvpChild<Base, T, Parent>> : public std::true_type {};

template <typename T>
struct is_nvp_type<FreeNvp<T>> : public std::true_type {};

/* Create a name-value pair from a object member and its name */
template <typename Base, typename T>
Nvp<Base, T> constexpr makeNvp(T Base::*t, const char* name) {
    return Nvp<Base, T>(t, name);
}

/**
 * Constructs a name-value pair that is a subfield of a `parent` object.
 * The resulting name-value pair will have the name "rootfield.subfield".
 */
template <typename Base, typename T, typename Parent>
NvpChild<Base, T, Parent> constexpr makeNvpWithParent(const Nvp<Base, T>& child,
                                                      const Parent& parent) {
    return NvpChild<Base, T, Parent>(child.t, child.name, parent);
}

/**
 * hasField determines whether a type Base has a member of the given type T as
 * the Nth member out of M total members which have name value pairs.
 */
// By default, if N>=M the index is out of bounds and hasField is false-y.
template <typename Base, typename T, size_t N, size_t M, bool = (N < M)>
struct hasField : public std::false_type {};

// Nth member in the Base::fields tuple (i.e. the list of fields for which we
// have name-value pairs)
// Must have same type as the given argument.
template <typename Base, typename T, size_t N, size_t M>
struct hasField<Base, T, N, M, true>
    : public std::is_same<T Base::*, decltype(std::get<N>(Base::mongo_odm_mapped_fields()).t)> {};

/**
 * wrapbool iterates through the fields and returns true if a matching member exists, and false
 * otherwise.
 */

// forward declarations for wrapbool
template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && !hasField<Base, T, N, M>::value, bool> wrapbool();

template <typename Base, typename T, size_t N, size_t M, T Base::*>
constexpr std::enable_if_t<N == M, bool> wrapbool();

template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && hasField<Base, T, N, M>::value, bool> wrapbool() {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return true;
    } else {
        return wrapbool<Base, T, N + 1, M, t>();
    }
}

template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && !hasField<Base, T, N, M>::value, bool> wrapbool() {
    return wrapbool<Base, T, N + 1, M, t>();
}

template <typename Base, typename T, size_t N, size_t M, T Base::*>
constexpr std::enable_if_t<N == M, bool> wrapbool() {
    return false;
}

/**
 * wrapimpl uses template arguments N and M to iterate over the fields of a Base
 * class, and returns the name-value pair corresponding to the given member
 * field.
 */

// forward declarations for wrapimpl
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T>> wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t);

// If Nth field has same type as T, check that it points to the same member.
// If not, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t) {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return std::get<N>(Base::mongo_odm_mapped_fields());
    } else {
        return wrapimpl<Base, T, N + 1, M>(t);
    }
}

// If current field doesn't match the type of T, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t) {
    return wrapimpl<Base, T, N + 1, M>(t);
}

// If N==M, we've gone past the last field, return nullptr.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T>> wrapimpl(T Base::*) {
    return Nvp<Base, T>(nullptr, nullptr);
}

/**
 * Returns a name-value pair corresponding to the given member pointer.
 * @tparam Base The class containing the member in question
 * @tparam T    The type of the member
 * @param t     A member pointer to an element of type T in the class Base.
 * @return      The name-value pair corresponding to this member pointer.
 */
template <typename Base, typename T>
constexpr const Nvp<Base, T> wrap(T Base::*t) {
    return wrapimpl<Base, T, 0, std::tuple_size<decltype(Base::mongo_odm_mapped_fields())>::value>(
        t);
}

/**
 * A struct that has a call() method that returns a name-value pair corresponding
 * to the given member pointer,
 * but only if such a member exists.
 */
template <typename T, T, typename = void>
struct hasCallIfFieldIsPresent {};

template <typename Base, typename T, T Base::*ptr>
struct hasCallIfFieldIsPresent<
    T Base::*, ptr,
    std::enable_if_t<wrapbool<
        Base, T, 0, std::tuple_size<decltype(Base::mongo_odm_mapped_fields())>::value, ptr>()>> {
    static constexpr const Nvp<Base, T> call() {
        return wrap(ptr);
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
