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

#include <mongo_odm/expression_syntax.hpp>
#include <mongo_odm/macros.hpp>
#include <mongo_odm/util.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <typename NvpT, typename T>
class nvp_base;

struct current_date_t {
    constexpr current_date_t() {
    }
};
constexpr current_date_t current_date;

/**
 * An object that represents a name-value pair of a member in an object.
 * It is templated on the class of the member and its type.
 */
template <typename Base, typename T>
class nvp : public nvp_base<nvp<Base, T>, T> {
   public:
    using type = T;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;

    /**
     * Create a name-value pair from a member pointer and a name.
     * @param  t    A pointer to the member
     * @param  name The name of the member
     */
    constexpr nvp(T Base::*t, const char* name) : t(t), name(name) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr update_expr<nvp<Base, T>, no_opt_type> operator=(const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
    * Creates an expression that sets a date value to the current date.
    * This is only enabled for std::chrono::time/duration values, as well as b_date.
    */
    template <typename U = no_opt_type>
    constexpr std::enable_if_t<is_date_v<U>, current_date_expr<nvp>> operator=(
        const current_date_t&) const {
        return {*this, true};
    }

    /**
     * Creates an expression that sets a date value to the current date.
     * This is only enabled for std::chrono::time/duration values, as well as b_date.
     */
    template <typename U = no_opt_type>
    constexpr std::enable_if_t<std::is_same<bsoncxx::types::b_timestamp, U>::value,
                               current_date_expr<nvp>>
    operator=(const current_date_t&) const {
        return {*this, false};
    }

    /**
     * Returns the name of this field.
     * @return The field name as a string.
     */
    std::string get_name() const {
        return name;
    }

    std::string& append_name(std::string& s) const {
        return s.append(name);
    }

    T Base::*t;
    const char* name;
};

/**
 * Class that represents a name-value pair for a field of an object that is a member of another
 * object.
 * The name of this name-value pair will be "<parent-name>.field_name".
 * The parenet can be either an nvp, or another nvp_child.
 * @tparam Base The type of the object that contains this field
 * @tparam T    The type of the field
 * @tparam Parent The type of the parent name-value pair, either an nvp<...> or nvp_child<...>.
 */
template <typename Base, typename T, typename Parent>
class nvp_child : public nvp_base<nvp_child<Base, T, Parent>, T> {
   public:
    using type = T;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;

    constexpr nvp_child(T Base::*t, const char* name, const Parent& parent)
        : t(t), name(name), parent(parent) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr update_expr<nvp_child<Base, T, Parent>, T> operator=(const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
    * Creates an expression that sets a date value to the current date.
    * This is only enabled for std::chrono::time/duration values, as well as b_date.
    */
    template <typename U = no_opt_type>
    constexpr std::enable_if_t<is_date_v<U>, current_date_expr<nvp_child>> operator=(
        const current_date_t&) const {
        return {*this, true};
    }

    /**
     * Creates an expression that sets a date value to the current date.
     * This is only enabled for std::chrono::time/duration values, as well as b_date.
     */
    template <typename U = no_opt_type>
    constexpr std::enable_if_t<std::is_same<bsoncxx::types::b_timestamp, U>::value,
                               current_date_expr<nvp_child>>
    operator=(const current_date_t&) const {
        return {*this, false};
    }

    /**
     * Returns the name of this field, in dot notation.
     * @return The fully qualified name of this field in dot notation.
     */
    std::string get_name() const {
        std::string s;
        return append_name(s);
    }

    /**
     * Returns the qualified name of this field in dot notation, i.e. "parent.child".
     * @return A string containing the name of this field in dot notation.
     */
    std::string& append_name(std::string& s) const {
        return parent.append_name(s).append(1, '.').append(name);
    }

    T Base::*t;
    const char* name;
    const Parent& parent;
};

template <typename NvpT>
class array_element_nvp
    : public nvp_base<array_element_nvp<NvpT>, iterable_value_t<typename NvpT::no_opt_type>> {
   public:
    using type = iterable_value_t<typename NvpT::no_opt_type>;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<type>;

    constexpr array_element_nvp(const NvpT& nvp, std::size_t i) : _nvp(nvp), _i(i) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr update_expr<array_element_nvp<NvpT>, no_opt_type> operator=(
        const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
     * Returns the qualified name of this field in dot notation, i.e. "array.i".
     * @return A string containing the name of this field in dot notation.
     */
    std::string get_name() const {
        std::string s;
        return append_name(s);
    }

    /**
     * Returns the qualified name of this field in dot notation, i.e. "field.<index>", where index
     * is a number.
     * @return A string containing the name of this field in dot notation.
     */
    std::string& append_name(std::string& s) const {
        return _nvp.append_name(s).append(1, '.').append(std::to_string(_i));
    }

   private:
    const NvpT& _nvp;
    const std::size_t _i;
};

/**
 * Represents a field that does not have a name, i.e. an array element.
 * This is used in $elemMatch expressions on scalar arrays, where the elements can be compared but
 * don't have fields of their own.
 * @tparam T    The type of the field.
 */
template <typename T>
class free_nvp : public nvp_base<free_nvp<T>, T> {
   public:
    // In case this corresponds to an optional type, store the underlying type.
    using no_opt_type = remove_optional_t<T>;
    using type = T;

    // TODO This shouldn't have a name, but it needs to work with Expressions when building queries.
    std::string& append_name(std::string& s) const {
        return s;
    }
};

template <typename NvpT>
struct is_free_nvp : public std::false_type {};

template <typename T>
struct is_free_nvp<free_nvp<T>> : public std::true_type {};

template <typename T>
constexpr bool is_free_nvp_v = is_free_nvp<T>::value;

/**
 * Represents the $ operator applied to a field.
 */
template <typename NvpT>
class dollar_operator_nvp : public nvp_base<dollar_operator_nvp<NvpT>, typename NvpT::type> {
   public:
    using type = iterable_value_t<typename NvpT::no_opt_type>;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<type>;

    constexpr dollar_operator_nvp(const NvpT& nvp) : _nvp(nvp) {
    }

    /**
     * Creates an update expression that sets the field to the given value.
     */
    constexpr update_expr<dollar_operator_nvp<NvpT>, no_opt_type> operator=(
        const no_opt_type& val) const {
        return {*this, val, "$set"};
    }

    /**
     * Returns the qualified name of this field in dot notation, i.e. "array.i".
     * @return A string containing the name of this field in dot notation.
     */
    std::string get_name() const {
        std::string s;
        return append_name(s);
    }

    /**
     * Returns the name of this field with the $ operator, i.e. "field.$"
     * @return A string containing the name of this field in dot notation.
     */
    std::string& append_name(std::string& s) const {
        return _nvp.append_name(s).append(1, '.').append(1, '$');
    }

   private:
    const NvpT& _nvp;
};

/**
 * A CRTP base class that contains member functions for name-value pairs.
 * These functions are identical between nvp<...> and nvp_child<...>, but their return types are
 * templated on the nvp's types, so they are defined here using CRTP.
 * @tparam NvpT The type of the name-value pair
 * @tparam T    The type of the field referred to by the name-value pair.
 */
template <typename NvpT, typename T>
class nvp_base {
   private:
    // Type trait that checks if the given iterable class contains the same value type as either
    // a) this nvp's type, or
    // b) this nvp's value type, if this nvp is also an iterable.
    template <typename Iterable, typename Default = void>
    using enable_if_matching_iterable_t =
        std::enable_if_t<is_iterable_not_string_v<Iterable> &&
                             std::is_convertible<iterable_value_t<Iterable>,
                                                 iterable_value_t<remove_optional_t<T>>>::value,
                         Default>;

   public:
    using type = T;
    // In case this field is wrapped in an optional, store the underlying type.
    using no_opt_type = remove_optional_t<T>;
    using child_base_type = iterable_value_t<no_opt_type>;

    /**
     * Chains two name-value pairs to access a sub-field, i.e. a field with the name "parent.child".
     * This also allows accessing the fields of documents that are in an array.
     * @tparam U    The type of the child NVP
     * @param child An NVP that corresponds to a sub-field of this NVP. Its base class must be the
     * same as this field's current type. If this field is an array of documents,
     * then 'child' corresponds to a subfield of the documents in this array.
     * @return      An NVP with the same base class and type as the subfield, but with a link to a
     * parent so that its name is qualified in dot notation.
     */
    template <typename U>
    constexpr nvp_child<iterable_value_t<no_opt_type>, U, NvpT> operator->*(
        const nvp<iterable_value_t<no_opt_type>, U>& child) const {
        return {child.t, child.name, *static_cast<const NvpT*>(this)};
    }

    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr array_element_nvp<NvpT> operator[](std::size_t i) const {
        return {*static_cast<const NvpT*>(this), i};
    }

    /**
     * Creates a sort expression that sorts documents by this field.
     * @param ascending Whether to sort by ascending order (+1 in MongoDB syntax).
     *                  If false, sorts by descending order (-1 in MongoDB syntax).
     * @return a sort_expr that reprsents the sort expression {field: +/-1}.
     */
    constexpr sort_expr<NvpT> sort(bool ascending) const {
        return {*static_cast<const NvpT*>(this), ascending};
    }

    /**
     * Creates an expression that checks whether the value of this field matches any value in the
     * given iterable.
     * @tparam Iterable A type that works in range-based for loops, and yields objects convertible
     * to the type of this name-value pair.
     */
    template <typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr comparison_expr<NvpT, Iterable> in(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$in"};
    }

    /**
     * Creates an expression that checks whether the value of this field matches none of the values
     * in the given iterable.
     * @tparam Iterable A type that works in range-based for loops, and yields objects convertible
     * to the type of this name-value pair.
     */
    template <typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr comparison_expr<NvpT, Iterable> nin(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$nin"};
    }

    /**
     * Creates an expression that checks the existence of a certain field.
     * This is only enabled for fields that are optional types.
     * {field: {$exists: <bool>}}
     * @param exists    If false, checks that the given field does *not* exist in a document.
     *                  True by default.
     */
    template <typename U = T, typename = std::enable_if_t<is_optional_v<U>>>
    constexpr comparison_expr<NvpT, bool> exists(const bool& exists) const {
        return {*static_cast<const NvpT*>(this), exists, "$exists"};
    }

    /**
     * Creates a mod_expr that represents a query with the $mod operator.
     * Such a query essentially checks that "nvp_value % divisor == remainder"
     * @param divisor   The divisor for the modulus operation
     * @param remainder   The remainder after dividing a value by `divisor`
     * @returns A mod_expr representing this query.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<std::is_arithmetic<U>::value>>
    constexpr mod_expr<NvpT> mod(const int& divisor, const int& remainder) const {
        return {*static_cast<const NvpT*>(this), divisor, remainder};
    }

    /**
     * Creates a comparison expression that represents a query with a $regex operator.
     * Such a query only works for string fields.
     * @param regex     The regex to check against.
     * @param options   Options to pass to the regex.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_string_v<U>>>
    constexpr comparison_value_expr<NvpT, bsoncxx::types::b_regex> regex(
        const char* regex, const char* options) const {
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
              typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr comparison_expr<NvpT, Iterable> all(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$all"};
    }

    /**
     * Creates a query with the $elemMatch operator that finds elements in this field that match the
     * given queries. This can include "free" expressions, that don't contain a field name,
     * in the case of a scalar array. e.g. "arr: {$elemMatch: {$gt: 4, .....}}"
     * This is only enabled if the current field is an iterable itself.
     *
     * @tparam Expr     A query expression object.
     * @param queries   Queries to comapre values against.
     * @returns         A comparison expression with the $elemMatch operator.
     */
    template <typename Expr, typename = std::enable_if_t<details::is_query_expression_v<Expr>>,
              typename U = no_opt_type, typename = std::enable_if_t<is_iterable_v<U>>>
    constexpr comparison_expr<NvpT, Expr> elem_match(const Expr& queries) const {
        return {*static_cast<const NvpT*>(this), queries, "$elemMatch"};
    }

    /**
     * Constructs a nameless name-value-pair that corresponds to an element in a scalar array, if
     * this field is an array. This is used to create expressions with $elemMatch.
     * This is only enabled if this current field is an array.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr free_nvp<iterable_value_t<no_opt_type>> element() const {
        return {};
    }

    /**
     * Creates an array query expression with the $size operator.
     * This is only enabled if this current field is an array.
     * @param n The size the array should be.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr comparison_expr<NvpT, std::int64_t> size(const std::int64_t& n) const {
        return {*static_cast<const NvpT*>(this), n, "$size"};
    }

    /* bitwise queries, enabled only for integral types. */

    /**
     * Creates a query that uses the $bitsAllSet operator to check a numerical field with a bitmask.
     * $bitsAllSet checks that every bit in the bitmask is set in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAllSet operator
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename Mask,
              typename = std::enable_if_t<std::is_integral<Mask>::value ||
                                          std::is_same<Mask, bsoncxx::types::b_binary>::value>>
    constexpr comparison_expr<NvpT, Mask> bits_all_set(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAllSet"};
    }

    /**
     * Creates a query that uses the $bitsAllSet operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename... Args>
    constexpr comparison_value_expr<NvpT, std::int64_t> bits_all_set(std::int64_t pos1,
                                                                     std::int64_t pos2,
                                                                     Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAllSet"};
    }

    /**
     * Creates a query that uses the $bitsAnySet operator to check a numerical field with a bitmask.
     * $bitsAnySet checks that a least one bit in the bitmask is set in the field's value.
     * @param bitmask - A bitmask to pass to the $bitsAnySet operator
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename Mask,
              typename = std::enable_if_t<std::is_integral<Mask>::value ||
                                          std::is_same<Mask, bsoncxx::types::b_binary>::value>>
    constexpr comparison_expr<NvpT, Mask> bits_any_set(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAnySet"};
    }

    /**
     * Creates a query that uses the $bitsAnySet operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename... Args>
    constexpr comparison_value_expr<NvpT, std::int64_t> bits_any_set(std::int64_t pos1,
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
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename Mask,
              typename = std::enable_if_t<std::is_integral<Mask>::value ||
                                          std::is_same<Mask, bsoncxx::types::b_binary>::value>>
    constexpr comparison_value_expr<NvpT, Mask> bits_all_clear(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAllClear"};
    }

    /**
     * Creates a query that uses the $bitsAllClear operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename... Args>
    constexpr comparison_value_expr<NvpT, std::int64_t> bits_all_clear(std::int64_t pos1,
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
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename Mask,
              typename = std::enable_if_t<std::is_integral<Mask>::value ||
                                          std::is_same<Mask, bsoncxx::types::b_binary>::value>>
    constexpr comparison_expr<NvpT, Mask> bits_any_clear(const Mask& bitmask) const {
        return {*static_cast<const NvpT*>(this), bitmask, "$bitsAnyClear"};
    }

    /**
     * Creates a query that uses the $bitsAnyClear operator to check a series of bits, given as bit
     * positions. This function has two positional arguments to distinguish from the signature that
     * takes a bit mask (see above)
     * @param pos1  The first bit position to check
     * @param pos2  The second bit position to check
     * @param positions...  Variadic argument containing further bit positions.
     * @returns A comparison_expr representing this query
     */
    template <typename U = no_opt_type,
              typename = std::enable_if_t<std::is_integral<U>::value ||
                                          std::is_same<U, bsoncxx::types::b_binary>::value>,
              typename... Args>
    constexpr comparison_value_expr<NvpT, std::int64_t> bits_any_clear(std::int64_t pos1,
                                                                       std::int64_t pos2,
                                                                       Args... positions) const {
        return {*static_cast<const NvpT*>(this), bit_positions_to_mask(pos1, pos2, positions...),
                "$bitsAnyClear"};
    }

    constexpr update_expr<NvpT, no_opt_type> set_on_insert(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$setOnInsert"};
    }

    /* Update operators */

    /**
     * Creates an expression that unsets the current field. The field must be of optional type.
     * @returns An unset_expr that unsets the current field.
     */
    template <typename U = T, typename = std::enable_if_t<is_optional_v<U>>>
    constexpr unset_expr<NvpT> unset() const {
        return {*static_cast<const NvpT*>(this)};
    }

    /**
     * Creates an expression that uses the $min operator to only update a field if the new value is
     * lower than the current value.
     * @param val   The (tentative) new value.
     * @returns     An UpdateExpression with the $min operator.
     */
    constexpr update_expr<NvpT, no_opt_type> min(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$min"};
    }

    /**
     * Creates an expression that uses the $max operator to only update a field if the new value is
     * greater than the current value.
     * @param val   The (tentative) new value.
     * @returns     An UpdateExpression with the $max operator.
     */
    constexpr update_expr<NvpT, no_opt_type> max(const no_opt_type& val) const {
        return {*static_cast<const NvpT*>(this), val, "$max"};
    }

    /* Array update operators */
    /**
     * Returns a name-value pair with the $ operator appended to it.
     * When used in an update expression, this modifies the first array element that satisfies a
     * query.
     * @returns a dollar_operator_nvp that corresponds to "<field_name>.$"
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr dollar_operator_nvp<NvpT> first_match() const {
        return {*static_cast<const NvpT*>(this)};
    }

    /**
     * Creates an update expression with the $pop operator.
     * This is only enabled if the current field is an array type.
     * @param last  If true, removes the element from the end of the array.
     *              If false, from the start.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr update_value_expr<NvpT, int> pop(bool last) const {
        return {*static_cast<const NvpT*>(this), last ? 1 : -1, "$pop"};
    }

    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given value exactly.
     * This is only enabled if the current field is an array type.
     * @param val   The value to remove. This must match the type *contained* by this array field.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr update_expr<NvpT, iterable_value_t<no_opt_type>> pull(
        const iterable_value_t<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, "$pull"};
    }
    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given query.
     * This query can contain free expressions, similarly to the $elemMatch operator.
     * This is only enabled if the current field is an array type.
     * @tparam Expr The type of the given query, must be a query expression.
     * @param  expr A query expression against which to compare
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>,
              typename Expr>
    constexpr std::enable_if_t<details::is_query_expression_v<Expr>, update_expr<NvpT, Expr>> pull(
        const Expr& expr) const {
        return {*static_cast<const NvpT*>(this), expr, "$pull"};
    }

    /**
     * Creates an update expression with the $pull operator, that removes an element if it matches
     * the given value exactly.
     * This is only enabled if the current field is an array type.
     * @param iter   An iterable containing the values to remove.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr update_expr<NvpT, Iterable> pull_all(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, "$pullAll"};
    }

    /**
     * Creates an update expression with the $addToSet operator,
     * that adds a single value to an array, if it is unique.
     * This is only enabled if the current field is an array type.
     * @param val   The value to add.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr add_to_set_update_expr<NvpT, iterable_value_t<no_opt_type>> add_to_set(
        const iterable_value_t<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, false};
    }

    /**
     * Creates an update expression with the $addToSet operator and the $each modifier,
     * that adds a list of value to an array, only keeping the unique values.
     * This is only enabled if the current field is an array.
     * @param iter   The list of values to add.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>>
    constexpr add_to_set_update_expr<NvpT, Iterable> add_to_set(const Iterable& iter) const {
        return {*static_cast<const NvpT*>(this), iter, true};
    }

    /**
     * Creates an update epxression with the $push operator, that adds a single value to an array.
     * This is only enabled if the current field is an array.
     * @param val   The value to add.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>>
    constexpr push_update_expr<NvpT, iterable_value_t<no_opt_type>> push(
        const iterable_value_t<no_opt_type>& val) const {
        return {*static_cast<const NvpT*>(this), val, false};
    }

    /**
     * Creates an update epxression with the $push operator and the $each modifier,
     * that adds a list of value to an array. Further modifiers can be given as arguments to this
     * function, or by modifying the resulting push_update_expr object.
     * This is only enabled if the current field is an array.

     * @tparam Iterable An iterable that contains the same value type as this field's array.
     * @tparam Sort     The type of the given sort expression, if one is given.
     * @param  iter     The iterable containing values to add.
     * @param  slice    An optional argument containing the value of the $slice modifier.
     * @param  sort    An optional argument containing an expression for the $sort modifier.
     * @param  position    An optional argument containing the value of the $position modifier.
     */
    template <typename U = no_opt_type, typename = std::enable_if_t<is_iterable_not_string_v<U>>,
              typename Iterable, typename = enable_if_matching_iterable_t<Iterable>,
              typename Sort = int,
              typename = std::enable_if_t<details::is_sort_expression_v<Sort> ||
                                          std::is_same<int, Sort>::value>>
    constexpr push_update_expr<NvpT, Iterable, Sort> push(
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
struct is_nvp : public std::false_type {};

template <typename Base, typename T>
struct is_nvp<nvp<Base, T>> : public std::true_type {};

template <typename Base, typename T, typename Parent>
struct is_nvp<nvp_child<Base, T, Parent>> : public std::true_type {};

template <typename T>
struct is_nvp<free_nvp<T>> : public std::true_type {};

template <typename NvpT>
struct is_nvp<array_element_nvp<NvpT>> : public std::true_type {};

template <typename T>
constexpr bool is_nvp_v = is_nvp<T>::value;

/* Create a name-value pair from a object member and its name */
template <typename Base, typename T>
nvp<Base, T> constexpr make_nvp(T Base::*t, const char* name) {
    return nvp<Base, T>(t, name);
}

/**
 * Constructs a name-value pair that is a subfield of a `parent` object.
 * The resulting name-value pair will have the name "rootfield.subfield".
 */
template <typename Base, typename T, typename Parent>
nvp_child<Base, T, Parent> constexpr make_nvp_with_parent(const nvp<Base, T>& child,
                                                          const Parent& parent) {
    return nvp_child<Base, T, Parent>(child.t, child.name, parent);
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
constexpr std::enable_if_t<N == M, const nvp<Base, T>> wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const nvp<Base, T>> wrapimpl(
    T Base::*t);

// If Nth field has same type as T, check that it points to the same member.
// If not, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && hasField<Base, T, N, M>::value, const nvp<Base, T>> wrapimpl(
    T Base::*t) {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return std::get<N>(Base::mongo_odm_mapped_fields());
    } else {
        return wrapimpl<Base, T, N + 1, M>(t);
    }
}

// If current field doesn't match the type of T, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const nvp<Base, T>> wrapimpl(
    T Base::*t) {
    return wrapimpl<Base, T, N + 1, M>(t);
}

// If N==M, we've gone past the last field, return nullptr.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const nvp<Base, T>> wrapimpl(T Base::*) {
    return nvp<Base, T>(nullptr, nullptr);
}

/**
 * Returns a name-value pair corresponding to the given member pointer.
 * @tparam Base The class containing the member in question
 * @tparam T    The type of the member
 * @param t     A member pointer to an element of type T in the class Base.
 * @return      The name-value pair corresponding to this member pointer.
 */
template <typename Base, typename T>
constexpr const nvp<Base, T> wrap(T Base::*t) {
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
    static constexpr const nvp<Base, T> call() {
        return wrap(ptr);
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
