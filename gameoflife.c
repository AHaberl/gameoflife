#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <omp.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))
#define TRUE 1
#define FALSE 0

void show(unsigned* currentfield, int w, int h) {
  printf("\033[H");
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    printf("\033[E");
  }
  fflush(stdout);
}


float convert2BigEndian( const float inFloat )
{
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat    = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

void writeVTK(unsigned* currentfield, int w, int h, int t, int threadNum, char* prefix) {
  char name[1024] = "\0";
  sprintf(name, "out/%s_%d%d.vtk", prefix, t, threadNum);
  FILE* outfile = fopen(name, "w");

  /*Write vtk header */                                                           
  fprintf(outfile,"# vtk DataFile Version 3.0\n");       
  fprintf(outfile,"frame %d\n", t);     
  fprintf(outfile,"BINARY\n");     
  fprintf(outfile,"DATASET STRUCTURED_POINTS\n");     
  fprintf(outfile,"DIMENSIONS %d %d %d \n", w, h, 1);        
  fprintf(outfile,"SPACING 1.0 1.0 1.0\n");//or ASPECT_RATIO                            
  fprintf(outfile,"ORIGIN 0 0 0\n");                                              
  fprintf(outfile,"POINT_DATA %d\n", h*w);                                    
  fprintf(outfile,"SCALARS data float 1\n");                              
  fprintf(outfile,"LOOKUP_TABLE default\n");         
 
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float value = currentfield[calcIndex(w, x,y)]; // != 0.0 ? 1.0:0.0;
      value = convert2BigEndian(value);
      fwrite(&value, 1, sizeof(float), outfile);
    }
  }
  fclose(outfile);
}


int calcIndexAbsolute(int x, int y, int w, int h){

  return calcIndex(w, (x+w) % w, (y+h)%h);
}


int noAlliveFields(int x, int y, int w,int h, unsigned* currentField){
  int noFields = 0;
  for(int i = -1; i <= 1 ; i++){
    for(int j = -1; j <= 1; j++){
       int index = calcIndexAbsolute(x+i, y+j, w, h);
       if(currentField[index]) noFields++;     
    }  
  }
  
  return noFields;
}

 
int evolve(unsigned* currentfield, unsigned* newfield, int w, int h, int t) {
int changes = FALSE;

#pragma omp parallel for firstprivate(t)
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int index = calcIndex(w,x,y);
      int noAllive = noAlliveFields(x, y, w, h, currentfield);
      if(currentfield[index]) noAllive--;
      
      if((noAllive == 3) || (noAllive == 2 && currentfield[index])) {
	changes = TRUE;
	newfield[index] = TRUE;
      } else {
        newfield[index] = FALSE;

      }
	int threadNum = omp_get_thread_num();
	writeVTK(currentfield, w, h, t, threadNum, "output");
    }
  }
//TODO if changes == 0, the time loop will not run! 
  return changes;
}


void filling(unsigned* currentfield, int w, int h) {
  for (int i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
  currentfield[calcIndexAbsolute(2, 2, w, h)] = TRUE;
  currentfield[calcIndexAbsolute(2, 3, w, h)] = TRUE;
  currentfield[calcIndexAbsolute(2, 4, w, h)] = TRUE;

}



void filling_ascii(unsigned* currentfield, int width, int height, char filename[50]) {

    FILE *filePointer;
    filePointer = fopen(filename, "r");

    if(filePointer == NULL){
        perror("Error while opening input file\n");
        exit(EXIT_FAILURE);
    }

    char line[width+1];
    int index = 0;
    while (fgets(line, sizeof(line), filePointer)) {
      //  printf("%d : %s",i, line); 
        for(int j=0; j<sizeof(line)-1; j++){            
            if(line[j] == 49){
                currentfield[index] = 1;
            } else {
                currentfield[index] = 0;

            }
            index++;
        }     
    }
   
    fclose(filePointer);
    

}

 
void game(int w, int h, int timesteps, char inputFile[50]) {
  unsigned *currentfield = calloc(w*h, sizeof(unsigned));
  unsigned *newfield     = calloc(w*h, sizeof(unsigned));
  
  //filling(currentfield, w, h);
  filling_ascii(currentfield, w, h, inputFile);

  for (int t = 0; t < timesteps; t++) {
// TODO consol output
   show(currentfield, w, h);
    
    int changes = evolve(currentfield, newfield, w, h, t);
    if (changes == 0) {
    	sleep(3);
    	break;
    }
    
    usleep(200000);

    //SWAP
    unsigned *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
}
 
int main(int c, char **v) {
     
  int w = 0, h = 0, timesteps = 10;
  if (c > 1) w = atoi(v[1]); ///< read width
  if (c > 2) h = atoi(v[2]); ///< read height
  if (c > 3) timesteps = atoi(v[3]);
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height
    
    char inputFile[50];
    puts("insert file name\n");
    scanf("%s", inputFile);
    
  game(w, h, timesteps, inputFile);
}
