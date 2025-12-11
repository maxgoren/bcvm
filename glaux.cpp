#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include "parse/lexer.hpp"
#include "parse/parser.hpp"
#include "vm/stackitem.hpp"
#include "compile/bcgen.hpp"
#include "vm/vm.hpp"
using namespace std;

class Compiler {
    private:
        Lexer lexer;
        Parser parser;
        ByteCodeGenerator compiler;
    public:
        Compiler() {

        }
        ConstPool& getConstPool() {
            return compiler.getConstPool();
        }
        vector<Instruction> compile(CharBuffer* buff) {
            return compiler.compile(parser.parse(lexer.lex(buff)));
        }
        vector<Instruction> operator()(CharBuffer* buff) {
            return compile(buff);
        }
};

void compileAndRun(CharBuffer* buff, int verbosity) {
    VM vm;
    Compiler compiler;
    vector<Instruction> code = compiler.compile(buff);
    vm.setConstPool(compiler.getConstPool());
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
    Compiler compiler;
    VM vm;
    while (looping) {
        string input;
        cout<<"Glaux> ";
        getline(cin, input);
        sb->init(input);
        vector<Instruction> code = compiler.compile(sb);
        vm.setConstPool(compiler.getConstPool());
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
    srand(time(0));
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