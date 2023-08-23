#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>
using namespace std;

enum CommandType {
    C_ARITHMETIC,
    C_PUSH,
    C_POP,
    C_LABEL,
    C_GOTO,
    C_IF,
    C_FUNCTION,
    C_RETURN,
    C_CALL
};

class Parser {
private:
    string currCommand;
    ifstream file;
public:
    Parser(string filename) : file(filename) {}
    bool fileExists() {
        return file.is_open();
    }
    bool hasMoreCommands() {
        return file.peek() != EOF;
    }
    void advance() {
        do{
            getline(file, currCommand);
            int x = currCommand.find("//");
            if(x != -1) currCommand.erase(x);
            currCommand.erase(remove(currCommand.begin(), currCommand.end(), '\n'), currCommand.end());
            currCommand.erase(remove(currCommand.begin(), currCommand.end(), '\r'), currCommand.end());
        }while(hasMoreCommands() && currCommand.size() == 0);
    }
    CommandType commandType() {
        if (currCommand.substr(0,3) == "pop") return C_POP;
        if (currCommand.substr(0,4) == "push") return C_PUSH;
        if (currCommand.substr(0,4) == "goto") return C_GOTO;
        if (currCommand.substr(0,4) == "call") return C_CALL;
        if (currCommand.substr(0,4) == "func") return C_FUNCTION;
        if (currCommand.substr(0,4) == "if-g") return C_IF;
        return C_ARITHMETIC;
    }
    string arg1() {
        int x = currCommand.find(" ");
        int y = currCommand.find_last_of(" ");
        if(x == y) return currCommand.substr(x+1);
        return currCommand.substr(x+1, y-x-1);
    }
    string arg2() {
        int x = currCommand.find_last_of(" ");
        return currCommand.substr(x+1);
    }
    ~Parser() {
        file.close();
    }
};
class CodeWriter {
private:
    ofstream file;
    int labelCount;
    map<string, string> segmentMap = {
        {"local", "LCL"},
        {"argument", "ARG"},
        {"this", "THIS"},
        {"that", "THAT"},
        {"temp", "R5"},
        {"pointer", "R3"},
        {"static", "16"}
    };
public:
    CodeWriter(string filename) : file(filename), labelCount(0){}
    void writeArithmetic(string command) {
        if (command == "neg" || command == "not") {
            file << "@SP" << endl;
            file << "A=M-1" << endl;
            if (command == "neg") file << "M=-M" << endl;
            else if (command == "not") file << "M=!M" << endl;
            return;
        }
        else{
            file << "@SP" << endl;
            file << "M=M-1" << endl;
            file << "A=M" << endl;
            file << "D=M" << endl;
            file << "A=A-1" << endl;
            if (command == "add") file << "M=M+D" << endl;
            else if (command == "sub") file << "M=M-D" << endl;
            else if (command == "and") file << "M=D&M" << endl;
            else if (command == "or") file << "M=D|M" << endl;
            else {
                file << "D=M-D" << endl;
                file << "@label" << labelCount << endl;
                if(command == "eq") file << "D;JEQ" << endl;
                if(command == "lt") file << "D;JLT" << endl;
                if(command == "gt") file << "D;JGT" << endl;
                file << "@SP" << endl;
                file << "A=M-1" << endl;
                file << "M=0" << endl;
                file << "@label" << labelCount + 1 << endl;
                file << "0;JMP" << endl;
                file << "(label" << labelCount << ")" << endl;
                file << "@SP" << endl;
                file << "A=M-1" << endl;
                file << "M=-1" << endl;
                file << "(label" << labelCount + 1 << ")" << endl;
                labelCount += 2;
            }
        }
    }
    void writePush(string segment, int index) {
        if(segment == "constant") {
            file << "@" << index << endl;
            file << "D=A" << endl;
        }
        else{
            file << "@" << index << endl;
            file << "D=A" << endl;
            file << "@" << segmentMap[segment] << endl;
            if (segment == "temp" || segment == "pointer" || segment == "static") file << "A=D+A" << endl;
            else file << "A=D+M" << endl;
            file << "D=M" << endl;
        }
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
    }
    void writePop(string segment, int index) {
        file << "@" << index << endl;
        file << "D=A" << endl;
        file << "@" << segmentMap[segment] << endl;
        if (segment == "temp" || segment == "pointer" || segment == "static") file << "D=D+A" << endl;
        else file << "D=D+M" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "A=A-1" << endl;
        file << "D=M" << endl;
        file << "A=A+1" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M-1" << endl;
    }
    ~CodeWriter() {
        file.close();
    }
};
int main(int argc, char** argv){
    if(argc<2) {
        cout << "Usage: ./VMtranslator <filename(s)>" << endl;
        return 1;
    }
    for(int i=1;i<argc;i++) {
        string file=argv[i];
        Parser* parser = new Parser(file);
        if(!parser->fileExists()) {
            cout << "File " << file << " does not exist" << endl;
            continue;
        }
        string outfile = file.substr(0, file.find_last_of(".")) + ".asm";
        CodeWriter code(outfile);
        while(parser->hasMoreCommands()) {
            parser->advance();
            switch (parser->commandType())
            {
            case C_ARITHMETIC:
                code.writeArithmetic(parser->arg1());
                break;
            case C_PUSH:
                code.writePush(parser->arg1(), stoi(parser->arg2()));
                break;
            case C_POP:
                code.writePop(parser->arg1(), stoi(parser->arg2()));
                break;
            case C_LABEL:
                break;
            case C_GOTO:
                break;
            case C_IF:
                break;
            case C_FUNCTION:
                break;
            case C_RETURN:
                break;
            case C_CALL:
                break;
            default:
                break;
            }
        }
    }
    return 0;
}