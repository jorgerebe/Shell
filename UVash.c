/**
 * Autor: Jorge Rebé Martín
 * Asignatura: ESO
 * Examen 25-03-2021
 * Funcionalidad
 * 	Lo que sí funciona:
 * 		
 * 	Lo que no funciona:
 */ 		


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


struct comando
{
   int argc;
   char* argv[10];
   bool Redireccion;
   char* archivoRedireccion;
};

//funciones
void leeComandos(bool);
int ncomandos(char* linea);
char** parseaComandos(int n, char* linea);
bool esRedireccion(char*);
char* parseaArchivo();
void parsea(char*[], int);
int ejecuta(struct comando*, int);
bool builtInCommand(struct comando);
void salir(int);
void cd(int, char*[]);
int contarArgumentos(char*);
void error();

FILE* Fichero;
struct comando* comandos;
char* linea;
char mensaje_error[] = "An error has occurred\n";

#define DEPURA

//main
int main(int argc, char* argv[])
{
   bool interactivo;

   if(argc > 2)
   {
      error();
      return 1;
   }

   else if(argc == 2)
   {
      Fichero = fopen(argv[1], "r");
      if(Fichero == NULL)
      {
         error();
         return 1;
      }
      interactivo = false;
   }
   else
   {
      Fichero = stdin;
      interactivo = true;
   }

   leeComandos(interactivo);
   return 0;
}

void leeComandos(bool interactivo)
{
   size_t valor;
   ssize_t tamano;

   linea = NULL;
   valor = 0;

   while(true)
   {
      linea = NULL;

      if(interactivo == true)
      {
         printf("UVash> ");
      }

      tamano = getline(&linea, &valor, Fichero);

      if(tamano != -1)
      {
         int numeroComandos;
         int i;
         char* aux = linea;
         char* palabra;
         char delim[] = "&";


         linea[strlen(linea)-1] = '\0';
         numeroComandos = ncomandos(linea);

         if(numeroComandos == 0)
         {
            if(strchr(linea, '&') != NULL)
            {
               error();
            }
            free(linea);
            continue;
         }

         char* comandosSeparados[numeroComandos];

         palabra = strtok(aux, delim);
         i = 0;

         while(palabra != NULL)
         {
            comandosSeparados[i] = palabra;
            i++;
            palabra = strtok(NULL, delim);
         }

         comandosSeparados[i] = palabra;

         parsea(comandosSeparados, numeroComandos);
	      free(comandos);
         free(linea);
         valor = 0;
      }
      else
      {
	      valor = 0;
	      fclose(Fichero);
         free(linea);
         return;
      }
   }
}

int ncomandos(char* linea)
{
   int i;
   bool hayRedir = false;
   bool habiaRedir = false;
   bool hayEspacio = false;
   int comandos = 0;
   int tamano = strlen(linea);


   if(linea[0] == '&')
   {
      hayRedir = true;
   }

   if(linea[0] == ' ' || linea[0] == '\t')
   {
      hayEspacio = true;
   }

   if(hayRedir == false && hayEspacio == false && linea[0] != '\0')
   {
      comandos++;
   }

   i = 0;

   while(i < tamano)
   {
      if(linea[i] == ' ' || linea[i] == '\t')
      {
         hayEspacio = true;
      }

      if(linea[i] == '&')
      {
         hayRedir = true;
      }

      if(hayEspacio == false && hayRedir == false && (habiaRedir == true || comandos == 0))
      {
         comandos++;
         habiaRedir = false;
      }

      if(habiaRedir == true && hayEspacio == true)
      {
         hayRedir = true;
      }

      habiaRedir = hayRedir;
      hayRedir = false;
      hayEspacio = false;

      i += sizeof(linea[i]);
   }

   return comandos;
}

// Parsea y ejecuta
void parsea(char* comandosSeparados[], int ncomandos)
{
   int argsTotales = 0;
   int i;
   int j;
   char delimRedireccion[] = ">";
   char delimArgv[] = " \t";
   char* palabra;

   for(i = 0; i < ncomandos; i++)
   {
      argsTotales += contarArgumentos(comandosSeparados[i]);
   }

   comandos = (struct comando*)malloc(ncomandos * (sizeof(struct comando)));

   i = 0;
   while(i < ncomandos)
   {

      if(contarArgumentos(comandosSeparados[i]) == 0)
      {
         ncomandos--;
         continue;
      }

      comandos[i].Redireccion = esRedireccion(comandosSeparados[i]);

      if(comandos[i].Redireccion == true)
      {
         comandosSeparados[i] = strtok(comandosSeparados[i], delimRedireccion);
         
         comandos[i].archivoRedireccion = parseaArchivo();

         if(comandos[i].archivoRedireccion == NULL)
         {
            error();
            ncomandos--;
            continue;
         }
      }

      comandos[i].argc = contarArgumentos(comandosSeparados[i]);

      if(comandos[i].argc == 0)
      {
         error();
         continue;
      }


      palabra = strtok(comandosSeparados[i], delimArgv);
      j = 0;

      while(palabra != NULL)
      {
         comandos[i].argv[j] = palabra;
         j++;
         palabra = strtok(NULL, delimArgv);
      }

      comandos[i].argv[j] = palabra;
      i++;
   }

#undef DEPURA
#ifdef DEPURA
   for(i = 0; i < ncomandos; i++)
   {
      printf("NArgumentos: %d\n", comandos[i].argc);
      printf("EsRedireccion: %s\n", comandos[i].Redireccion?"true":"false");
      printf("ArchivoRedireccion: %s\n", comandos[i].archivoRedireccion);
      for(j = 0; j < comandos[i].argc; j++)
      {
         printf("Arg %d.- %s\n", j, comandos[i].argv[j]);
      }
   }
#endif

   if(ncomandos > 0)
   {
      ejecuta(comandos, ncomandos);
   }
}

bool esRedireccion(char* linea)
{
   int i;
   int n = strlen(linea);
   int contador = 0;
   bool redir = false;

   for(i = 0; i < n; i++)
   {
      if(linea[i] == '>')
      {
         redir = true;
         contador++;
      }
   }
   return redir;
}

char* parseaArchivo()
{
   char* palabra = strtok(NULL, " \t");
   if(strtok(NULL, " \t") != NULL)
   {
      palabra = NULL;
   }

   return palabra;
}

//Ejecuta
int ejecuta(struct comando* comandos, int ncomandos)
{
   int i;

   for(i = 0; i < ncomandos; i++)
   {

      if(builtInCommand(comandos[i]) == true)
      {
         ncomandos--;
         continue;
      }

      char* Ejecutable = comandos[i].argv[0];
      //se crea proceso hijo
      int RetExec = 0;
      int pid = fork();

      if(pid == 0)
      {
         if(comandos[i].Redireccion == true)
         {
            int fichero = open(comandos[i].archivoRedireccion, O_RDWR | O_CREAT, 0666 | O_WRONLY | O_TRUNC);

            if(fichero == -1)
            {
               error();
               exit(0);
            }
            dup2(fichero, 1);
            dup2(fichero, 2);
         }
         RetExec = execvp(Ejecutable, comandos[i].argv);
         if(RetExec == -1)
         {
            error();
         }
         exit(0);
      }
   }

   for(i = 0; i < ncomandos; i++)
   {
      wait(0);
   }

   return 0;
}

bool builtInCommand(struct comando comandos)
{
   char* Ejecutable = comandos.argv[0];
   bool isBuiltInCommand = true;
   if(strcmp(Ejecutable, "exit") == 0)
   {
      salir(comandos.argc);
   }
   else if( strcmp(Ejecutable, "cd") == 0)
   {
      cd(comandos.argc, comandos.argv);
   }
   else
   {
      isBuiltInCommand = false;
   }
   return isBuiltInCommand;
}

void salir(int argc)
{
   if(argc == 1)
   {
      free(comandos);
      free(linea);
      fclose(Fichero);
      exit(0);
   }
   error();
}

void cd(int argc, char* argv[])
{
   if(argc != 2)
   {
      error();
   }
   else if(chdir(argv[1]) == -1)
   {
      error();
   }
}

void error()
{
   fprintf(stderr, "%s", mensaje_error);
}

int contarArgumentos(char* linea)
{
   int i;
   int argc = 0;
   int tamano = strlen(linea);
   bool habiaEspacio = false;
   bool hayEspacio = false;

   if(tamano == 0)
   {
      return 0;
   }

   if(linea[0] == ' ' || linea[0] == '\t')
   {
      hayEspacio = true;
   }
   if(hayEspacio == false)
   {
      argc++;
   }

   hayEspacio = false;
   i = 0;

   while(i < tamano)
   {
      if(linea[i] == ' ' || linea[i] == '\t')
      {
         hayEspacio = true;
      }


      if(hayEspacio == false && habiaEspacio == true)
      {
         argc++;
         habiaEspacio = false;
      }

      habiaEspacio = hayEspacio;
      hayEspacio = false;
      
      i += sizeof(linea[i]);

   }
   return argc;
}
