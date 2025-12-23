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
        ByteCodeGenerator codeGen;
    public:
        Compiler(int verbosity = 0) {
            if (verbosity > 0) {
                lexer = Lexer(true);
                parser = Parser(true);
                codeGen = ByteCodeGenerator(true);
            }
        }
        ConstPool& getConstPool() {
            return codeGen.getConstPool();
        }
        vector<Instruction> compile(CharBuffer* buff) {
            return codeGen.compile(parser.parse(lexer.lex(buff)));
        }
        vector<Instruction> operator()(CharBuffer* buff) {
            return compile(buff);
        }
};

void initStdLib(Compiler& compiler, VM& vm) {
    FileStringBuffer* fb = new FileStringBuffer();
    fb->readFile("/usr/local/bin/vm/stdlib.owl");
    auto code = compiler.compile(fb);
    vm.setConstPool(compiler.getConstPool());
    vm.run(code, 0);
}

void compileAndRun(CharBuffer* buff, int verbosity) {
    VM vm;
    Compiler compiler(verbosity);
    initStdLib(compiler, vm);
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
    Compiler compiler(vb);
    VM vm;
    initStdLib(compiler, vm);
    unsigned int lno = 0;
    while (looping) {
        string input;
        cout<<"Glaux("<<lno++<<")> ";
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