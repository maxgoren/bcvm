#ifndef buffer_hpp
#define buffer_hpp
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

class CharBuffer {
    public:
        virtual ~CharBuffer() {

        } 
        virtual void advance() = 0;
        virtual bool done() = 0;
        virtual char get() = 0;
        virtual int markStart() = 0;
        virtual string sliceFromStart(int matchlen) = 0;
        virtual int lineNo() = 0;
};


class StringBuffer : public CharBuffer {
    private:
        string buff;
        int spos;
        int start;
        int ln;
    public:
        StringBuffer() {
            ln = 0;
        }
        ~StringBuffer() {

        }
        void init(string str) {
            buff = str;
            spos = 0;
        }
        int markStart() {
            start = spos;
            return start;
        }
        string sliceFromStart(int matchlen) {
            string slice;
            for (int i = start; i <= start+matchlen; i++)
                if (buff[i] != '\"' && buff[i-1] != '\\')
                    slice.push_back(buff[i]);
            return slice;
        }
        char get() {
            if (buff[spos] == '\n') ln++;
            return buff[spos];
        }
        bool done() {
            return spos >= buff.size();
        }
        void advance() {
            spos++;
        }
        int lineNo() {
            return ln;
        }
};


class FileStringBuffer : public CharBuffer {
    private:
        vector<string> lines;
        int line_pos;
        int str_pos;
        int start;
        int line_start;
        void read(std::string fname) {
            ifstream gfile(fname, ios::in);
            if (!gfile.is_open()) {
                cout<<"Couldnt open "<<fname<<endl;
                return;
            }
            string buff;
            while (gfile.good()) {
                getline(gfile, buff);
                lines.push_back(buff);
            }
            line_pos = 0;
            str_pos = 0;
        }
    public:
        FileStringBuffer() {
            line_pos = -1;
            str_pos = -1;
            start = -1;
            line_start = -1;
        }
        ~FileStringBuffer() {

        }
        int markStart() {
            start = str_pos;
            line_start = line_pos;
            return start;
        }
        string sliceFromStart(int matchlen) {
            string slice;
            int j = 0, t = 0;
            for (int k = line_start; k <= line_pos && j <= matchlen; k++) {
                for (int n = t > 0 ? 0:start; n < lines[line_pos].length() && j <= matchlen; n++, j++) {
                    slice.push_back(lines[k][n]);
                }
                t++;
            }
            return slice;
        }
        void advance() {
            if (line_pos != -1 && line_pos < lines.size()) {
                if (str_pos < lines[line_pos].size()) {
                    str_pos++;
                } else {
                    line_pos++;
                    str_pos = 0;
                }
            }
        }
        void rewind() {
            if (done()) {
                line_pos = lines.size()-1;
                str_pos = lines[line_pos].length()-1;
            } else {
                if (str_pos > 0) {
                    str_pos--;
                } else {
                    line_pos--;
                    str_pos = lines[line_pos].length()-1;
                }
            }
        }
        char get() {
            if (line_pos == -1)
                return 0;
            return lines[line_pos][str_pos];
        }
        bool done() {
            if (line_pos == -1)
                return true;
            return line_pos == lines.size()-1 && str_pos == lines[line_pos].size();
        }
        void readFile(string fname) {
            read(fname);
        }
        int lineNo() {
            return line_pos;
        }
};

#endif
