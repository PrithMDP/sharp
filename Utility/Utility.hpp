/**
 * @file Utility.hpp
 * @author Aaryaman Sagar
 *
 * This file contains useful utilities that would have otherwise gone in
 * <utility>
 */

#pragma once

#include <type_traits>

namespace sharp {

/**
 * @function match_forward
 *
 * A function that very much like std::forward just forward the object passed
 * to it with the right value category as determined by the type passed to
 * match_forward.  This can be used to forward another object by considering
 * the value category of somethin different but related
 *
 * This is to be used in scenarios where you get a variable of unknown
 * referenceness and you want to forward another object, maybe related to the
 * first one with the same reference-ness as the one with unknown
 * reference-ness.  The usage for this function is illustrated below
 *
 *  template <typename Something>
 *  decltype(auto) forward_another_thing(Something&& something) {
 *      auto&& another = std::forward<Something>(something).get_another();
 *      return sharp::match_forward<Something, decltype(another)>(another);
 *  }
 *
 * With respect to the implementation of this function, there are several
 * possible cases, each corresponding to a combination of reference-ness of
 * the first template parameter of this function with that of the second,
 *
 *  TypeToMatch -> &   Type -> &
 *  TypeToMatch -> &   Type -> &&
 *  TypeToMatch -> &   Type ->
 *
 *  TypeToMatch ->     Type -> &
 *  TypeToMatch ->     Type -> &&
 *  TypeToMatch ->     Type ->
 *
 *  TypeToMatch -> &&  Type -> &
 *  TypeToMatch -> &&  Type -> &&
 *  TypeToMatch -> &&  Type ->
 *
 * Of these cases the following cases are invalid and should throw errors
 *
 *  TypeToMatch -> &   Type -> &&
 *  TypeToMatch -> &   Type ->
 *
 * Since these two cases will result in the function forwarding an lvalue as
 * an rvalue, which can lead to dangling referneces and the like.  In these
 * cases the implementation fails to compile
 *
 * In all other cases other than the error ones mentioned above, a reference
 * is returned that matches the reference-ness of the type on the left
 */
template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>&);
template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>&&);

/**
 * @function move_if_movable
 *
 * Returns an xvalue referring to the object passed in if the class Type has a
 * constructor that can be invoked with an rvalue of type std::decay_t<Type>&&
 *
 * Useful in templated situations where you don't know if the class you want
 * to move or copy has deleted its constructor or not
 */
template <typename Type, typename>
decltype(auto) move_if_movable(Type&& object);

/**
 * @class Crtp
 *
 * This class is a utility class that makes making CRTP base classes just a
 * little more expressive, instead of using the following ugly construct
 *
 *  static_cast<Derived*>(this)->method()
 *
 * The following much simpler alternative makes things easier
 *
 *  this->instance().method();
 */
template <typename Base>
class Crtp;
template <template <typename> class Base, typename Derived>
class Crtp<Base<Derived>> {
public:

    /**
     * Functions that static cast the this pointer and return a reference to
     * the Derived class, instead of having ugly versions of this inline in
     * every class that is a CRTP base class, these functions can be used
     */
    Derived& instance();
    const Derived& instance() const;
};

/**
 * @class LessPtr
 *
 * A transparent comparator to compare two pointer like types by value of the
 * things they point to
 *
 * For example, this can be used to order a set based on the values of the
 * things pointed to by the pointers in the set
 *
 *      std::set<std::unique_ptr<int>, LessPtr> set_ptrs;
 *      set_ptrs.insert(std::make_unique<int>(1));
 *      set_ptrs.insert(std::make_unique<int>(0));
 *
 *      // output for the following always is "0 1"
 *      for (const auto& ptr : set_ptrs) {
 *          cout << *ptr << ' ';
 *      }
 *      cout << endl;
 */
class LessPtr;

} // namespace sharp

#include <sharp/Utility/Utility.ipp>