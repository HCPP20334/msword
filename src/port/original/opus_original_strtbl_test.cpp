#include <cstring>

extern "C" {
int IdFromSzgSz(int string_group, char* text);
int CchSzFromSzgId(char* destination, int string_group, int id, int capacity);
}

/* res.c owns this routine in the full engine. The focused storage test keeps
 * its behavior local so it can execute before the resource module is linked. */
extern "C" int ChUpperLookup(const int character) {
    return character >= 'a' && character <= 'z' ? character - ('a' - 'A') : character;
}

int main() {
    constexpr int szgFltNormal = 1;
    constexpr int fltAuthor = 17;

    char keyword[] = "author";
    if (IdFromSzgSz(szgFltNormal, keyword) != fltAuthor) {
        return 1;
    }

    char output[256]{};
    const int count = CchSzFromSzgId(output, szgFltNormal, fltAuthor,
                                    static_cast<int>(sizeof(output)));
    if (count != 7 || std::strcmp(output, "AUTHOR") != 0) {
        return 2;
    }
    return 0;
}
