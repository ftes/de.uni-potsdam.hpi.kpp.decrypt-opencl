#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <omp.h>
#include <crypt.h>

using namespace std;

struct Password
{
    string user;
    string password;
};

const unsigned int NUMBER_OF_CHARS_CRYPT = 8;

vector<string> dict;
vector<Password> toCrack;
vector<Password> cracked;

string outFile = "output.txt";

vector<string> parseDict(string fileName)
{
    ifstream file(fileName.c_str());
    vector <string> result;
    string line;
    string lastAcceptedWord = "";
    while (getline(file, line))
    {
        if (line[line.size() - 1] == '\r')
            line.resize(line.size() - 1);
        string cropped = line.substr(0, NUMBER_OF_CHARS_CRYPT);
        if (cropped != lastAcceptedWord) {
            lastAcceptedWord = cropped;
            result.push_back(cropped);
        }
    }
    file.close();
    return result;
}

vector<Password> parsePasswords(string fileName)
{
    ifstream file(fileName.c_str());
    vector <Password> result;
    string line;
    while (getline(file, line))
    {
        Password *p = new Password;
        int i = line.find(":");
        p->user = line.substr(0, i);
        string password = line.substr(i+1, line.length()-1);
        if (password[password.size() - 1] == '\r')
            password.resize(password.size() - 1);
        p->password = password;
        result.push_back(*p);
    }
    file.close();
    return result;
}

void writeOutput()
{
    remove(outFile.c_str());
    ofstream output(outFile.c_str());

    for (unsigned int i=0; i<cracked.size(); i++)
    {
        Password p = cracked[i];
        output << p.user << ";" << p.password << "\n";
    }

    output.close();
}

bool cryptAndTestR(string inFile, string plainText, crypt_data *data)
{
    string salt = inFile.substr(0, 2);
    string expected(crypt_r(plainText.c_str(), salt.c_str(), data));
    return expected == inFile;
}

bool cryptAndTest(string inFile, string plainText)
{
    string salt = inFile.substr(0, 2);
    string expected;
    //crypt is not threadsafe, but crypt_r is very slow
    #pragma omp critical(crypt)
    expected = crypt(plainText.c_str(), salt.c_str());
    return expected == inFile;
}

string testWordCryptR(string word, Password p, crypt_data *data)
{
    data->initialized = 0;

    if (cryptAndTestR(p.password, word, data))
    {
        return word;
    }
    if (word.length() < NUMBER_OF_CHARS_CRYPT) {
        for (int j=48; j<58; j++)
        {
            data->initialized = 0;
            string wordJ(word + (char) j);
            if (cryptAndTestR(p.password, wordJ, data))
            {
                return wordJ;
            }
        }
    }
    return "";
}

string testWordCryptR(string word, Password p)
{
    testWordCryptR(word, p, new crypt_data);
}

string testWordCrypt(string word, Password p)
{
    if (cryptAndTest(p.password, word))
    {
        return word;
    }
    for (int j=48; j<58; j++)
    {
        string wordJ(word + (char) j);
        if (cryptAndTest(p.password, wordJ))
        {
            return wordJ;
        }
    }
    return "";
}

void crack()
{
    //this is really slow -> god knows why, false sharing should not be an issue as we only read the arrays but for one
    //to that we write very seldomly
    #pragma omp parallel for schedule(dynamic)
    for (unsigned int i=0; i<toCrack.size(); i++)
    {
        string plaintext = "";
        bool found = false;
        struct crypt_data data;
        data.initialized = 0;

        //this is also slower than without omp...
        //#pragma omp parallel for
        for (unsigned int j=0; j<dict.size(); j++)
        {
            if (found) continue;
            plaintext = testWordCryptR(dict[j], toCrack[i], &data);
            if (!plaintext.empty())
            {
                #pragma omp atomic write
                found = true;
                Password newP;
                newP.user = toCrack[i].user;
                newP.password = plaintext;
                #pragma omp critical(cracked)
                cracked.push_back(newP);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    string pwFile = string(argv[1]);
    string dictFile = string(argv[2]);

    dict = parseDict(dictFile);
    toCrack = parsePasswords(pwFile);

    double start = omp_get_wtime();
    crack();
    printf("Runtime: %f\n", omp_get_wtime() - start);

    writeOutput();

    exit(0);
}
