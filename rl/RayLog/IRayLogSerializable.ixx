export module Rl.RayLog.ISerializable;

import <string>;

namespace Rl::RayLog
{

/*
 * Abstract interface for serializable RayLog logging classes
 *
 */
export template <class T> class IRayLogSerializable
{
public:
    /* Default destructor for the printer strategy */
    virtual ~IRayLogSerializable() = default;

    /* Must inherit the ToString method (needs a value) */
    virtual constexpr std::string ToString(T type) const
    {
        return {};
    };
};

/*
 * Abstract interface for serializable RayLog logging classes,
 * no parameter needed to convert to string
 */
export class IRayLogSerializable2
{
public:
    /* Default destructor for the printer strategy */
    virtual ~IRayLogSerializable2() = default;

    /* Must inherit the ToString method (needs a value) */
    virtual constexpr std::string ToString() const
    {
        return {};
    };
};

} // namespace Rl::RayLog
