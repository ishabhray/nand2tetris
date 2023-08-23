#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>

using namespace std;

class Parser {
private:
    string currInstruction;
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
            getline(file, currInstruction);
            int x = currInstruction.find("//");
            if(x != -1) currInstruction.erase(x);
            currInstruction.erase(remove(currInstruction.begin(), currInstruction.end(), ' '), currInstruction.end());
            currInstruction.erase(remove(currInstruction.begin(), currInstruction.end(), '\n'), currInstruction.end());
            currInstruction.erase(remove(currInstruction.begin(), currInstruction.end(), '\r'), currInstruction.end());
        }while(hasMoreCommands() && currInstruction.size() == 0);
    }
    string instructionType() {
        if(currInstruction[0] == '@') {
            return "A_COMMAND";
        }
        else if(currInstruction[0] == '(') {
            return "L_COMMAND";
        }
        else {
            return "C_COMMAND";
        }
    }
    string symbol() {
        string x = instructionType();
        if(x == "A_COMMAND") {
            return currInstruction.substr(1);
        }
        else if(x == "L_COMMAND") {
            return currInstruction.substr(1, currInstruction.length()-2);
        }
        else {
            return "";
        }
    }
    string dest() {
        int x = currInstruction.find("=");
        if (x == -1) return "null";
        return currInstruction.substr(0, x);
    }
    string comp() {
        int x = currInstruction.find("=");
        string s = "";
        for(int i = x + 1; i < currInstruction.length(); i++) {
            if(currInstruction[i] == ';')
                break;
            s += currInstruction[i];
        }
        return s;
    }
    string jump() {
        int i;
        for(i = 0; i < currInstruction.length(); i++) {
            if(currInstruction[i] == ';')
                break;
        }
        i++;
        if(i < currInstruction.length()) return currInstruction.substr(i);
        else return "null";
    }
    ~Parser() {
        file.close();
    }
};

class SymbolTable {
private:
    map<string, int> table;
    int curr;
public:
    SymbolTable(){
        curr = 16;
        table["SP"] = 0;
        table["LCL"] = 1;
        table["ARG"] = 2;
        table["THIS"] = 3;
        table["THAT"] = 4;
        table["R0"] = 0;
        table["R1"] = 1;
        table["R2"] = 2;
        table["R3"] = 3;
        table["R4"] = 4;
        table["R5"] = 5;
        table["R6"] = 6;
        table["R7"] = 7;
        table["R8"] = 8;
        table["R9"] = 9;
        table["R10"] = 10;
        table["R11"] = 11;
        table["R12"] = 12;
        table["R13"] = 13;
        table["R14"] = 14;
        table["R15"] = 15;
        table["SCREEN"] = 16384;
        table["KBD"] = 24576;
        curr = 16;
    }
    int get(string symbol) {
        if(table.find(symbol) == table.end()) {
            table[symbol] = curr++;
        }
        return table[symbol];
    }
    void add(string symbol, int address) {
        table[symbol] = address;
    }
};

namespace Code {
    string dest(string mnemonic) {
        if(mnemonic == "null") return "000";
        else if(mnemonic == "M") return "001";
        else if(mnemonic == "D") return "010";
        else if(mnemonic == "MD") return "011";
        else if(mnemonic == "A") return "100";
        else if(mnemonic == "AM") return "101";
        else if(mnemonic == "AD") return "110";
        else if(mnemonic == "AMD") return "111";
        else return "000";
    }
    string comp(string mnemonic) {
        if(mnemonic == "0") return "0101010";
        else if(mnemonic == "1") return "0111111";
        else if(mnemonic == "-1") return "0111010";
        else if(mnemonic == "D") return "0001100";
        else if(mnemonic == "A") return "0110000";
        else if(mnemonic == "!D") return "0001101";
        else if(mnemonic == "!A") return "0110001";
        else if(mnemonic == "-D") return "0001111";
        else if(mnemonic == "-A") return "0110011";
        else if(mnemonic == "D+1") return "0011111";
        else if(mnemonic == "A+1") return "0110111";
        else if(mnemonic == "D-1") return "0001110";
        else if(mnemonic == "A-1") return "0110010";
        else if(mnemonic == "D+A") return "0000010";
        else if(mnemonic == "D-A") return "0010011";
        else if(mnemonic == "A-D") return "0000111";
        else if(mnemonic == "D&A") return "0000000";
        else if(mnemonic == "D|A") return "0010101";
        else if(mnemonic == "M") return "1110000";
        else if(mnemonic == "!M") return "1110001";
        else if(mnemonic == "-M") return "1110011";
        else if(mnemonic == "M+1") return "1110111";
        else if(mnemonic == "M-1") return "1110010";
        else if(mnemonic == "D+M") return "1000010";
        else if(mnemonic == "D-M") return "1010011";
        else if(mnemonic == "M-D") return "1000111";
        else if(mnemonic == "D&M") return "1000000";
        else if(mnemonic == "D|M") return "1010101";
        else return "0000000";
    }
    string jump(string mnemonic) {
        if(mnemonic == "null") return "000";
        else if(mnemonic == "JGT") return "001";
        else if(mnemonic == "JEQ") return "010";
        else if(mnemonic == "JGE") return "011";
        else if(mnemonic == "JLT") return "100";
        else if(mnemonic == "JNE") return "101";
        else if(mnemonic == "JLE") return "110";
        else if(mnemonic == "JMP") return "111";
        else return "000";
    }
};
int main(int argc, char** argv){
    if(argc<2) {
        cout << "Usage: ./assembler <filename(s)>" << endl;
        return 1;
    }
    for(int i=1;i<argc;i++) {
        string file=argv[i];
        Parser* parser = new Parser(file);
        SymbolTable table;
        int curr = 0;
        if(!parser->fileExists()) {
            cout << "File " << file << " does not exist" << endl;
            continue;
        }
        while(parser->hasMoreCommands()) {
            parser->advance();
            if(parser->instructionType() == "L_COMMAND") {
                table.add(parser->symbol(), curr);
            }
            else {
                curr++;
            }
        }
        parser = new Parser(file);
        string outfile = file.find_last_of(".") != string::npos ? file.substr(0, file.find_last_of(".")) + "1.hack" : file + "1.hack";
        ofstream fout(outfile);
        while(parser->hasMoreCommands()) {
            parser->advance();
            if(parser->instructionType() == "C_COMMAND")  {
                fout << "111";
                fout << Code::comp (parser->comp());
                fout << Code::dest (parser->dest());
                fout << Code::jump (parser->jump()) << endl;
                curr++;
            }
            else if (parser->instructionType() == "A_COMMAND") {
                int x;
                try{
                    x = stoi(parser->symbol());
                }
                catch(...){
                    x = table.get(parser->symbol());
                }
                string s = "";
                while(x) {
                    s += to_string(x%2);
                    x /= 2;
                }
                while(s.length() < 15) s += "0";
                reverse(s.begin(), s.end());
                fout << "0" << s << endl;
                curr++;
            }
        }
        fout.close();
    }
    return 0;
}