#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstdio>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>
using namespace std;

void executeProgram(vector<string> commands);

vector<string> getCommands() { //Read commands by "words"
    string commands;
    getline(cin, commands); //Read line
    if (!cin) { // Proceeding Ctr+D
        commands = "exit";
        printf("\n");
    }
    vector<string> commandWords;
    int flag = -1;
    int textFlag = 0; //Is there any " "
    int size = commands.size();
    for (int i = 0; i < size ; i++){ //Searching for " or spaces
        if (commands[i] == ' ' and flag != -1 and textFlag == 0) {
            string subCommand = commands.substr(flag, i-flag);
            commandWords.push_back(subCommand);
            flag = -1;
        }
        else if (commands[i] == '"' and textFlag == 0) {
            textFlag = 1;
            if (flag == -1) {
                flag = i;
            }
        }
        else if (commands[i] == '"' and textFlag == 1){
            textFlag = 0;
        }
        else if (commands[i] != ' ' and flag == -1 and textFlag == 0) {
            flag = i;
        }

    }
    if (flag != -1) {
        string subCommand = commands.substr(flag, size-flag);
        commandWords.push_back(subCommand);
    }
    return commandWords;
}

bool checkMaskEquality(string word, string mask) { //contain error with only one "
    //cout << word << endl;
    //cout << mask << endl;
    vector<string> subWords; //Words among *
    int textFlag = 0; //is there "
    int textPresFlag = 0; //was there "
    int posFlag = 0; //position of word begining
    int starPresenceFlag = 0; //for reading the sequence of *
    int endStarFlag = 0; //is there star at the end
    for (int i = 0; i < mask.size(); i++) {
        if (mask[i] != '*' and starPresenceFlag == 1) {
            starPresenceFlag = 0;
            posFlag = i;
        }
        if (mask[i] == '"' and textFlag == 0 and starPresenceFlag == 0) {
            textFlag = 1;
            textPresFlag = 1;
        }
        else if (mask[i] == '"' and textFlag == 1 and starPresenceFlag == 0) {
            textFlag = 0;
        }
        else if (mask[i] == '*' and textFlag == 0 and starPresenceFlag == 0) {
            string subword = mask.substr(posFlag, i - posFlag);
            subWords.push_back(subword);
            starPresenceFlag = 1;
        }
    }
    if (starPresenceFlag == 1) {
        endStarFlag = 1;
    }
    else {
        string subword = mask.substr(posFlag, mask.size() - posFlag);
        subWords.push_back(subword);
    }
    int numOfSubword = 0;
    int posInWord = 0;
    int subWordCount = subWords.size();
    if (endStarFlag == 1 and subWordCount == 1 and subWords[0].empty()) { // * word
        return true;
    }
    else {
        if (subWords[0].empty() == false or mask[0] != '*'){ //parsing begining
            textFlag = 0;
            for (int i = 0; i < subWords[0].size(); i++){
                if (subWords[numOfSubword][i] == '"') {
                    if (textFlag == 0) {
                        textFlag = 1;
                    }
                    else {
                        textFlag = 0;
                    }
                    continue;
                }
                if (posInWord >= word.size()) {
                    return false;
                }
                if (subWords[numOfSubword][i] !=  word[posInWord] and subWords[numOfSubword][i] != '?' and textFlag == 0) {
                    return false;
                }
                else if (subWords[numOfSubword][i] !=  word[posInWord] and textFlag == 1) {
                    return false;
                }
                posInWord++;
            }
        }
        if (endStarFlag == 1 and subWordCount == 1) { //aa* word
            return true;
        }
        numOfSubword++;
        int iterations; //number of middle subwords
        iterations = subWordCount - 1;
        if (endStarFlag == 0) {
            iterations--;
        }
        if (iterations == -1) { //word without *
            if (posInWord != word.size()) {
                return false;
            }
            else {
                return true;
            }
        }
        for (int i = 0; i < iterations; i++) {// parsing middle subwords
            int midFlag = 0;
            int varPosInWord = posInWord;
            while(midFlag == 0) {
                midFlag = 1;
                textFlag = 0;
                varPosInWord = posInWord;
                for(int j = 0; j < subWords[numOfSubword].size(); j++) {
                    if (subWords[numOfSubword][j] == '"') {
                        if (textFlag == 0) {
                            textFlag = 1;
                        }
                        else {
                            textFlag = 0;
                        }
                        continue;
                    }
                    if (varPosInWord >= word.size()) {
                        return false;
                    }
                    if (subWords[numOfSubword][j] !=  word[varPosInWord] and subWords[numOfSubword][j] != '?' and textFlag == 0) {
                        midFlag = 0;
                        break;
                    }
                    else if (subWords[numOfSubword][j] !=  word[varPosInWord] and textFlag == 1) {
                        midFlag = 0;
                        break;
                    }
                    varPosInWord++;      
                }
                posInWord++;
            }
            numOfSubword++;
        }
        if (endStarFlag == 0 and subWordCount > 1) { //parsing ending
            posInWord = word.size() - subWords[subWordCount - 1].size();
            textFlag = 0;
            for (int i = 0; i < subWords[subWordCount - 1].size(); i++){
                if (subWords[subWordCount - 1][i] == '"') {
                    if (textFlag == 0) {
                        textFlag = 1;
                    }
                    else {
                        textFlag = 0;
                    }
                    posInWord++;
                    continue;
                }
                if (posInWord >= word.size()) {
                    return false;
                }
                if (subWords[subWordCount - 1][i] !=  word[posInWord] and subWords[subWordCount - 1][i] != '?' and textFlag == 0) {
                    return false;
                }
                else if (subWords[subWordCount - 1][i] !=  word[posInWord] and textFlag == 1) {
                    return false;
                }
                posInWord++;
            }
        }
        else if (endStarFlag == 1 and subWordCount > 1) {
            return true;
        }
        if (posInWord != word.size()) {
            return false;
        }
        return true;
    }

}

vector<string> dirPaths(string dir, string mask) { //returns all files in dir compared with mask
    vector<string> paths;
    char *path = (char *)dir.c_str(); 
    struct stat entryInfo;
    if (stat(path, &entryInfo) != 0) {
        return paths;
    }
    if (S_ISDIR(entryInfo.st_mode)) {
        DIR *d = opendir(path); //may contain error /path/dir/, not /path/dir
        if (d != NULL) {
            struct dirent *entry = readdir(d);
            while (entry != NULL) {
                if ((entry->d_name)[0] == '.'){
                    entry = readdir(d);
                    continue;
                }
                if (checkMaskEquality(string(entry->d_name), mask)) {
                    paths.push_back(dir + string(entry->d_name));
                }
                entry = readdir(d);
            }
            closedir(d);
        }
    }
    return paths; //Maybe sort them???
}

vector<string> openWordMask(string word){ //Returns vector of unmasked words
    int size = word.size();
    vector<string> unmaskedWords;
    int genPath = 1;
    string curDir = "/";
    if (word[0] != '/') {
        genPath = 0;
        curDir = string(get_current_dir_name()) + '/';
    }
    //cout << curDir << endl;
    unmaskedWords.push_back(word);
    int textFlag = 0; //" presence
    int slashFlag = 0; // / presence
    int maskFlag = 0; //mask presence
    string curMask = ""; //current Mask in processing
    string afterMask = ""; //for adding in recursion
    for (int i = genPath; i < size ; i++) {
        if (word[i] != '/') {
            curMask = curMask + word[i];
        }
        else {
            for (int j = 0; j < curMask.size(); j++) {
                if (curMask[j] == '"') {
                    if (textFlag == 0) {
                        textFlag = 1;
                    }
                    else {
                        textFlag = 0;
                    }
                }
                if ((curMask[j] == '*' or curMask[j] == '?') and textFlag == 0) {
                    maskFlag = 1;
                    break;
                }
            }
            if (maskFlag == 1){
                vector<string> subMasks = dirPaths(curDir,curMask);
                if (subMasks.size() == 0) {
                    return subMasks;
                }
                vector<string> results;
                for (int k = 0; k < subMasks.size(); k++) {
                    vector<string> subRes = openWordMask(subMasks[k] + word.substr(i, word.size() - i));
                    for (int l = 0; l < subRes.size(); l++) {
                        results.push_back(subRes[l]);
                    }
                }
                return results;
            }
            else {
                curDir = curDir + curMask + '/';
                curMask = "";
            }
        }
    }
    for (int j = 0; j < curMask.size(); j++) {
        if (curMask[j] == '"') {
            if (textFlag == 0) {
                textFlag = 1;
            }
            else {
                textFlag = 0;
            }
        }
        if ((curMask[j] == '*' or curMask[j] == '?') and textFlag == 0) {
            maskFlag = 1;
            break;
        }
    }
    if (maskFlag == 1){
        vector<string> subMasks = dirPaths(curDir,curMask);
        if (subMasks.size() == 0) {
            return subMasks;
        }
        vector<string> results;
        for (int k = 0; k < subMasks.size(); k++) {
            vector<string> subRes = openWordMask(subMasks[k]);
            for (int l = 0; l < subRes.size(); l++) {
                results.push_back(subRes[l]);
            }
        }
        return results;

    }
    else {
        vector<string> subMasks;
        subMasks.push_back(curDir + curMask);
        return subMasks;
        
    }
}

int openMasks(vector<string> * commands) {
    int retValue = 0; //no mask failure if 0
    vector<string> help;
    int size = (*commands).size();
    for (int i = 0; i < size; i++) { //putting words to help vector
        help.push_back((*commands)[i]);
    }
    (*commands).clear();
    for (int i = 0; i < size; i++) { //putting unmasked words to commands
        int wordSize = help[i].size();
        int textFlag = 0; // is there any "
        int maskFlag = 0; // is there any masks
        for (int j = 0; j < wordSize; j++){
            if (help[i][j] == '"' and textFlag == 0) {
                textFlag = 1;
            }
            else if (help[i][j] == '"' and textFlag == 1) {
                textFlag = 0;
            }
            else if ((help[i][j] == '?' or help[i][j] == '*') and textFlag == 0) {
                maskFlag = 1;
                break;
            }
        }
        if (maskFlag == 0) { //No masks in word
            (*commands).push_back(help[i]);
        }
        else { //Masks in word
            vector<string> openedMasks = openWordMask(help[i]);
            for (int j = 0; j < openedMasks.size(); j++) {
                (*commands).push_back(openedMasks[j]);
            }
            if (openedMasks.size() == 0) {
                retValue = 1;
            }
        }
    }
    return retValue;
}

vector<vector<string>> convParse(vector<string> *commands) { //Parsing for conveyor usage
    vector<vector<string>> parsedCommands;
    vector<string> convElem; //Element of conveyor
    bool isConveyor = true;
    for (int i = 0; i < (*commands).size(); i++) {
        if ((*commands)[i] == "|") {
            parsedCommands.push_back(convElem);
            convElem.clear();
        }
        else {
            convElem.push_back((*commands)[i]);
        }
        if ((*commands)[i] == "<" or (*commands)[i] == ">") {
            isConveyor = false;
        }
    }
    parsedCommands.push_back(convElem);
    convElem.clear();
    if (isConveyor == false and parsedCommands.size() > 1) {
        printf("Conveyor error: presence of '<' or '>'\n");
        parsedCommands.clear();
    }
    return parsedCommands;
}

void executeCd(vector<string> *commands, int comNumFlag) { //Start cd
    if ((*commands).size() > 2 ) {
        printf("Cd error: too many arguments\n");
        return;
    }
    if ((*commands).size() == 1 and comNumFlag == 1) {
        printf("Cd error: no file that equals the mask\n");
        return;
    }
    if ((*commands).size() == 1) {
        (*commands).push_back(getenv("HOME")); //cd to home directory
    }
    string path = (*commands)[1];
    if (path[0] != '/') {
        path = string(getenv("HOME")) + '/' + path; 
    }
    struct stat s;
    if (stat((char *)path.c_str(), &s) != 0) {
        printf("Cd error: not enough rights to access or not directory\n");
        return;
    }
    if (S_ISDIR(s.st_mode)) {
        chdir((char *)path.c_str());
    }
    else {
        printf("Cd error: it's not a dirrectory\n");
    }
}

void executeTime(vector<string> commands) {
    vector<string> realCommands; //commands without time
    for (int i = 1; i < commands.size(); i++) {
        realCommands.push_back(commands[i]);
    }
    struct timeval startTime;
    struct timeval endTime;
    gettimeofday(&startTime, NULL);
    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Fork error");
            break;
        case 0: //child process runs program
            executeProgram(realCommands);
            exit(EXIT_SUCCESS);
        default: //parrent process is waiting and counting time
            waitpid(pid, NULL, 0);
            gettimeofday(&endTime, NULL);
            struct rusage usage;
            getrusage(RUSAGE_CHILDREN, &usage);
            //Mb some problems here???
            printf("Walltime: %f ; Systemtime: %f ; Usertime: %f\n", (float)(endTime.tv_usec - startTime.tv_usec) / 1000000 , (float)usage.ru_stime.tv_usec / 1000000, (float)usage.ru_utime.tv_usec / 1000000 );
    }
}

void executeEcho(vector<string> commands) { //executing echo command
    string varName;
    string varRes;
    for(int i = 1; i < commands.size(); i++){
        if (commands[i][0] == '$') { // if command is environment variable
            varName = commands[i].substr(1);
            varRes = getenv(varName.c_str());
            cout << varRes + ' ';
        }
        else { // usuall argument
            cout << commands[i] + ' ';
        }
    }
    printf("\n");
}

void executeSet(vector<string> commands) { //idea didn't work... left it here
    printf("Don't use set in conveyor, it's useless!!!\n");
}

void executeProgram(vector<string> commands) {
    if (commands.empty() == true) {
        printf("exec error: trying to exec empty command line\n");
        return;
    }
    if (commands[0] == "pwd") {
        printf("%s\n", get_current_dir_name());
        return;
    }
    if (commands[0] == "time") {
        executeTime(commands);
        return;
    }
    if (commands[0] == "set") {
        executeSet(commands);
        return;
    }
    if (commands[0] == "echo") {
        executeEcho(commands);
        return;
    }
    char *adrprog = (char *)(commands[0]).c_str();// if exec usuall program
    char *shellprog = (char *)("/bin/" + commands[0]).c_str(); // if exec shell command
    vector<char *> execCommands;
    for(int i = 0; i < commands.size(); i++) {
        execCommands.push_back((char *)commands[i].c_str());
    }
    execCommands.push_back(NULL); //special for execv
    for (int i = 0 ; i < execCommands.size() - 1 ; i++) {
        cout << commands[i] << endl;
    }
    execv(adrprog, &execCommands[0]); //exec usuall program
    execv(shellprog, &execCommands[0]); //exec shell program otherwise
    printf("exec error: prog does not execute\n");

}

int openInputFile(vector<string> commands) {
    int i = 0;
    int input = -1;
    while(i < commands.size() and commands[i] != "<") {
        i++;
    }
    if (i == commands.size()) {
        return -2;
    }
    else {
        if (i + 1 == commands.size()) {
            printf("turning error: no arguments after <\n");
        }
        else {
            input = open((char *)commands[i+1].c_str(), O_RDONLY);
            if (input == -1) {
                perror("Open input error");
            }
        }
        return input;
    }
    
}

int openOutputFile(vector<string> commands) {
    int i = 0;
    int output = -1;
    while(i < commands.size() and commands[i] != ">") {
        i++;
    }
    if (i == commands.size()) {
        return -2;
    }
    else {
        if (i + 1 == commands.size()) {
            printf("turning error: no arguments after >\n");
        }
        else {
            output = open((char *)commands[i+1].c_str(), O_WRONLY | O_CREAT);
            if (output == -1) {
                perror("Open output error");
            }
        }
        return output;
    }
}

void executeStream(vector<string> commands, int comNumFlag) { //Not conveyor execution, works good only with one < or >
    if (commands[0] == "cd") {
        executeCd(&commands, comNumFlag);
        return;
    }
    vector<string> subProgCommands; //for redirected input/output execution
    int input = openInputFile(commands); //From where we get input
    int output = openOutputFile(commands); //Where to turn output
    if (input == -1 or output == -1) {
        close(input);
        close(output);
        return;
    }
    pid_t pid = fork();
    int i = 0;
    switch (pid) {
        case -1:
            perror("Fork error");
            close(input);
            close(output);
            break;
        case 0: //child does programm with changed input/output
            while (i < commands.size() and commands[i] != "<" and commands[i] != ">") {
                subProgCommands.push_back(commands[i]);
                i++;
            }
            if (input > 0) {
                dup2(input, 0);
            }
            if (output > 0) {
                dup2(output, 1);
            }
            executeProgram(subProgCommands);
            exit(EXIT_SUCCESS);
        default: //parrent process is waiting for child
            waitpid(pid, NULL, 0);
            close(input);
            close(output);
    }
}

void executeConveyor(vector<vector<string>> *parsedCommands) {
    int convElemNum = (*parsedCommands).size(); //number of conveyor elements
    int fd[convElemNum - 1][2]; //array of pipe descriptors
    vector<string> convElemProg; // subprograms of conveyor
    for (int i = 0 ; i < convElemNum - 1; i++) { //forking every conveyor element
        pipe(fd[i]);
        pid_t pid = fork();
        switch (pid) {
            case -1:
                perror("Fork error");
                break;
            case 0: // i-element of conveyor
                close(fd[i][0]); //closing read end of pipe (for this process)
                dup2(fd[i][1], 1); //turning stdout to the pipe
                if (i != 0) {
                    dup2(fd[i-1][0], 0); //read from previous pipe, not stdin (not for first element)
                }
                convElemProg = (*parsedCommands)[i];
                executeProgram(convElemProg); //executing subprogramm of conveyor
                exit(EXIT_SUCCESS);
            default:
                close(fd[i][1]); //closing write end of pipe (for this process)
                break;
        }
    }
    dup2(fd[convElemNum - 2][0], 0); 
    convElemProg = (*parsedCommands)[convElemNum - 1];
    executeProgram(convElemProg); // execute last conveyor element (stdout is normal)
    exit(EXIT_SUCCESS);
}

void executeConveyorPrep(vector<vector<string>> *parsedCommands) { //Conveyor to child process
    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Fork error");
            break;
        case 0: //child process is running
            executeConveyor(parsedCommands);
        default: //parent process waits for child
            waitpid(pid, NULL, 0);
    }

}


void executeCommands(vector<vector<string>> *parsedCommands, int comNumFlag) { //start commands
    int size = (*parsedCommands).size();
    if (size == 1) {
        executeStream((*parsedCommands)[0], comNumFlag);
    }
    if (size > 1) {
        executeConveyorPrep(parsedCommands);
    }
}

string shellDir(string dir) { //~dir (like in shell)
    char * homeDir = getenv("HOME");
    int lenh = strlen(homeDir);
    int lend = dir.size();
    dir = '~' + dir.substr(lenh, lend - lenh);
    return dir;
}

int main() {
    printf("Welcome to microshell! Enter \"exit\" to leave.\n");
    int flag = 0;
    char supSymb;
    string curDir;
    char * startDir = get_current_dir_name(); //Remember start dir
    chdir(getenv("HOME")); //Work from home dir
    while(flag == 0) {
        if (getuid() == 0) { // Is User Privileged? (> or !)
            supSymb = '!';
        }
        else {
            supSymb = '>';
        }
        curDir = shellDir(get_current_dir_name()); //print current dir
        cout << curDir;
        printf("%c ", supSymb);
        vector<string> commands = getCommands(); //Read commands
        int comNumFlag = 0; //Fast bugfix for cd, when nothing equals the mask
        if (commands.size() > 1) {
            comNumFlag = 1;
        }
        if (commands[0] == "set") { //set execution
            if (commands.size() > 2) {
                printf("Set error\n");
                continue;
            }
            for(int j = 0; j < commands[1].size(); j++){
                if (commands[1][j] == '=') {
                    if (j == 0 or j == (commands[1].size() - 1)) {
                        printf("Set error\n");
                        break;
                    }
                    string varName = commands[1].substr(0,j);
                    string varRes = commands[1].substr(j+1);
                    setenv(varName.c_str(), varRes.c_str(), 1);
                    break;
                }
            }
            continue;
        } 
        int maskFlag = openMasks(&commands); //Open * and ? in words
        if (maskFlag == 1) {
            printf("Some masks cannot be opened\n");
            continue;
        }
        if (commands.empty() == true) {
            continue;
        }
        if (commands[0] == "exit") {
            break;
        }
        //for (int i = 0 ; i < commands.size() ; i++) {
            //cout << commands[i] << endl;
        //}
        vector<vector<string>> parsedCommands = convParse(&commands); //Parse for conveyor usage
        if (parsedCommands.empty() == true) { //checking for conveyor error
            continue;
        }
        executeCommands(&parsedCommands, comNumFlag);
        
    }
    chdir(startDir); //Come back to start dir
    return 0;
}