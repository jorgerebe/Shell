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
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_ARGUMENTOS 64

//struct que representa a un comando
struct comando
{
   int argc;
   char* argv[MAX_ARGUMENTOS];
   bool Redireccion;
   char* archivoRedireccion;
};

//funciones
void leeComandos(bool);
int ncomandos(char* linea);
bool esRedireccion(char*);
char* parseaArchivo();
void parsea(char*[], int);
int ejecuta(struct comando*, int);
bool builtInCommand(int);
void salir(int);
void cd(int, char*[]);
int contarArgumentos(char*);
void error();

//Variables globales
FILE* Fichero;
struct comando* comandos;
char* linea;
char mensaje_error[] = "An error has occurred\n";	//mensaje de error común a todos los errores

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

   if(argc == 2)
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

//función que se encarga de leer los comandos
void leeComandos(bool interactivo)
{
   size_t valor;
   ssize_t tamano;

   linea = NULL;
   valor = 0;

   //bucle principal del programa
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

	 //se cambia el retorno de carro del final de la línea por el carácter terminador de línea
         linea[strlen(linea)-1] = '\0';
         numeroComandos = ncomandos(linea);

	 //si no se han introducido posibles comandos en la línea, se muestra un error y se va a la siguiente iteración
         if(numeroComandos == 0)
         {
            if(strchr(linea, '&') != NULL)
            {
               error();
            }
            free(linea);
            continue;
         }

         //se crea el array de punteros a char en el que se almacenará cada comando

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

         //se parsean los comandos

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

//funcion para contar el numero de comandos en una linea
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

//funcion para parsear cada linea de comandos, separar en distintos comandos si los hubiera (comandos paralelos), verificar si un comando tiene redireccion y obtener dicho archivo en su caso
void parsea(char* comandosSeparados[], int ncomandos)
{
   int i;
   int j;
   char delimRedireccion[] = ">";
   char delimArgv[] = " \t";
   char* palabra;

   //se reserv espacio para el array de struct comandos
   comandos = (struct comando*)malloc(ncomandos * sizeof(struct comando));

   //se asignan los datos de cada comando en su correspondiente struct comando
   i = 0;
   while(i < ncomandos)
   {
      //si no se encuentran argumentos en el comando, se resta 1 al numero de comandos válidos y se va a la siguiente iteración
      if(contarArgumentos(comandosSeparados[i]) == 0)
      {
         ncomandos--;
         continue;
      }

      //EXAMEN MODIFICACION 2//
      if(strstr(comandosSeparados[i], "$>") != NULL)
      {

         char* comando = strtok(comandosSeparados[i], "$>");
         char* archivo = strtok(NULL, "$>");
         char* nuevoComando = (char*)malloc(128*sizeof(char));
         int index;
         int cuenta = 0;

         char* time = "time -p --quiet ";

         for(index = 0; index < strlen(time); index++)
         {
            nuevoComando[cuenta+index] = time[index];
         }

         cuenta += index;

         for(index = 0; index < strlen(comando); index++)
         {
            nuevoComando[cuenta+index] = comando[index];
         }

         cuenta += index;

         nuevoComando[cuenta] = '>';

         cuenta++;

         for(index = 0; index < strlen(archivo); index++)
         {
            nuevoComando[cuenta+index] = archivo[index];
         }

         cuenta += index;

         comandosSeparados[i] = nuevoComando;

      }//FIN EXAMEN MODIFICACION 2///

      //se comprueba si el comando contiene redireccion

      comandos[i].Redireccion = esRedireccion(comandosSeparados[i]);

      if(comandos[i].Redireccion == true)
      {
         //se comprueba que, si el comando pretende hacer una redirección, existe el nombre de un posible archivo donde hacerla
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

      //se parsean los argumentos y se asignan en su correspondiente posición del array argv en el struct comando iésimo

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

   //si hay al menos un comando válido, se ejecuta

   if(ncomandos > 0)
   {
      ejecuta(comandos, ncomandos);
   }

}

//funcion para contar los argumentos de un comando

int contarArgumentos(char* comando)
{
   int i;
   int argc = 0;
   int tamano = strlen(comando);
   bool habiaEspacio = false;
   bool hayEspacio = false;

   if(tamano == 0)
   {
      return 0;
   }

   if(comando[0] == ' ' || comando[0] == '\t')
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
      if(comando[i] == ' ' || comando[i] == '\t')
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

      i += sizeof(comando[i]);

   }
   return argc;
}

//función que comprueba si un comando pretende hacer una redirección
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

//función que obtiene el archivo de la redirección de un comando
char* parseaArchivo()
{
   char* palabra = strtok(NULL, " \t");
   if(strtok(NULL, " \t") != NULL)
   {
      palabra = NULL;
   }

   return palabra;
}

//funcion para ejecutar los comandos
int ejecuta(struct comando* comandos, int ncomandos)
{
   int i;

   for(i = 0; i < ncomandos; i++)
   {

      if(builtInCommand(i) == true)
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

//funcion que identifica si el comando es un builtInCommand y, si lo fuera, lo ejecuta. Recibe como argumento
//el indice del array de comandos en el que se encuentra el comando que hay que ver si contiene un builtInCommand
//le paso el índice en lugar del struct comando porque entonces no puedo modificar los argumentos en el caso del ls para añadir el color
//sí que podría haber pasado un puntero al struct comando, pero el contenido de mi array de struct comando no son punteros (en este momento)

bool builtInCommand(int indice)
{
   char* Ejecutable = comandos[indice].argv[0];
   bool isBuiltInCommand = true;
   int i;

   if(strcmp(Ejecutable, "exit") == 0)
   {
      salir(comandos[indice].argc);
   }
   else if( strcmp(Ejecutable, "cd") == 0)
   {
      cd(comandos[indice].argc, comandos[indice].argv);
   }
   else if(strcmp(Ejecutable, "ls") == 0)
   {
      for(i = 0; i < comandos[indice].argc; i++)
      {
         if(strncmp(comandos[indice].argv[i], "--color", strlen("--color")) == 0)
         {
            return false;
         }
      }

      comandos[indice].argv[comandos[indice].argc] = "--color=auto";
      comandos[indice].argc++;
      isBuiltInCommand = false;
   }//INICIO MODIFICACION 1 EXAMEN
   else if(strcmp(Ejecutable, "pwd") == 0)
   {
      //declaro las variables aqui en lugar del principio de la funcion para mayor claridad
      size_t tamano = 256; //tamaño maximo que podra tener el pathname del directorio actual
      char* nombreDirectorio = (char*)malloc(sizeof(tamano));
      if(getcwd(nombreDirectorio, tamano) == NULL)
      {
         error();
         return isBuiltInCommand;
      }

      printf("%s\n", nombreDirectorio);


   }//FIN MODIFICACION 1 EXAMEN
   else
   {
      isBuiltInCommand = false;
   }
   return isBuiltInCommand;
}

//funcon para salir del UVaShell. Libera la memoria ocupada por el array de struct comando, por la línea y cierra el fichero si la llamada a exit es correcta
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

//funcion para el builtInCommand cd

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

//funcion que muestra el mensaje de error, común a todos los errores
void error()
{
   fprintf(stderr, "%s", mensaje_error);
}
