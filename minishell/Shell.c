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
static pid_t jobsTab[10];
int increment = 0;

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

static void
do_help( struct Shell *this, const struct StringVector *args )
{
    printf( "-> commands: exit, cd, help, ?, pwd, external commands : ! commands -options.\n" );
    (void)this;
    (void)args;
}

static void
externalCMD(struct Shell *this, const struct StringVector *args) {
    char *tmp = malloc(256*sizeof(char));
    char *path = malloc(256*sizeof(char));
    //char *options = malloc(256*sizeof(char));
    int j = 0;
        strjoinarray(tmp, args, 1, args->size, " ");
        //strjoinarray(options, args, 1, args->size, " ");
        //printf("test otions split : %s \n", options);
        //char *tmp2[256] = {options, NULL};
        char *tmp2[256];
        for (size_t i=1; i<args->size; i++){
            tmp2[j] = args->strings[i];
            j ++;
        }
        tmp2[j+1] = NULL;
        tmp2[j+2] = '\0';
        strcat(path, "/bin/");
        strcat(path, args->strings[1]);
        execv(path, tmp2);

    free(path);
    free(tmp);
    //free(options);
    (void)this;
}

static void
do_system( struct Shell *this, const struct StringVector *args )
{
    pid_t p = fork();
    int s;
    // Il fuat trouver un moyen pour supprimer les processus qui sont terminés
    jobsTab[increment] = p;
    increment += 1;
        if (p == -1){
            printf("error");
            exit(EXIT_FAILURE);
        } else if (p == 0) {
            externalCMD(this, args);
            exit(EXIT_SUCCESS);
        }
    if (strcmp(args->strings[args->size - 1],"&") != 0) {
        waitpid(p, &s, 0);
    }
}

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

static void
do_jobs()
{
    for (int i=0; i<increment; i++){
        printf("Id du processus %d : %d \n",i+1,jobsTab[i]);
    }
}

typedef void ( *Action )( struct Shell *, const struct StringVector * );

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
