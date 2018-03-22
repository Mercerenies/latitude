#ifndef PROTECTION_HPP
#define PROTECTION_HPP

class Protection {
private:
    unsigned char internal;
    explicit Protection(unsigned char impl);
public:
    static Protection NO_PROTECTION;
    static Protection PROTECT_ASSIGN;
    static Protection PROTECT_DELETE;
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
