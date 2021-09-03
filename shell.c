#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifdef DEBUG
    #define DPRINT(x) do {printf x;} while(0)
#else
    #define DPRINT(x) do {} while(0)
#endif

struct words {
    char *smbl;
    struct words *next;
};

struct files {
    char *in;
    char *out;
    char *inEnd;
};

struct numFiles {
    int in;
    int out;
    int inEnd;
};

enum report {
    space, endOfLine, endOfFile, errQuote, errAmp, errArr, errStick,
    errNumArg, ampersand, inFile, inFileEnd, fromFile, stick
};

void bufferInc(int **buffer, int *bufferSize)
{
    int *tmp, i;
    tmp = malloc(2 * (*bufferSize) * sizeof(*tmp));
    for (i=0; i < 2 * (*bufferSize); i++) 
        *(tmp+i) = 0;
    for (i=0; i < *bufferSize; i++)
        *(tmp+i) = *(*buffer+i);
    free(*buffer);
    *buffer = tmp;
    *bufferSize = 2 * (*bufferSize);
}

int isItAmpersand()
{
    int c, symb=0;
    while ((c=getchar()) != EOF && c != '\n'){
        if (c != ' ')
            symb = 1; 
    }
    if (symb==1)
        return errAmp;
    if (c=='\n')
        return ampersand;
    else
        return endOfFile;
}

int wordToBuffer(int **buffer, int *bufferSize)
{
    int ptr = 0, i, isOpen = 0, flag=-1;
    int c = -1;
    for (i=0; i < *bufferSize; i++) 
        *(*buffer + i) = 0;
    while ((c = getchar()) && (c != -1) && (c != '\n')
          && (c != ' ' || isOpen == 1)){
        if (c == '|') {
            flag = stick;
            break;
        }
        if (c == '>' || c == '<'){
            flag = (c=='>') ? inFile : fromFile;
            break;
        }
        if (c == '&' && isOpen == 0){
            flag = isItAmpersand();
            break;
        }
        if (ptr >= *bufferSize - 2)
            bufferInc(buffer,bufferSize);
        if (c != '\"'){
            *(*buffer + ptr) = c;
            ptr++;
        } else
            isOpen = (isOpen+1) % 2;
    }
    if (flag == -1) 
        switch (c) {
            case '\n':
                flag = endOfLine;
                break;
            case -1: 
                flag = endOfFile; 
                break;
            case ' ': 
                flag = space; 
                break;
        }
    if (isOpen == 1)
        flag = errQuote;
    return flag; 
}

struct words *bufferToList(int *buffer, int *ctr)
{
    int i, m = 0;
    struct words *tmp;
    while (*(buffer + m) != 0) 
        m++;
    if (m == 0)
        return NULL;
    tmp = malloc(sizeof(*tmp));
    (*tmp).smbl = malloc((m+2)*sizeof((*(*tmp).smbl)));
    for (i=0; i<=m; i++)
        *((*tmp).smbl + i) = *(buffer + i);
    *((*tmp).smbl + m + 1) = 0;
    (*tmp).next = NULL;
    (*ctr)++;
    return tmp;        
}

void wordsListOut(const struct words *list)
{
    if (list != NULL){
        wordsListOut(list->next);
        printf("%s\n",(*list).smbl);
    }
}

void wordsListOutCount(const struct words *list, const int ctr, const int n)
{
    if (list != NULL){
        wordsListOutCount(list->next, ctr, n+1);
        printf("%d %s\n",ctr-n,(*list).smbl);
    }
}

void wordsListFree(struct words **pWordsList)
{
    struct words *tmp;
    if (*pWordsList != NULL){
        tmp = *pWordsList;
        *pWordsList = (*pWordsList)->next;
        free((*tmp).smbl);
        free(tmp);
        wordsListFree(pWordsList);
    }      
}        

int wordsNum(const struct words *list)
{
    if (list == NULL) 
        return 0;
    else 
        return 1 + wordsNum(list->next);
}       

int smblNum(const char *c)
{
    int i = 0;
    while (*(c+i) != 0) 
        i++;
    return i;
} 

int argsNum(char **arg)
{
    int i=0;
    while (*(arg+i) != 0)
        i++;
    return i;
}        

char ***wordsListToArg(struct words *wordsList, int *stickMas)
{
    char ***Parg;
    int wrdNumArg, words_number, smbl_number, arg_number=0, i, j, k, ind;
    struct words *tmp;
    tmp = wordsList;  
    while (stickMas[arg_number] != 0)
        arg_number++;
    arg_number++;
    words_number = wordsNum(wordsList);
    Parg = malloc((arg_number+1)*sizeof(*Parg));
    for (k=0; k<arg_number; k++){ 
        ind = arg_number - k - 1;     
        if (ind == 0){
            if (stickMas[ind] == 0)
                wrdNumArg = words_number;
            else
                wrdNumArg = stickMas[ind];
        }
        else if (ind==arg_number - 1)
            wrdNumArg = (words_number)-stickMas[ind-1];
        else 
            wrdNumArg = stickMas[ind]-stickMas[ind-1];
        *(Parg + arg_number - k - 1) = malloc((wrdNumArg+1) * sizeof(**Parg));
        for (i=0; i<wrdNumArg; i++){
            smbl_number = smblNum(tmp->smbl);
            *(*(Parg + arg_number - k - 1) + wrdNumArg - i - 1) 
                = malloc((smbl_number + 1) * sizeof(***Parg));
            for (j=0; j<=smbl_number; j++)
                *(*(*(Parg + arg_number - k - 1) + wrdNumArg-i-1)+j) 
                    = *((tmp->smbl) + j);
            tmp = tmp -> next;
        }
        *(*(Parg + arg_number - k - 1) + wrdNumArg) = NULL;
    }
    *(Parg + arg_number) = NULL;
    return Parg;
}

void argOut(char ***arg)
{
    int j=0, k=0;
    while (*(arg+k) != NULL){
        while (*(*(arg+k)+j) != NULL){
            printf("%s\n",*(*(arg+k)+j));
            j++;
        }
        k++;
        j = 0;
        printf("\n");
    }
}

int isItCd(const char *arg_1)
{
    return (*arg_1 == 'c') && (*(arg_1+1) == 'd') && (*(arg_1+2) == 0);
}

int changeThreadOut(struct files fname)
{
    int f;
    if (fname.out != 0){
        f = open(fname.out, O_RDONLY);
        if (f==-1){
            perror(fname.out);
            return -1;
        }else {
            dup2(f,0);
            close(f);
        }
    }
    return 0;
}  
   
int changeThreadIn(struct files fname)
{
    int f;    
    if (fname.in != 0){
        f = open(fname.in, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (f==-1){
            perror(fname.in);
            return -1;
        } else {
            dup2(f,1);
            close(f);
        }
    } 
    if (fname.inEnd != 0){
        f = open(fname.inEnd, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (f==-1){
            perror(fname.inEnd);
            return -1;        
        }else {
            dup2(f,1);
            close(f);
        }
    }
    return 0;
}

void freeArg(char ****arg)
{
    int i=0, j=0;
    while (*(*arg+i) != NULL){
        while (*(*(*arg+i)+j) != NULL){
            free (*(*(*arg+i)+j));
            j++;
        }
        j = 0;
        i++;
        free(*(*arg+i));
    }
    free(*arg);   
} 

int checkPid(int *pid, int ptr, int w)
{
    int i, flag=0;
    for (i=0; i<ptr; i++){
        if (pid[i] != 0)
            flag = 1;
        if (w == pid[i]){
            pid[i] = 0;
            if (checkPid(pid,ptr,w) == 0)
                return 0;
        }
    }
    return flag;
}
    

void makeCommand(char ****pArg, const int result, struct files fname) 
{
    int p, w, j=0, pidSize = 1, ptr=0;
    int fd[2], d=0, *pid;
    pid = malloc(sizeof(*pid));
    *pid = 0;
    if (*pArg != NULL){
        if (isItCd(***pArg)){
            if (argsNum(**pArg) == 2){
                p = chdir(*(**pArg+1));
                if (p == -1) 
                    perror(*(**pArg+1));    
            } else
                perror(*(**pArg+1));
        } else {
            while (*(*pArg+j) != NULL){
                pipe(fd);
                p = fork();
                if (p == 0) {        
                    if (j == 0){
                        if (changeThreadOut(fname)==-1)
                            exit(1);
                    } else {
                        dup2(d,0);
                        close(d);
                    }
                    if (*(*pArg+j+1) != NULL)
                        dup2(fd[1],1);
                    else {
                        if (changeThreadIn(fname)==-1)
                            exit(1);
                    }                  
                    close(fd[1]);
                    close(fd[0]);
                    execvp(**(*pArg+j), *(*pArg+j));
                    perror(**(*pArg+j));
                    exit(1);
                }
                DPRINT(("PID %d\n",p));
                if (j != 0)
                    close(d);
                d = dup(fd[0]);
                close(fd[1]);
                close(fd[0]);
                if (result == endOfLine){
                    if (ptr >= pidSize)
                        bufferInc(&pid, &pidSize);
                    pid[ptr] = p;
                    ptr++;
                }
                j++;
            }  
            if (result == endOfLine)
                while (1){
                    w = wait(NULL);
                    if (checkPid(pid,ptr,w) == 0)
                        break;
                }  
        }
    }
    DPRINT(("check free 1\n"));
    free(pid);
    DPRINT(("check free 2\n"));
    freeArg(pArg); 
    DPRINT(("check after free arg\n"));
}

void cleanNumFiles(struct numFiles *nfile)
{
    (*nfile).in = 0;
    (*nfile).out = 0;
    (*nfile).inEnd = 0;
}

char *copyStr(char *str)
{
    char *tmp;
    int size=0, i;
    while (str[size++] != 0)
        size++;
    tmp = malloc((size+1)*sizeof(*tmp));
    for (i=0; i<=size; i++)
        tmp[i] = str[i];
    return tmp;
}
    
void fileName(struct words **wordsList, struct files *fname, 
                struct numFiles nfile, int ctr)
{
    struct words *tmp;
    int i, j, k, masFile[3]={nfile.in, nfile.out, nfile.inEnd};
    if (masFile[0] == 0 && masFile[1] == 0 && masFile[2] == 0)
        return;
    for (i=0; i<3; i++)
        if (masFile[i] != 0)
            masFile[i] = ctr - masFile[i];       
    for (k=0; k<2; k++)
        for (i=0; i<3; i++)
            if (masFile[i] == 1){
                switch (i){
                    case 0:
                        (*fname).in = copyStr((*wordsList)->smbl);
                        break;
                    case 1:
                        (*fname).out = copyStr((*wordsList)->smbl);
                        break;
                    case 2:
                        (*fname).inEnd = copyStr((*wordsList)->smbl);
                        break;
                }
                free((*wordsList)->smbl);
                tmp = *wordsList;
                (*wordsList) = (*wordsList)->next;
                free(tmp);
                for (j=0; j<3; j++){
                    masFile[j]--;
                }
                break;
            }
}   
    
void cleanFileNames(struct files *fname)
{
    if ((*fname).in != 0)
        free((*fname).in);
    if ((*fname).out != 0)
        free((*fname).out);
    if ((*fname).inEnd != 0)
        free((*fname).inEnd);
    (*fname).in = 0;
    (*fname).out = 0;
    (*fname).inEnd = 0;
}
    
int checkForCtr(struct numFiles nfile, int ctr)
{
    int max, maxIn=nfile.in, maxOut=nfile.out;
    if (maxIn < nfile.inEnd)
        maxIn = nfile.inEnd;
    max = maxIn;
    if (max < maxOut)
        max = maxOut;
    if (max == 0)
        return 0;
    if ((ctr-max) > 1 || ((maxOut-maxIn) > 1 && maxIn != 0) 
        || ((maxIn-maxOut) > 1 && maxOut != 0))
        return 1;
    return 0;
} 
    
void changeNumFile(struct numFiles *nfile, int *result, int *i, int ctr)
{
    (*i)++;
    if (*result == fromFile){
        if ((*nfile).out != 0)             
            *result = errArr;
        else
            if ((*nfile).in == ctr)
                *result = errArr;
            else
                (*nfile).out = ctr;
    }
    if (*result == inFile){
        if ((*nfile).in != 0 || (*nfile).inEnd != 0){
            if ((*nfile).in == ctr && (*i)==1){
                (*nfile).in = 0;
                (*nfile).inEnd = ctr;
            } else 
                *result = errArr;
        } else 
            if ((*nfile).out == ctr)
                *result = errArr;
            else
                (*nfile).in = ctr;
        *i = 0;
    }  
}
    
void errorReport(int result)
{
    printf("ERROR: ");
    switch (result){
        case errQuote: 
            printf("Quotes not closed\n");
            break;
        case errAmp:
            printf("Symbols after &\n");
            break;
        case errNumArg:
            printf("Wrong number of arguments\n");
            break;
        case errArr:
            printf("Wrong using of >,<,>>\n");
            break;
        case errStick:
            printf("Wrong using of |\n");
            break;   
    }
    //printf(">>");
}
    
int numStick(int **stickMas, int *stickSize, int ctr)
{
    int ptr = 0;
    while (*(*stickMas + ptr) != 0)
        ptr++;
    if (ptr >= *stickSize-1)
        bufferInc(stickMas, stickSize);
    if (ptr != 0 && (*(*stickMas + ptr - 1)) == ctr)
        return -1;
    *(*stickMas + ptr) = ctr;
    return 0;
}
    
void masOut(int *mas)
{
    int i=0;
    while (mas[i] != 0){
        printf("%d ",mas[i]);
        i++;
    }
    printf("\n");
} 
    
void cleanStickMas(int **stickMas)
{
    int i=0;
    while (*(*stickMas + i) != 0){
        *(*stickMas + i) = 0;
        i++;
    }
}
    
int main()
{
    int result, bufferSize = 1, ctr=0, i=0, stickSize = 1, clean=0;
    int *buffer, *stickMas;
    char ***arg;
    struct words *wordsList, *tmp;
    struct files fname={0,0,0};
    struct numFiles nfile={0,0,0};
    wordsList = NULL;        
    buffer = malloc(sizeof(*buffer));
    stickMas = malloc(sizeof(*stickMas));
    *stickMas = 0;
    tmp = NULL;
    arg = NULL;
    printf(">>");
    do {
        result = wordToBuffer(&buffer, &bufferSize);     
        tmp = bufferToList(buffer,&ctr);
        if (tmp != NULL){
            (*tmp).next = wordsList;
            wordsList = tmp;
        }
        changeNumFile(&nfile, &result, &i, ctr);
        if (result == stick){
            if (numStick(&stickMas, &stickSize, ctr)==-1)
                result = errStick;
        }
        if ((result == endOfLine || result == ampersand) 
                && checkForCtr(nfile,ctr)){
            result = errNumArg;
        }
        if (result == errQuote || result == errNumArg 
        || result == errArr || result == errAmp || 
        result == errStick){
            clean = 1;
            errorReport(result);
            #if 0
            wordsListFree(&wordsList);
            if (result == errArr || result == errStick)
                isItAmpersand();
            cleanNumFiles(&nfile);
            cleanFileNames(&fname);
            cleanStickMas(&stickMas);
            ctr = 0;
            #endif
        }      
        if (result == endOfLine || result == ampersand){
            fileName(&wordsList, &fname, nfile, ctr);
            arg = wordsListToArg(wordsList, stickMas);
            makeCommand(&arg, result, fname);
            //printf(">>");
            clean = 1;
            #if 0
            ctr = 0;
            wordsListFree(&wordsList);
            cleanFileNames(&fname);
            cleanNumFiles(&nfile);
            cleanStickMas(&stickMas);
            #endif
        }
        #if 1
        if (clean == 1){
            wordsListFree(&wordsList);
            if (result == errArr || result == errStick)
                isItAmpersand();
            cleanNumFiles(&nfile);
            cleanFileNames(&fname);
            cleanStickMas(&stickMas);
            ctr = 0;
            clean = 0;
            printf(">>");
        }
        #endif
    } while (result != endOfFile);
    free(buffer);
    free(stickMas);
    return 0;
}
