#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: slangcc <source-file.sl>" << std::endl;
        return 1;
    }

    std::ifstream sourceFile(argv[1]);
    if (!sourceFile.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }

    try {
        Lexer lexer(sourceFile);
        Parser parser(lexer);
        auto program = parser.parseProgram();

        CodeGenerator codegen;
        codegen.generate(*program, "output");
    } catch (const ParseError& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
