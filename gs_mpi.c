#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
// faltaria incluir string.h para poder usar memcpy

#define MAX_ITER 100
#define MAX 100 //maximum value of the matrix element
#define TOL 0.000001

// Variables globales para tener en cuenta el tiempo
double s_time;
double e_time;
int numnodes;

// Generate a random float number with the maximum value of max
float rand_float(int max){
  return ((float)rand()/(float)(RAND_MAX)) * max;
}


// Allocate 2D matrix
void allocate_init_2Dmatrix(float ***mat, int n, int m){
  int i, j;
  *mat = (float **) malloc(n * sizeof(float *));
  for(i = 0; i < n; i++) {
    (*mat)[i] = (float *)malloc(m * sizeof(float));
    for (j = 0; j < m; j++)
      (*mat)[i][j] = rand_float(MAX);
  }
}

// solver
void solver(float ***mat, int n, int m){
  float diff = 0, temp;
  int done = 0, cnt_iter = 0, i, j, myrank, rows, strt;

  // Empezar a contar el tiempo
  s_time = MPI_Wtime();

  while (!done && (cnt_iter < MAX_ITER)){
    diff = 0;

    // Crear una copia de las filas de la matriz que se van a mandar
    // Si queremos dividir la matriz en partes iguales para cada proceso
    // *cmat = (float **) malloc( n/n_nodos * sizeof(float *) );
    // despues tocaría copiar el trozo de matriz que toca en la copia
    // numero de elementos que se van a pasar numero_filas * (columnas)m * sizeof(float)
    // memcpy(&cmat, &mat[myrank], (n/n_nodos) * m * sizeof(float));

    // Aqui ya se podria usar una funcion para mandar la copia de la matriz y
    // asi operar solo lo que le toque a cada uno

    // He visto otra forma de hacer las cosas en la que en vez de hacer copias, el master le manda a los clientes dos mensajes
    // uno con el upper bound de desde donde van a empezar a procesar y otro con el lower bound, igual es mas eficiente que tener
    // que ir copiando trocitos de la matriz

    // para esto primero se comprueba que el rango es mayor que 0 y con eso ya recibe los valores, cuando termina de operar los manda de vuelta y a correr

    // proceso master
    if (myrank == 0) {
      // dividir las filas equitativamente, sin contar primera y ultima
      rows = (n - 2) / (numnodes - 1);
      int strt = 0;

      // Enviar datos a otros nodos
      for (int otherrank = 1; otherrank < numnodes; otherrank++) {
        // Cada proceso se tiene que llevar su fila, la anterior y la siguiente
        /*
            buf: principio del mensaje: 
                 donde tiene que empezar a calcular
                 el numero de filas que tiene que calcular
                 las filas de la matriz en las que va a operar
            count: elementos
            datatype: int y float
            dest: destino: otherrank
            tag: 1
            comm: MPI_COMM_WORLD
        */
        // En que punto de la matriz va a empezar a calcular cosas
        MPI_Send(&strt, 1, MPI_INT, otherrank, 1, MPI_COMM_WORLD);
        // Cuantas filas tiene que operar
        MPI_Send(&rows, 1, MPI_INT, otherrank, 1, MPI_COMM_WORLD);
        // Las filas de la matriz que va a utilizar para solapar cosas
          // Se le manda desde la fila anterior porque necesita los datos de la fila anterior aunque no los vaya a modificar
        MPI_Send(&mat[strt - 1][0], (rows + 1) * m, MPI_FLOAT, otherrank, 1, MPI_COMM_WORLD);
        //Calcular donde va a empezar a calcular el siguiente nodo
        strt = strt + rows;
      }
    }

    // Trabajo en los nodos
    if (myrank > 0) {
      // obtener el número de filas que le toca operar
      /*
        buf: principio del mensaje: la fila anterior a la que le toca calcular
        count: elementos: rows
        datatype: float
        dest: destino: otherrank
        tag: 1
        comm: MPI_COMM_WORLD
        status: error status
      */
      //MPI_Recv(&strt, &rows, MPI_INT, );
      // Recibir los datos
      // Realizar los cálculos que le toca a cada parte
      // dos fors anidados que calculen los resultados para los datos que le toca computar
    }

    // Codigo orignial
    /*for (i = 1; i < n - 1; i++)
      for (j = 1; j < m - 1; j++) {
        temp = (*mat)[i][j];
        (*mat)[i][j] = 0.2 * ((*mat)[i][j] + (*mat)[i][j - 1] + (*mat)[i - 1][j] + (*mat)[i][j + 1] + (*mat)[i + 1][j]);
        diff += abs((*mat)[i][j] - temp);
      }*/

    // se recibe aqui o despues de comprobar la diferencia?
    // igual renta mas despues de haber terminado el numero total de iteraciones, porque asi se mandan menos mensajes aunque igual sea algo mas pesado con lo cual
    // dependeria menos de la red, perderiamos menos tiempo en eso y seriamos mas rapidos
    // usamos despues alguna barrera?

    if (diff/n/n < TOL)
      done = 1;
    cnt_iter ++;

  }

  // Contar el tiempo cuando termina
  e_time = MPI_Wtime();

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  if (myrank == 0) {
    if (done)
      printf("Solver converged after %d iterations\n", cnt_iter);
    else
      printf("Solver not converged after %d iterations\n", cnt_iter);
  }
}

int main(int argc, char *argv[]) {
  int np, myrank, n, communication;
  float **a;

  numnodes = np;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (argc < 3) {
    if (myrank == 0) {
      printf("Call this program with two parameters: matrix_size communication \n");
      printf("\t matrix_size: Add 2 to a power of 2 (e.g. : 18, 1026)\n");
      printf("\t communication:\n");
      printf("\t\t 0: initial and final using point-to-point communication\n");
      printf("\t\t 1: initial and final using collective communication\n");

   }
    MPI_Finalize();
    exit(1);
  }

  n = atoi(argv[1]);
  communication =  atoi(argv[2]);
  printf("Matrix size = %d communication = %d\n", n, communication);


  switch (communication){
  case 0: {
    if (myrank == 0)
      allocate_init_2Dmatrix(&a, n, n);
    // p2p communication for scattering the matrix
    break;
  }
  case 1: {
    if (myrank == 0)
      allocate_init_2Dmatrix(&a, n, n);
    // collective communication for scattering the matrix
    break;
  }
  }

  // parallelize the solver inside
  solver(&a, n, n);


  switch (communication){
  case 0: {
    // p2p communication for scattering the matrix
    break;
  }
  case 1: {
    // collective communication for gathering the matrix
    break;
  }

  }
  MPI_Finalize();
  return 0;
}
