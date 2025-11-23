#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include "parse/lexer.hpp"
#include "parse/parser.hpp"
#include "vm/stackitem.hpp"
#include "compile/bytecodecompiler.hpp"
#include "vm/vm.hpp"
using namespace std;


vector<Instruction> compile(CharBuffer* buff) {
    Lexer lexer;
    Parser parser;
    ByteCodeCompiler compiler;
    return compiler.compile(parser.parse(lexer.lex(buff)));
}


void compileAndRun(CharBuffer* buff, int verbosity) {
    VM vm;
    vector<Instruction> code = compile(buff);
    vm.run(code, verbosity);
}

void runScript(string filename, int verbosity) {
    FileStringBuffer* fb = new FileStringBuffer();
    fb->readFile(filename);
    compileAndRun(fb, verbosity);
}

void runCommand(string cmd, int verbosity) {
    cout<< "Running: "<<cmd<<endl;
    StringBuffer* sb = new StringBuffer();
    sb->init(cmd);
    compileAndRun(sb, verbosity);
}

void repl(int vb) {
    bool looping = true;
    StringBuffer* sb = new StringBuffer();
    Lexer lexer;
    Parser parser;
    ByteCodeCompiler compiler;
    VM vm;
    while (looping) {
        string input;
        cout<<"bcvm> ";
        getline(cin, input);
        sb->init(input);
        vector<Instruction> code = compiler.compile(parser.parse(lexer.lex(sb)));
        vm.run(code, vb);
    }
}

int verbosityLevel(char *str) {
    int vlev = 0;
    for (char *x = str; *x; x++)
        if (*x == 'v')
            vlev++;
    return vlev;
}

int main(int argc, char* argv[]) {
    switch (argc) {
        case 1: repl(0); break;
        case 2: repl(verbosityLevel(argv[1]));
        default:
            if (argc == 3 && argv[1][0] == '-') {
                switch (argv[1][1]) {
                    case 'e': runCommand(argv[2], verbosityLevel(argv[1])); break;
                    case 'f': runScript(argv[2], verbosityLevel(argv[1])); break;
                    default: break;
                }
            }
    }
    return 0;
}