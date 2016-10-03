#include "util.hpp"
#include "securestring.hpp"

int main(void) {
    {
        char cstr[] = {'f', 'o', 'o', '\0'};
        SecureString s(cstr);

        for(size_t i = 0; i < sizeof(cstr); i += 1) {
            verify(cstr[i] == '\0');
        }
        verify(s.size() == 3);
    }

    {
        uint8_t cstr[] = {'f', 'o', 'o', '\0'};
        SecureString s(cstr, sizeof(cstr));

        for(size_t i = 0; i < sizeof(cstr); i += 1) {
            verify(cstr[i] == '\0');
        }
        verify(s.size() == 4);
    }

    {
        SecureString s(4);
        s.data()[0] = 'f';
        s.data()[1] = 'o';
        s.data()[2] = 'o';
        s.data()[3] = '\0';

        verify(s.size() == 4);
    }

    {
        SecureString s;
        verify(s.data() == nullptr);
        verify(s.size() == 0);
    }

    {
        char cstr[] = {'f', 'o', 'o', '\0'};
        SecureString s1(cstr);
        SecureString s2;

        s1.moveInto(s2);
        verify(s1.size() == 0);
        verify(s2.size() == 3);

        for(size_t i = 0; i < sizeof(cstr); i += 1) {
            verify(cstr[i] == '\0');
        }

        for(size_t i = 0; i < s1.size(); i += 1) {
            verify(s1.data()[i] == '\0');
        }

        verify(strcmp(s2.c_str(), "foo") == 0);
    }

    return 0;
}
