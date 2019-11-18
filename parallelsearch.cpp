#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <limits.h>
using namespace std;

vector<char *> filename(struct dirent *entry, int flagn, char * path);

class Kmp{ // KMP automat for reading words
    private:
        vector<char> word; //input word for building automat
        struct state {
            vector<char> symbols; //Symbols of the state
            state * arrows[128]; //Arrows to another states
            bool isFinal;
        };
        vector<struct state> states;
    public:
        Kmp(vector<char> wordin){ //Constructing automat
            word = wordin;
            size_t wordlen = word.size();
            for (int i = 0; i <= wordlen; i++){ //Constructing states
                state curstate;
                vector<char> prefixword;
                if (i != 0) {
                    for (int j = 0; j < i; j++){
                        prefixword.push_back(word[j]);
                    }
                    curstate.symbols = prefixword;
                }
                states.push_back(curstate);
            }
            for (int i = 0; i <= wordlen; i++){ //Nulling arrows and finaling bool
                for (int j = 0; j < 128; j++){
                    states[i].arrows[j] = &states[0];
                }
                states[i].isFinal = false;
            }
            states[wordlen].isFinal = true;
            for (int i = 1; i <= wordlen; i++) { // Backward arrows 
                for (int j = 0; j < 128; j++){
                    vector<char> prefix = states[i].symbols;
                    prefix.push_back(j);
                    vector<char> prefixreverse;
                    int flag[i];
                    int n = prefix.size();
                    for (int k = 0; k < n; k++){
                        prefixreverse.push_back(prefix[n-1-k]);  
                    }
                    for (int k = 0; k < i; k++){
                        prefixreverse.pop_back();
                        vector<char> prefixrevrev;
                        for (int l = 0; l < prefixreverse.size(); l++){
                            prefixrevrev.push_back(prefixreverse[prefixreverse.size() - 1 - l]);
                        }
                        int flag = 1;
                        for (int l = 0; l < prefixrevrev.size(); l++){
                            if (prefixrevrev[l]!= states[i - k].symbols[l]){
                                flag = 0;
                                break;
                            }
                        }
                        if (flag == 1) {
                            states[i].arrows[j] = &states[i - k];
                            break;
                        }
                    }
                }
            }
            for (int i = 0; i < wordlen; i++){ // Forward arrows
                char c;
                size_t length;
                length = states[i+1].symbols.size();
                c = states[i+1].symbols[length - 1];
                states[i].arrows[c] = &states[i+1];
            }

        }
        void printIfWordPresent(vector<char> line, char * path, int number){ // Finding a word in a line
            state *curState = &states[0];
            int n = line.size();
            for (int i = 0; i < n; i++){
                if (line[i] >= 0) { //escaping negative chars (many of them are in non-text files) (like ./a.out)
                    //they don't have any text meaning, so we ignore them
                    curState = curState->arrows[line[i]];
                }
                else {
                    curState = &states[0];
                }
                if(curState->isFinal == true){
                    printf("Substring was found in file: %s\n", path);
                    printf("Number of the string: %d\n", number);
                    printf("String:");
                    for (int j = 0; j < n; j++){
                        printf("%c", line[j]);
                    }
                    printf("\n");
                    break;
                }
            }
        }
};

vector<char *> findFileNames(int flagn, char * path){ //searching fo file paths
    vector<char *> files;
    DIR *dir;
    dir = opendir(path);
    if (dir != NULL) {
        struct dirent *entry;
        entry = readdir(dir);
        while (entry != NULL){
            struct stat entryInfo;
            if ((strcmp(entry->d_name, ".") == 0) or (strcmp(entry->d_name, "..") == 0)){
                entry = readdir(dir);
                continue;
            }
            char filepath[PATH_MAX];
            strncpy(filepath, path, PATH_MAX);
            strncat(filepath, "/", PATH_MAX);
            strncat(filepath, entry->d_name, PATH_MAX);
            if (lstat(filepath, &entryInfo) == 0) {
                if (S_ISREG(entryInfo.st_mode)){
                    char * copypath = (char *)malloc((strlen(filepath)+1)*sizeof(char));
                    strcpy(copypath, filepath);
                    files.push_back(copypath);
                }
                else if (S_ISDIR(entryInfo.st_mode)){
                    if (flagn == 0){
                        vector<char *> additionalfiles = findFileNames(flagn, filepath);
                        for (int i = 0; i < additionalfiles.size(); i++){
                            files.push_back(additionalfiles[i]);
                        }
                    }
                }
            }
            entry = readdir(dir);
        }
    }
    else {
        files.push_back(NULL);
    }
    closedir(dir);
    return files;
}

void findLines(vector<char *>& fileNames, mutex& m_fileNames, mutex& m_automat, Kmp& automat){ //Multithreading search
    int flag = 0;
    while (flag == 0){
        m_fileNames.lock();
        size_t size = fileNames.size();
        if (size == 0){
            m_fileNames.unlock();
            flag = 1;
        }
        else {
            char * filePath = fileNames[size - 1];
            fileNames.pop_back();
            m_fileNames.unlock();
            if (filePath == NULL) {
                free(filePath);
            }
            else {
                int fd = open(filePath, O_RDONLY);
                if (fd != -1) {
                    struct stat s;
                    int status = fstat(fd, &s);
                    int size = s.st_size;
                    if (size != 0) { //avoiding 0 size files to avoid mmap errors
                        int pagesize = getpagesize();
                        char * map;
                        map = (char *)mmap(0, pagesize, PROT_READ, MAP_PRIVATE, fd, 0);
                        vector <char> line;
                        int stringNumber = 0; //number of the line
                        for (int i = 0; i < pagesize; i++) {
                            char c = map[i];
                            if (c == '\n'){
                                stringNumber ++;
                                m_automat.lock();
                                automat.printIfWordPresent(line, filePath, stringNumber);
                                m_automat.unlock();
                                while (line.empty() != true) {
                                    line.pop_back();
                                }
                            }
                            else if (c == EOF) { //I suppose this is useless, but I'm not sure
                                break;
                            }
                            else {
                                line.push_back(c);
                            }
                        }
                        if (line.empty() != true){
                            stringNumber++;
                            m_automat.lock();
                            automat.printIfWordPresent(line, filePath, stringNumber);
                            m_automat.unlock();
                            while (line.empty() != true) {
                                line.pop_back();
                            }
                        }
                        munmap(map, pagesize);
                    }
                }
                free(filePath);
            }
        }
    }
}

int main(int argc, char *argv[], char *env[]) {
    vector<char> vword;
    int flagn = 0; // -n flag
    int flagt = 0; // -t flag
    int flagp = 0; // path flag
    char * path; //directory path
    char * flagpt; //-t string
    int threadamount = 0;
    vector<char *> words;
    for (int i = 1; i < argc; i++) { //parsing arguments of main
        if (strcmp(argv[i],"-n") == 0) {
            flagn = 1;
        }
        else if (*argv[i] == '/'){
            flagp = 1;
            path = argv[i];
        }
        else if ((*argv[i] == '-') and (*(argv[i] + 1) == 't')) {
            flagt = 1;
            flagpt = argv[i];
        }
        else { //words to search
            words.push_back(argv[i]);
        }
    }
    for (int i = 0; i < words.size(); i++){ //rebuilding words to search
        char * curword = words[i];
        for (int j = 0; j < strlen(curword); j++){
            vword.push_back(curword[j]);
        }
        vword.push_back(' ');
    }
    vword.pop_back();
    if (flagt == 1){ //counting number of threads
        for (int i = 2; i < strlen(flagpt); i++){
            threadamount = threadamount*10 + flagpt[i] - 48;
        }
    }
    else {
        threadamount = 1;
    }
    if (flagp == 0){ //finding current directory (may contain error while finding)
        path = get_current_dir_name();
    }
    vector<char *> fileNames = findFileNames(flagn,path); // finding all files
    mutex m_fileNames; //for blocking vector of files
    mutex m_automat; //for blocking automat
    Kmp Automat(vword); //building automat
    thread findingthread[threadamount];
    for (int i = 0; i < threadamount; i++){ //starting threads
        findingthread[i] = thread(findLines, ref(fileNames), ref(m_fileNames), ref(m_automat), ref(Automat));
    }
    for (int i = 0; i < threadamount; i++){ //finishing threads
        findingthread[i].join();
    }
    printf("Program finished\n");
    return 0;
}