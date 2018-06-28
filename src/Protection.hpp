//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PROTECTION_HPP
#define PROTECTION_HPP

/// \file
///
/// \brief The Protection class and its binary operators.

/// \brief An enum-like class representing the possible protection
/// schemes.
///
/// A Protection instance defines a means of protecting a slot on a
/// Latitude object. Protection instances cannot be explicitly
/// constructed. To use this class, start with the static constants
/// defined in the class and combine them using the bitwise
/// combination operators provided by the class.
class Protection {
private:
    unsigned char internal;
    explicit Protection(unsigned char impl);
public:

    /// \brief A constant representing a lack of protection.
    ///
    /// This instance provides no protection to the slot. It is the
    /// identity object of `operator|` and the zero object of
    /// `operator&`.
    static const Protection NO_PROTECTION;

    /// \brief A constant representing assignment protection.
    ///
    /// Slots with this type of protection cannot be assigned to using
    /// the := or ::= operators in Latitude. Likewise, attempting to
    /// assign to the slot using reflection (`Slots put` or `Object
    /// slot=`) will fail.
    static const Protection PROTECT_ASSIGN;

    /// \brief A constant representing deletion protection.
    ///
    /// Slots with this type of protection cannot be deleted using
    /// `Slots delete` or similar operators.
    static const Protection PROTECT_DELETE;

    /// \brief Explicitly constructs a Protection instance equivalent
    /// to Protection::NO_PROTECTION.
    Protection() = default;

    /// Removes any protection from `this` which is not present in
    /// `other`. Returns `this`.
    ///
    /// \param other the filtering Protection object
    /// \return `this`
    Protection& operator&=(Protection other);

    /// Adds all the protections present in `other` to `this`. Returns
    /// `this`.
    ///
    /// \param other the additive Protection object
    /// \return `this`
    Protection& operator|=(Protection other);

    friend Protection operator&(Protection a, Protection b);
    friend Protection operator|(Protection a, Protection b);
    friend bool operator==(Protection a, Protection b);
    friend bool operator!=(Protection a, Protection b);

};

/// Returns a new Protection instance consisting of the intersection
/// of protections present in the two arguments.
///
/// \param a the first object
/// \param b the second object
/// \return the intersection Protection object
Protection operator&(Protection a, Protection b);

/// Returns a new Protection instance consisting of the union of
/// protections present in the two arguments.
///
/// \param a the first object
/// \param b the second object
/// \return the union Protection object
Protection operator|(Protection a, Protection b);

/// Compares the two Protection instances. Two Protection instances
/// are equal if and only if they provide the exact same protections.
/// This relation is an equivalence relation.
///
/// \param a the first object
/// \param b the second object
/// \return whether the objects are equal
bool operator==(Protection a, Protection b);

/// Compares the two Protection instances. Two Protection instances
/// are equal if and only if they provide the exact same protections.
/// This relation is an equivalence relation.
///
/// \param a the first object
/// \param b the second object
/// \return whether the objects are unequal
bool operator!=(Protection a, Protection b);

#endif // PROTECTION_HPP
