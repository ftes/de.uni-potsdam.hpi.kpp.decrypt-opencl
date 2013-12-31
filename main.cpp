/*
crypt only looks at first 8 characters of plaintext -> drop the rest
use threadsafe crypt_r instead of crypt
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>
#include <vector>

#include "cl.hpp"
#include "opencl.hpp"
#include "timing.hpp"

using namespace std;

const int passwordLen = 8;
const int mutations = 11;
const int cryptedLen = 13;

struct Password
{
    string user;
    string salt;
    string password;
};

vector<string> dict;
vector<Password> toCrack;
vector<Password> cracked;

string outFile = "output.txt";

vector<string> parseDict(string fileName)
{
    ifstream file(fileName.c_str());
    vector <string> result;
    string line;
    //only look at first 8 chars of each word
    string lastAcceptedWord = "";
    int dropped = 0;
    while (getline(file, line))
    {
        if (line[line.size() - 1] == '\r')
            line.resize(line.size() - 1);
        string cropped = line.substr(0, passwordLen);
        if (cropped != lastAcceptedWord)
        {
            lastAcceptedWord = cropped;
            result.push_back(cropped);
        }
        else dropped++;
    }
    file.close();

#ifdef DEBUG
    printf("Dropped %d words, because first 8 chars identical\n", dropped);
#endif

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




int main(int argc, char* argv[])
{
#ifdef DEBUG
    timeval start = startTiming();
#endif // DEBUG

    string pwFile = string(argv[1]);
    string dictFile = string(argv[2]);

    dict = parseDict(dictFile);
    toCrack = parsePasswords(pwFile);

    int lp1 = passwordLen + 1;

    char dictC[lp1 * dict.size()];
    for (int i=0; i<dict.size(); i++)
        dict[i].copy(dictC + lp1 * i, passwordLen);

    cl::Device device = findFirstDeviceOfType(CL_DEVICE_TYPE_CPU);
    cl::Context context = getContext(device);
    cl::Program program = loadProgram(device, context, true);
    cl::CommandQueue queue(context, device);

    cl::Buffer dictB = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(char)* lp1 * dict.size(), dictC);
    cl::Buffer cryptedB = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(char) * (cryptedLen + 1));
    cl::Buffer resultB = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(char) * lp1);

    auto kernel = cl::make_kernel<cl::Buffer, cl::Buffer, cl::Buffer>(program, "crypt_multiple");
    //ranges: global offset, global (global number of work items), local (number of work items per work group)
    cl::EnqueueArgs eargs(queue, cl::NullRange, cl::NDRange(mutations * dict.size()), cl::NullRange);

#ifdef DEBUG
    timeval startKernel = startTiming();
#endif
    for (Password p : toCrack)
    {
        char resultC[passwordLen + 1] = "";
        char cryptedC[cryptedLen + 1];
        p.password.copy(cryptedC, cryptedLen);

        queue.enqueueWriteBuffer(cryptedB, CL_TRUE, 0, sizeof(char) * (cryptedLen + 1), cryptedC);
        queue.enqueueWriteBuffer(resultB, CL_TRUE, 0, sizeof(char) * lp1, resultC);
        kernel(eargs, cryptedB, dictB, resultB).wait();
        queue.enqueueReadBuffer(resultB, CL_TRUE, 0, sizeof(char) * lp1, resultC);

        if (strlen(resultC) > 0)
        {
            string plaintext = resultC;
#ifdef DEBUG
            printf("Found password for %s: %s\n", p.user.c_str(), resultC);
#endif // DEBUG

            Password newP;
            newP.user = p.user;
            newP.password = plaintext;
            cracked.push_back(newP);
        }
    }
#ifdef DEBUG
    outputElapsedSec("Kernel", startKernel);
#endif

    writeOutput();

#ifdef DEBUG
    outputElapsedSec("Overall", start);
#endif

    exit(0);
}
