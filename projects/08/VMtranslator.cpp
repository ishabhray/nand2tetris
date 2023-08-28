#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>
#include <boost/algorithm/string/trim.hpp>

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
            boost::algorithm::trim(currCommand);
        }while(hasMoreCommands() && currCommand.size() == 0);
    }
    CommandType commandType() {
        if (currCommand.substr(0,3) == "pop") return C_POP;
        if (currCommand.substr(0,4) == "push") return C_PUSH;
        if (currCommand.substr(0,4) == "goto") return C_GOTO;
        if (currCommand.substr(0,4) == "call") return C_CALL;
        if (currCommand.substr(0,4) == "func") return C_FUNCTION;
        if (currCommand.substr(0,4) == "if-g") return C_IF;
        if (currCommand.substr(0,5) == "label") return C_LABEL;
        if (currCommand.substr(0,6) == "return") return C_RETURN;
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
    int labelCount,staticCnt=16;
    map<string, string> segmentMap = {
        {"local", "LCL"},
        {"argument", "ARG"},
        {"this", "THIS"},
        {"that", "THAT"},
        {"temp", "R5"},
        {"pointer", "R3"},
        {"static", "16"}
    };
    map<string,int>staticMap;
public:
    CodeWriter(string filename) : file(filename), labelCount(0){
        file << "@256" << endl;
        file << "D=A" << endl;
        file << "@SP" << endl;
        file << "M=D" << endl;
        writeCall("Sys.init", 0);
    }
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
            if (command == "add") file << "M=D+M" << endl;
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
    void writePush(string segment, int index, string f="") {
        if (segment == "static") {
            string x = f + to_string(index);
            if (staticMap.find(x) == staticMap.end()) staticMap[x] = (++staticCnt);
            int idx = staticMap[x];
            file << "@" << idx <<endl;
            file << "D=M" << endl;
        }
        else if(segment == "constant") {
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
    void writePop(string segment, int index, string f="") {
        if (segment == "static") {
            string x = f + to_string(index);
            if (staticMap.find(x) == staticMap.end()) staticMap[x] = (++staticCnt);
            int idx = staticMap[x];
            file << "@" << idx <<endl;
            file << "D=A" << endl;
        }
        else {
            file << "@" << index << endl;
            file << "D=A" << endl;
            file << "@" << segmentMap[segment] << endl;
            if (segment == "temp" || segment == "pointer") file << "D=D+A" << endl;
            else file << "D=D+M" << endl;
        }
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
    void writeLabel(string label) {
        file << "(" << label << ")" << endl;
    }
    void writeGoto(string label) {
        file << "@" << label << endl;
        file << "0;JMP" << endl;
    }
    void writeIf(string label) {
        file << "@SP" << endl;
        file << "M=M-1" << endl;
        file << "A=M" << endl;
        file << "D=M" << endl;
        file << "@" << label << endl;
        file << "D;JNE" << endl;
    }
    void writeCall(string fname, int nArgs) {
        file << "@return" << labelCount << endl;
        file << "D=A" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
        file << "@LCL" << endl;
        file << "D=M" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
        file << "@ARG" << endl;
        file << "D=M" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
        file << "@THIS" << endl;
        file << "D=M" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
        file << "@THAT" << endl;
        file << "D=M" << endl;
        file << "@SP" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "M=M+1" << endl;
        file << "@SP" << endl;
        file << "D=M" << endl;
        file << "@" << nArgs + 5 << endl;
        file << "D=D-A" << endl;
        file << "@ARG" << endl;
        file << "M=D" << endl;
        file << "@SP" << endl;
        file << "D=M" << endl;
        file << "@LCL" << endl;
        file << "M=D" << endl;
        file << "@" << fname << endl;
        file << "0;JMP" << endl;
        file << "(return" << labelCount << ")" << endl;
        labelCount++;
    }
    void writeFunction(string fname, int nLocals) {
        file << "(" << fname << ")" << endl;
        for(int i=0;i<nLocals;i++) {
            writePush("constant", 0);
        }
    }
    void writeReturn() {
        file << "@LCL" << endl;
        file << "D=M" << endl;
        file << "@5" << endl;
        file << "D=D-A" << endl;
        file << "A=D" << endl;
        file << "D=M" << endl;
        file << "@R13" << endl;
        file << "M=D" << endl;

        file << "@SP" << endl;
        file << "A=M-1" << endl;
        file << "D=M" << endl;
        file << "@ARG" << endl;
        file << "A=M" << endl;
        file << "M=D" << endl;
        file << "D=A+1" << endl;
        file << "@SP" << endl;
        file << "M=D" << endl;
        file << "@LCL" << endl;
        file << "M=M-1" << endl;
        file << "A=M" << endl;
        file << "D=M" << endl;
        file << "@THAT" << endl;
        file << "M=D" << endl;
        file << "@LCL" << endl;
        file << "M=M-1" << endl;
        file << "A=M" << endl;
        file << "D=M" << endl;
        file << "@THIS" << endl;
        file << "M=D" << endl;
        file << "@LCL" << endl;
        file << "M=M-1" << endl;
        file << "A=M" << endl;
        file << "D=M" << endl;
        file << "@ARG" << endl;
        file << "M=D" << endl;
        file << "@LCL" << endl;
        file << "M=M-1" << endl;
        file << "A=M" << endl;
        file << "D=M" << endl;
        file << "@LCL" << endl;
        file << "M=D" << endl;
        file << "@R13" << endl;
        file << "A=M" << endl;
        file << "0;JMP" << endl;
    }
    ~CodeWriter() {
        file.close();
    }
};
int main(int argc, char** argv){
    if(argc<2) {
        cout << "Usage: ./VMtranslator <ProgramName> <filename(s)>" << endl;
        return 1;
    }
    string outfile = argv[1];
    outfile += ".asm";
    CodeWriter code(outfile);
    for(int i=2;i<argc;i++) {
        string file=argv[i];
        Parser* parser = new Parser(file);
        if(!parser->fileExists()) {
            cout << "File " << file << " does not exist" << endl;
            continue;
        }
        while(parser->hasMoreCommands()) {
            parser->advance();
            switch (parser->commandType())
            {
            case C_ARITHMETIC:
                code.writeArithmetic(parser->arg1());
                break;
            case C_PUSH:
                code.writePush(parser->arg1(), stoi(parser->arg2()), file);
                break;
            case C_POP:
                code.writePop(parser->arg1(), stoi(parser->arg2()), file);
                break;
            case C_LABEL:
                code.writeLabel(parser->arg1());
                break;
            case C_GOTO:
                code.writeGoto(parser->arg1());
                break;
            case C_IF:
                code.writeIf(parser->arg1());
                break;
            case C_FUNCTION:
                code.writeFunction(parser->arg1(), stoi(parser->arg2()));
                break;
            case C_RETURN:
                code.writeReturn();
                break;
            case C_CALL:
                code.writeCall(parser->arg1(), stoi(parser->arg2()));
                break;
            default:
                break;
            }
        }
    }
    return 0;
}