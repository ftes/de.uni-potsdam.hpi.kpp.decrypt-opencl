#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <omp.h>

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

bool cryptAndTest(string salt, string encrypted, string plainText)
{
    //cout << plainText << "\n";
    return true;
}

void crack()
{
    for (unsigned int i=0; i<toCrack.size(); i++)
    {
        Password p = toCrack[i];
        string salt = p.password.substr(0, 2);
        string encryptedPW = p.password.substr(2, p.password.size()-1);

        string plaintext("");
        for (string word : dict)
        {
            if (cryptAndTest(salt, encryptedPW, word))
            {
                plaintext = word;
                break;
            }
            for (int j=48; j<58; j++)
            {
                string wordJ = word + (char) j;
                if (cryptAndTest(salt, encryptedPW, wordJ))
                {
                    plaintext = wordJ;
                    break;
                }
            }
        }

        if (plaintext.empty())
        {
            printf("Password for %s could not be cracked\n", p.user.c_str());
        }
        else
        {
            p.password = plaintext;
            cracked.push_back(p);
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
