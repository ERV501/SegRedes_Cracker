#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <chrono>
#include <condition_variable>


using namespace std;

//containers
vector <char> vector_Char_buffer;
queue <string> queue_Passwd_buffer = queue<string>();
queue <string> queue_Passwd_try = queue<string>(); // cola de passwd a probar para cada hilo

//misc
string passwd_not_solved;
string opt_argument;
string path_to_file;
int passwd_lenght;
unsigned int n_threads;

//global check
bool breached_passwd = false;

//Synchronization
mutex mtx_Passwd_insert;
condition_variable cv_Passwd_try;

//functions
int main(int argc, char *argv[]);
void parseArgs(int argc, char *argv[], int &passwd_lenght);
void initializeSetup(int &passwd_lenght);
void bruteForceCharPool(char start_point, char end_point);
void launchThreads(int n_threads, vector <thread>&vector_Threads);
void bruteForceGeneratePasswd(int passwd_lenght);
void bruteForceOperation();
void threadFinalization(vector <thread>&vector_Threads);

int main(int argc, char *argv[]){
    vector_Char_buffer = vector<char>(); //vector para guardar todos los caracteres q se probaran para la passwd [a-z]
    vector<thread> vector_Threads = vector <thread>(); //vector de hilos
    queue_Passwd_buffer = queue<string>(); //cola con las posibles passwd generadas (totales)

    initializeSetup(passwd_lenght); //inicializar variables
    parseArgs(argc,argv,passwd_lenght); //leer y guardar argumentos
    bruteForceCharPool('a','z'); //Generar caracteres de la passwd, en este caso, minusculas de [a-z]
    launchThreads(n_threads,vector_Threads); //Lanzar hilos [maximo de hilos soportados por defecto]
    threadFinalization(vector_Threads); //Esperar finalizacion de hilos
    
    if(passwd_not_solved == string("")){
        cout << "-> Password \033[1;32m could not be found \033[0m \n";
    }

    return EXIT_SUCCESS;
}

void initializeSetup(int &passwd_lenght){
    passwd_lenght = 0; //en caso de no especificar un valor limite, por defecto en 4
    n_threads = std::thread::hardware_concurrency();
}

void parseArgs(int argc, char *argv[], int &passwd_lenght){

    for(int i = argc - 1; i > 0 ; i--){
        if(*(++argv)[0]=='-'){
            opt_argument = string(&((*argv)[1]));

           if (opt_argument == string("len")){
               passwd_lenght = atoi(*(++argv)); //guardar longitud definida para la password
               i--; //saltar y seguir
           }
           else if (opt_argument == string("file")){
               path_to_file = string(*(++argv)); //guardar ruta del archivo
               i--; //saltar y seguir
           }
        }
    }

    if(passwd_lenght != 0){
        cout << "-> Arguments: password length =\033[1;32m " << passwd_lenght << "\033[0m, file = \033[1;32m" << path_to_file << "\033[0m\n";
    }else{
        cout << "-> Arguments: password length\033[1;31m not specified\033[0m, please specify one using\"-len\" \n";
        exit(EXIT_FAILURE);
    }

}

void  bruteForceCharPool(char start_point, char end_point){
    for(char i = start_point; i <= end_point; i++){
        vector_Char_buffer.push_back(i); //generar grupo de caracteres a usar
    }
}

void launchThreads(int n_threads, vector <thread>&vector_Threads){
    cout << "-> Creating \033[1;32m" << n_threads << " threads \033[0m \n";

    vector_Threads.push_back(thread(bruteForceGeneratePasswd,passwd_lenght)); //hilo creador de claves
    for(int i=0;i < n_threads;i++){
        vector_Threads.push_back(thread(bruteForceOperation)); //hilos que comprueban claves creadas
    }
}

void bruteForceGeneratePasswd(int passwd_lenght){

    queue_Passwd_buffer.push(""); //valor inicial string vacio para evitar problemas

    string passwd_previous = "",passwd_appended = "";
    int i=0;

    //generar passwd:
    while(!breached_passwd){
        if(queue_Passwd_buffer.size()!=0){
            
            passwd_previous = queue_Passwd_buffer.front(); // guardar valor anterior -> Ej: [a] -> a_
            queue_Passwd_buffer.pop(); // expulsamos valor de la cola

            for(i=0;i<vector_Char_buffer.size();i++){ //iterar sobre todos los valores posibles [a-z]
                passwd_appended = passwd_previous + vector_Char_buffer[i]; // anexar al valor anterior cada valor posible -> Ej: [a] -> aa, ab, ac...

                if(passwd_appended.length() != passwd_lenght){ //Si no coincide con el tamaño deseado, no comprobar, seguir aumentando la passwd
                    queue_Passwd_buffer.push(passwd_appended); // guardar valor actual, para anexar la próxima iteración a este

                }else{
                    unique_lock<mutex> lk_Passwd_try(mtx_Passwd_insert);
                    queue_Passwd_try.push(passwd_appended); // guardar passwd q el hilo comprobará (critical_section)
                    lk_Passwd_try.unlock();
                    cv_Passwd_try.notify_one();
                }
            }

        }
    }
}

void bruteForceOperation(){
    string sys_command;
    string passwd_to_check;

    mutex mtx_Passwd_try;

    while(!breached_passwd){

        unique_lock<mutex>lk_Passwd_try(mtx_Passwd_try);
        cv_Passwd_try.wait(lk_Passwd_try,[]{return !queue_Passwd_try.empty();}); // Despierta cuando no esté vacía

        while(!queue_Passwd_try.empty()){
            //cout << "probando passwd_to_check: "<< passwd_to_check <<"\n";
            unique_lock<mutex> lk_Passwd_try(mtx_Passwd_insert);
            passwd_to_check = queue_Passwd_try.front(); //guardar contraseña a probar
            queue_Passwd_try.pop(); //quitar de la cola
            lk_Passwd_try.unlock();
            
            //Probar contraseña usando comando de terminal:
            sys_command = string("gpg --batch --yes --passphrase ") + passwd_to_check + string(" -d ") + path_to_file + string(" >>/dev/null 2>>/dev/null");

            if(system(sys_command.c_str()) == 0){

                
                cout << "-> Password found: \033[1;32m" << passwd_to_check << "\033[0m \n";
                exit(EXIT_SUCCESS);
                
            }else{
                passwd_not_solved="";
            }
        }
        
        breached_passwd = true;

    }
}


void threadFinalization(vector <thread>&vector_Threads){
    for(int i=0;i<vector_Threads.size();i++){ //Esperar finalizacion de hilos
        if(vector_Threads.at(i).joinable()){
            vector_Threads.at(i).join();
        }
    }
}