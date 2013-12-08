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
    string salt;
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
    int dropped = 0;
    while (getline(file, line))
    {
        if (line[line.size() - 1] == '\r')
            line.resize(line.size() - 1);
        string cropped = line.substr(0, NUMBER_OF_CHARS_CRYPT);
        if (cropped != lastAcceptedWord)
        {
            lastAcceptedWord = cropped;
            result.push_back(cropped);
        }
        else dropped++;
    }
    file.close();
    printf("Dropped %d words, because first 8 chars identical\n", dropped);
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
        p->salt = password.substr(0, 2);
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

bool cryptAndTestR(string inFile, string salt, string plainText, crypt_data *data)
{
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

    if (cryptAndTestR(p.password, p.salt, word, data))
    {
        return word;
    }
    if (word.length() < NUMBER_OF_CHARS_CRYPT)
    {
        for (int j=0; j<10; j++)
        {
            data->initialized = 0;
            string wordJ = word + to_string(j);
            if (cryptAndTestR(p.password, p.salt, wordJ, data))
            {
                return wordJ;
            }
        }
    }
    return "";
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
    /*
    #pragma omp parallel
    {
        string plaintext;
        bool found;
        struct crypt_data data;

        #pragma omp for
        for (unsigned int i=0; i<toCrack.size(); i++)
        {
            plaintext = "";
            found = false;
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
    }*/

        struct crypt_data data;
    #pragma omp parallel for private(data)
    for(unsigned int i = 0; i < toCrack.size(); i++)
    {
        /* start cracking of passwords */
        string username, userSalt, userPassword, dictWord, encryptedPw, testPassword;
        // double time_per_salt;
        // store result data and book keeping information from crypt_r()
        username = toCrack.at(i).user;
        userPassword = toCrack.at(i).password;
        userSalt = toCrack.at(i).salt;
        data.initialized = 0;
        for (unsigned int j = 0; j < dict.size(); j++)
        {

            dictWord = dict.at(j);

            //time_per_salt = now();

            if (dictWord.size() < NUMBER_OF_CHARS_CRYPT)
            {
                // combine dictionary word with numbers from 0 to 9 respectively
                for(int i = 0; i < 10; i++)
                {
                    testPassword = dictWord + to_string(i);
                    encryptedPw = (string)crypt_r(testPassword.c_str(), userSalt.c_str(), &data); // do actual encryption
                    //cout << "userSalt: " << userSalt << ", dictWord("<< testPassword.size() <<"): " << testPassword << ", encryptedPw: " << encryptedPw << endl;

                    // check if we found the user's password
                    if (userPassword.compare(encryptedPw) == 0)
                    {
                        #pragma omp critical
                        {
                            cout << "Cracked User-Password: " << username << ";" << testPassword << endl;
                        }
                    }
                }
            }

            // keep word as it is and check
            encryptedPw = (string)crypt_r(dictWord.c_str(), userSalt.c_str(), &data);
            //cout << "userSalt: " << userSalt << ", dictWord("<< dictWord.size() <<"): " << dictWord << ", encryptedPw: " << encryptedPw << ", num: " << omp_get_thread_num() << endl;

            if (userPassword.compare(encryptedPw) == 0)
            {
                #pragma omp critical
                {
                    cout << "Cracked User-Password: " << username << ";" << dictWord << endl;
                }
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
