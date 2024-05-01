Parser<string> name() {
    return CPPARSEC_MAKE(Parser<string>) {
        CPPARSEC_SAVE(firstLetter, upper());
        CPPARSEC_SAVE(restOfName, many(letter()));

        return string({ firstLetter }) + restOfName;
    };
}