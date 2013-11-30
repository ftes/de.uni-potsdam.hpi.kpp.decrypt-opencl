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

vector<string> dict;
vector<Password> toCrack;
vector<Password> cracked;

string outFile = "output.txt";

vector<string> parse(string fileName)
{
    ifstream file(fileName.c_str());
    vector <string> result;
    string line;
    while (getline(file, line))
    {
        result.push_back(line);
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
        p->password = line.substr(i+1, line.length()-1);
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
        cout << p.user << ";" << p.password << "\n";
    }

    output.close();
}

bool cryptAndTestR(string inFile, string plainText, crypt_data *data)
{
    string salt = inFile.substr(0, 2);
    string expected(crypt_r(plainText.c_str(), salt.c_str(), data));
    //printf("Salt: %s, Plain: %s, Crypt: %s\n", salt.c_str(), plainText.c_str(), expected.c_str());
    return expected == inFile;
}

bool cryptAndTest(string inFile, string plainText)
{
    string salt = inFile.substr(0, 2);
    string expected;
    #pragma omp critical(crypt)
    expected = crypt(plainText.c_str(), salt.c_str());
    //printf("Salt: %s, Plain: %s, Crypt: %s\n", salt.c_str(), plainText.c_str(), expected.c_str());
    return expected == inFile;
}

string testWordCryptR(string word, Password p, crypt_data *data)
{
    data->initialized = 0;

    if (cryptAndTestR(p.password, word, data))
    {
        return word;
    }
    for (int j=48; j<58; j++)
    {
        data->initialized = 0;
        string wordJ(word + (char) j);
        if (cryptAndTestR(p.password, wordJ, data))
        {
            return wordJ;
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
    //this is slow -> god knows why, false sharing should not be an issue as we only read arrays
    //#pragma omp parallel for schedule(dynamic)
    for (unsigned int i=0; i<toCrack.size(); i++)
    {
        Password p = toCrack[i];
        crypt_data data;
        string plaintext("");

        //#pragma omp parallel for schedule(static, 100)
        for (unsigned int j=0; j<dict.size(); j++)
        {
            string word = dict[j];
            if (! plaintext.empty()) continue;
            plaintext = testWordCryptR(word, p, &data);
            //plaintext = testWordCrypt(word, p);
            //if (plaintext != NULL) break;
        }

        if (plaintext.empty())
        {
            #pragma omp critical(console)
            printf("Password for %s could not be cracked\n", p.user.c_str());
        }
        else
        {
            Password newP;
            newP.user = p.user;
            newP.password = plaintext;
            #pragma omp critical(cracked)
            cracked.push_back(newP);
        }
    }
}

int main(int argc, char* argv[])
{
    string pwFile = string(argv[1]);
    string dictFile = string(argv[2]);

    dict = parse(dictFile);
    toCrack = parsePasswords(pwFile);

    double start = omp_get_wtime();
    crack();
    printf("Runtime: %f\n", omp_get_wtime() - start);

    writeOutput();

    exit(0);
}
