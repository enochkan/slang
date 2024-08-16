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

    Lexer lexer(sourceFile);
    Parser parser(lexer);
    ASTNode* ast = parser.parse();

    CodeGenerator codegen;
    codegen.generate(ast);

    return 0;
}
