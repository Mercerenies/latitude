#ifndef PROTECTION_HPP
#define PROTECTION_HPP

class Protection {
private:
    unsigned char internal;
    explicit Protection(unsigned char impl);
public:
    static const Protection NO_PROTECTION;
    static const Protection PROTECT_ASSIGN;
    static const Protection PROTECT_DELETE;
    Protection() = default;
    Protection& operator&=(Protection other);
    Protection& operator|=(Protection other);
    friend Protection operator&(Protection a, Protection b);
    friend Protection operator|(Protection a, Protection b);
    friend bool operator==(Protection a, Protection b);
    friend bool operator!=(Protection a, Protection b);
};

Protection operator&(Protection a, Protection b);
Protection operator|(Protection a, Protection b);
bool operator==(Protection a, Protection b);
bool operator!=(Protection a, Protection b);

#endif // PROTECTION_HPP
