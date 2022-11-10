#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/wait.h>

#include "Shell.h"
#include "StringVector.h"

// Il n'y aura que 10 tâches executables en arrières plan

int increment = 0;
int n = 0;
struct Job{
    pid_t pid;
    int num;
};
struct Job jobsTab[10];
pid_t pid;

void
shell_init( struct Shell *this )
{
    this->running     = false;
    this->line        = NULL;
    this->line_number = 0;
    this->line_length = 0;
}

void
shell_free( struct Shell *this )
{
    if ( NULL != this->line ) {
        free( this->line );
        this->line = NULL;
    }
    this->line_length = 0;
}

void
shell_run( struct Shell *this )
{
    this->running = true;
    printf( "* Shell started\n" );
    while ( this->running ) {
        shell_read_line( this );
        shell_execute_line( this );
    }
    printf( "* Shell stopped\n" );
}

void
shell_read_line( struct Shell *this )
{
    this->line_number++;
    char *buf = getcwd( NULL, 0 );
    printf( "%d: %s> ", this->line_number, buf );
    free( buf );
    getline( &this->line, &this->line_length, stdin );
}

/*
* Fonction : do_help
* ------------------
* Affiche une aide pour savoir quelles commandes sont implémentées 
*
* *this : le shell actif 
* *args : ce que l'utilisateur a rentrer dans le terminal
*/
static void
do_help( struct Shell *this, const struct StringVector *args )
{
    printf( "-> commands: exit, cd, help, ?, pwd, jobs / external commands : ! commands -options.\n" );
    (void)this;
    (void)args;
}

static void
externalCMD(struct Shell *this, const struct StringVector *args) {
    char *tmp = malloc(256*sizeof(char));
    char *path = malloc(256*sizeof(char));
    int j = 0;
    strjoinarray(tmp, args, 1, args->size, " ");
    char *tmp2[args->size];
    for (size_t i=1; i<args->size; i++){
        if (strcmp(args->strings[i],"&") != 0){
            tmp2[j] = args->strings[i];
            j++;
        }
    }
    tmp2[j] = '\0';
    strcat(path, "/bin/");
    strcat(path, args->strings[1]);
    execv(path, tmp2);

    free(path);
    free(tmp);
    (void)this;
}


/*void fin_job(){
    for(int i=0; i<increment; i++){
        if (jobsTab[i].pid == pid){
            jobsTab[i].pid = 0;
        }
    }
}*/

/*
* Fonction : do_system
* --------------------
* Permet de lancer les commandes externes correctement
*   (en arrière plan si un '&' a été ajouté a la fin de la commande) 
*
* *this : le shell actif 
* *args : ce que l'utilisateur a rentrer dans le terminal
*/
static void
do_system( struct Shell *this, const struct StringVector *args )
{
    int s;
    pid_t p = fork();
    struct Job actualjob =
    {
        p,n
    };
    n = n+1;

    jobsTab[increment] = actualjob;
    increment += 1;
    if (p == -1){
        printf("error");
        exit(EXIT_FAILURE);
    } else if (p == 0) {
        externalCMD(this, args);
        exit(EXIT_SUCCESS);
    }
    if (strcmp(args->strings[args->size - 1],"&") != 0) {  
        pid = waitpid(p, &s, 0);    
        //signal(SIGCHLD, fin_job);
    }
}


/*
* Fonction : do_cd
* ----------------
* Permet d'effectuer la commande cd qui permet
*   de changer de dossier
*
* *this : le shell actif 
* *args : ce que l'utilisateur a rentrer dans le terminal
*/
static void
do_cd( struct Shell *this, const struct StringVector *args )
{
    int   nb_tokens = string_vector_size( args );
    char *tmp;
    if ( 1 == nb_tokens ) {
        tmp = getenv( "HOME" );
    }
    else {
        tmp = string_vector_get( args, 1 );
    }
    int rc = chdir( tmp );
    if ( 0 != rc )
        printf( "directory '%s' not valid\n", tmp );
    (void)this;
}

static void
do_rappel( struct Shell *this, const struct StringVector *args )
{
    (void)this;
    (void)args;
}

static void
do_execute( struct Shell *this, const struct StringVector *args )
{
    (void)this;
    (void)args;
}

static void
do_exit( struct Shell *this, const struct StringVector *args )
{
    this->running = false;
    (void)this;
    (void)args;
}

/*
* Fonction : do_pwd
* -----------------
* Permet de lancer la commande pwd qui permet
*   de voir le chemin actuel
*
* *this : le shell actif 
* *args : ce que l'utilisateur a rentrer dans le terminal
*/
static void
do_pwd( struct Shell *this, const struct StringVector *args)
{
    int   nb_tokens = string_vector_size( args );
    char tmp[1024];
    if ( 1 == nb_tokens ) {
        getcwd(tmp, 1024);
        printf("Le chemin est : %s\n",tmp);
    } else {
        printf("Entrer une commande valide\n");
    }
    (void)this;
}

/*
* Fonction : do_jobs
* ------------------
* Permet de lancer la commande jobs qui affiche
*   les processus en cours
* *this : le shell actif 
* *args : ce que l'utilisateur a rentrer dans le terminal
*/
static void
do_jobs()
{
    for (int i=0; i<increment; i++){
        printf("Id du processus %d : %d \n",i+1,jobsTab[i].pid);
    }
}

typedef void ( *Action )( struct Shell *, const struct StringVector * );

/*
* Struct Action
* ------------------
* Regroupe toutes les actions (commandes) utilisables
*/
static struct {
    const char *name;
    Action      action;
} actions[] = { { .name = "exit", .action = do_exit },     { .name = "cd", .action = do_cd },
                {.name = "pwd", .action = do_pwd},         {.name = "jobs", .action = do_jobs},
                { .name = "rappel", .action = do_rappel }, { .name = "help", .action = do_help },
                { .name = "?", .action = do_help },        { .name = "!", .action = do_system },
                { .name = NULL, .action = do_execute } };

Action
get_action( char *name )
{
    int i = 0;
    while ( actions[i].name != NULL && strcmp( actions[i].name, name ) != 0 ) {
        i++;
    }
    return actions[i].action;
}

void
shell_execute_line( struct Shell *this )
{
    struct StringVector tokens    = split_line( this->line );
    int                 nb_tokens = string_vector_size( &tokens );

    if ( nb_tokens == 0 ) {
        printf( "-> Nothing to do !\n" );
    }
    else {
        char  *name   = string_vector_get( &tokens, 0 );
        Action action = get_action( name );
        action( this, &tokens );
    }

    string_vector_free( &tokens );
}
