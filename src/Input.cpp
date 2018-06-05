
#include "Input.hpp"
#include "Platform.hpp"
#include <iostream>
#include <list>
#include <cassert>

#ifdef USE_POSIX
#include <unistd.h>
#include <termios.h>
#endif

namespace ReadLine {

    std::string readBasic() {
        std::string result;
        getline(std::cin, result);
        return result;
    }

#ifdef USE_POSIX

    static std::list<std::string> history;
    static constexpr int MAX_HISTORY_SIZE = 99;

    using history_iterator = std::list<std::string>::iterator;

    bool isTermSmart() {
        char* term = std::getenv("TERM");
        bool tty = isatty(fileno(stdin));
        bool dumb = (term && (std::string(term) == "dumb"));
        return tty && !dumb;
    }

    void _replaceText(std::string input, std::string text) {
        for (int i = 0; i < (int)input.size(); i++)
            std::cout << '\b';
        for (int i = 0; i < (int)input.size(); i++)
            std::cout << ' ';
        for (int i = 0; i < (int)input.size(); i++)
            std::cout << '\b';
        std::cout << text;
    }

    bool _readChar(history_iterator& iter, std::string*& input) {
        char ch = (char)getchar();
        switch (ch) {
        case 0x7f: // Backspace
            if (!input->empty()) {
                input->pop_back();
                std::cout << "\b \b";
            }
            break;
        case 0x1b: // Escape
            {
                char next = getchar();
                char last = getchar();
                if (next == '[') {
                    switch (last) {
                    case 'A': // Up
                        ++iter;
                        if (iter == history.end())
                            iter = history.begin();
                        assert(iter != history.end());
                        _replaceText(*input, *iter);
                        input = &(*iter);
                        break;
                    case 'B': // Down
                        if (iter == history.begin())
                            iter = history.end();
                        --iter;
                        assert(iter != history.end());
                        _replaceText(*input, *iter);
                        input = &(*iter);
                        break;
                    case 'C': // Right
                    case 'D': // Left
                        break;
                    }
                }
            }
            break;
        case '\n':
            std::cout << std::endl;
            history.front() = *input;
            return false;
        case 0x04: // EOT
            // TODO Handle EOT (0x04) correctly (Currently just ignored)
            break;
        default:
            input->push_back(ch);
            std::cout << ch;
            break;
        }
        return true;
    }

    class ReadLineCleanupHandler {
    private:
        termios oldattr, newattr;
    public:
        ReadLineCleanupHandler(termios oldattr, termios newattr)
            : oldattr(oldattr), newattr(newattr) {
            tcsetattr(fileno(stdin), TCSANOW, &newattr);
        }
        ~ReadLineCleanupHandler() {
            tcsetattr(fileno(stdin), TCSANOW, &oldattr);
        }
    };

    std::string _readLine() {
        auto fn = fileno(stdin);
        termios oldattr, newattr;

        tcgetattr(fn, &oldattr);
        newattr = oldattr;
        newattr.c_lflag &= ~( ICANON | ECHO );
        ReadLineCleanupHandler cleanup { oldattr, newattr };

        history.push_front("");
        history_iterator iter = history.begin();
        std::string* input = &history.front();
        while (_readChar(iter, input));

        while (history.size() > MAX_HISTORY_SIZE)
            history.pop_back();

        return history.front();

    }

    std::string readRich() {
        if (!isTermSmart())
            return readBasic();

        return _readLine();

    }

#endif

#ifdef USE_WINDOWS

    bool isTermSmart() {
        return false;
    }

    std::string readRich() {
        return readBasic();
    }

#endif

#ifdef USE_NULL

    bool isTermSmart() {
        return false;
    }

    std::string readRich() {
        return readBasic();
    }

#endif

}
