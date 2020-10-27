

int byteDestuff(unsigned char* data, int size) {

  char aux[size+5];

  strcpy(aux,data);
  
  int finalSize=4;

  for(int i = 4; i < size+5; i++){

    if(aux[i] == 0x7d ) {
        if(aux[i+1] == 0x5d){
            data[finalSize]=0x7d;
        }
        else if(aux[i+1] == 0x5e){
            data[finalSize]=0x7e;
        }
      finalSize ++;
      i++;
    }
    else{
      data[finalSize] = aux[i];
      finalSize++;
    }
  }

  return finalSize;
}