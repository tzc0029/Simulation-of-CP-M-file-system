#include "cpmfsys.h"
#include "diskSimulator.h"
#include <ctype.h>

/**
 * COMP 7500 Project 4
 * @author: Tianshi Che 
 * @date: Apr 26, 2019
 */


bool freeList[NUM_BLOCKS];

DirStructType *mkDirStruct(int index, uint8_t *e) {
   char cha;
   DirStructType *dir;
   dir = malloc(sizeof(DirStructType));
   
   dir->status = (e+index*EXTENT_SIZE)[0];
   int offset = 1;
   
   // read in names
   do {
      cha = (e+index*EXTENT_SIZE)[offset];
      (dir->name)[offset-1] = cha;
      offset++;
   } while (offset < 9 && cha != ' ');
  // string terminator depends on the length of the name
   if (cha == ' ') { 
      dir->name[offset-2] = '\0';
   } 
   else { 
      dir->name[offset-1] = '\0'; 
   }

   offset = 9;
   // read in extensions
   do  {
      cha = (e+index*EXTENT_SIZE)[offset];
      dir->extension[offset-9] = cha;
      offset++;
   } while (offset < 12 && cha != ' ');
   
   if (cha  == ' ') {
      dir->extension[offset-10] = '\0';
   } 
   else {
      dir->extension[offset-9] = '\0';
   }
   // rest of the struct
   dir->XL = (e+index*EXTENT_SIZE)[12];
   dir->BC = (e+index*EXTENT_SIZE)[13];
   dir->XH = (e+index*EXTENT_SIZE)[14];
   dir->RC = (e+index*EXTENT_SIZE)[15];
   memcpy(dir->blocks, e+index*EXTENT_SIZE+16, 16);

   return dir;
}


void writeDirStruct(DirStructType *dir,uint8_t index, uint8_t *e) {
   (e+index*EXTENT_SIZE)[0] = dir->status;
   int offset = 1;
   // read names
   while(dir->name[offset-1] != '\0' && dir->name[offset-1] != '.') {
      (e+index*EXTENT_SIZE)[offset] = dir->name[offset-1];
      offset++;
   }
  // set all block to blank
   for(; offset<9;offset++) {  
      (e+index*EXTENT_SIZE)[offset] = ' ';
   }
  // read file extension
   int extcounter = 0;
   while(dir->extension[extcounter] != '\0') {
      (e+index*EXTENT_SIZE)[offset] = dir->extension[extcounter];
      offset++;  
      extcounter++;
   }
  // set all block to blank
   for(;offset<12;offset++) {
      (e+index*EXTENT_SIZE)[offset] = ' ';
   }
  // rest of the struct
   (e+index*EXTENT_SIZE)[12] = dir->XL;
   (e+index*EXTENT_SIZE)[13] = dir->BC;
   (e+index*EXTENT_SIZE)[14] = dir->XH;
   (e+index*EXTENT_SIZE)[15] = dir->RC;
   memcpy(e+index*EXTENT_SIZE+16,dir->blocks,16);

}


void makeFreeList() {
   uint8_t buffer[BLOCK_SIZE];
   DirStructType *dir;
   int i;
   // initialization
   for (i = 1; i < NUM_BLOCKS; i++) {
      freeList[i] = true;
   }
   
   // read the directory block from the disk
   blockRead(buffer,(uint8_t) 0);
   for (i = 0; i < BLOCK_SIZE / EXTENT_SIZE; i++) {
      dir = mkDirStruct(i,buffer);
    // check if the block is in use
      if (dir->status != 0xe5) {
         for (int j = 0; j < 16; j++) {
            if (dir->blocks[j] != 0) {
               int k = dir->blocks[j];
               freeList[k] = false;
            }
         }
      }
   }
   // make sure block 0 is never free
   freeList[0] = false;
}


void printFreeList() {
   printf("FREE BLOCK LIST (* means in-use):\n");
   for (int i = 0; i < 16; i++) {
      printf("%3x: ",i * 16);
      for (int j = 0; j < 16; j++) {
         // block is free
         if (freeList[i*16+j] == true) {  
            printf(". ");
         } 
         // block in use
         else {  
            printf("* ");
         }
      }        
      printf("\n");
   }
}

int findExtentWithName(char *name, uint8_t *block0) {
   char namebuffer[9];
   char extbuffer[4];
   int i = 0;
   // check if name is legal
   if(checkLegalName(name) == false) {
      return -1;
   }     
   // fill in name buffer
   while (name[i] != '\0' && name[i] != '.'&& i < 8) {
      namebuffer[i] = name[i];
      i++;
   }
   namebuffer[i] = '\0'; 
   // if name more than 8 characters
   if (i == 8 && name[i] != '.') {
      if(name[i] != '\0')
      {
         return -1;
      }
   } 
   // check extension
   else if (name[i] == '.') {
      int extcounter = 0; 
      i++;
      while ( name[i] != '\0' && extcounter  < 3) {
         // cannot have anything other than numbers and upper lower case characters(from ASCII table)
         if (name[i] < 48 || (name[i] > 57 && name[i] < 65) || (name[i] > 90 && name[i] < 97) || (name[i] > 122)) {
            return -1;
         }
         extbuffer[extcounter] = name[i];
         i++;
         extcounter++;
      }
      extbuffer[extcounter] = '\0';
      // if extention is too long
      if (extcounter == 3 && name[i] != '\0') {
         return -1;
      }
   } 
    
   else { 
      extbuffer[0] = '\0';
   }
   
  // search through the directory sector for a file with right name, ext
   for (i = 0; i < BLOCK_SIZE/EXTENT_SIZE; i++) {
      DirStructType *dir;
      dir = mkDirStruct(i,block0);
      if (!strcmp(dir->name, namebuffer) && !strcmp(dir->extension, extbuffer)) {
         // check if unused
         if (dir->status == 0xe5) {
            return -1; 
         }
         return i;
      }
   }
   // file not found
   return -1; 
}

bool checkLegalName(char *name) {
   int i = 0;
  
   while ( name[i] != '\0' && name[i] != '.'&& i < 8) {
      // cannot have anything other than numbers and upper lower case characters(from ASCII table)
      if (name[i] < 48 || (name[i] > 57 && name[i] < 65) || (name[i] > 90 && name[i] < 97) || (name[i] > 122)) {
         return false;
      }
      i++;
   }
   // if name more than 8 characters
   if (i == 8 && name[i] != '.' && name[i] != '\0') {  
      return false;
   } 
   else if (name[i] == '.') { // need to process the extension
      i++ ;  //  get past the . in name
      int extcounter = 0;
    // no blanks, control chars, or punctuation in the extension
      while ( name[i] != '\0' && extcounter  < 3) {
         if (name[i] < 48 || (name[i] > 57 && name[i] < 65) ||
         (name[i] > 90 && name[i] < 97) || (name[i] > 122)) {
            return false; // illegal character in ext
         }
         i++;
         extcounter++;
      }
      // if extention is too long
      if (extcounter == 3 && name[i] != '\0') {
         return false;
      }
   }  
   return true;
}


void cpmDir() {
   DirStructType *dir;
   int filesize = 0;  
   int i = 0;
   uint8_t buffer[BLOCK_SIZE];
   // read the directory block from the disk
   blockRead(buffer,(uint8_t) 0);
  
   printf("DIRECTORY LISTING:\n");
   for (;i < BLOCK_SIZE / EXTENT_SIZE; i++) {
      dir = mkDirStruct(i, buffer);
      filesize = 0;
      
      if (dir->status != 0xe5) { 
         for (int j = 0; j<16; j++) {
            // how many blocks fully used
            if (dir->blocks[j] != 0) {
               filesize = filesize + BLOCK_SIZE;
            }
         }
         filesize = filesize - BLOCK_SIZE + ((int) dir->RC)*128 + (int)dir->BC;
         printf("%s.%s %d\n", dir->name, dir->extension, filesize);
      }
   }
}

int cpmRename(char *oldName, char *newName)
{
   uint8_t *blockBuffer=malloc(1024);
   blockRead(blockBuffer,0);
   int extentIndex = findExtentWithName(oldName, blockBuffer);
   // if name is not valid or file does not exist
   if(!checkLegalName(oldName) || !checkLegalName(newName) || 
      extentIndex == -1 || findExtentWithName(newName,blockBuffer) != -1) {
      return -1;
   }
   
   else {
   	// if dot present in newName
      if (strchr(newName, '.') != NULL)
      {
         int dotIndex=(int)(strchr(newName, '.') - newName);
         int startIndex = extentIndex * 32 + 1;
         int extentionIndex = extentIndex*32+9;
         //copy new name
         for(int i = 0; i < dotIndex; i++)
         {
            blockBuffer[startIndex] = newName[i];
            startIndex++;
         }
         // copy new extention
         for(int j = dotIndex + 1; j < strlen(newName); j++) {
            blockBuffer[extentionIndex] = newName[j];
            extentionIndex++;
         }
         
         for(int k = startIndex; k < extentIndex * 32 + 9; k++) {
            blockBuffer[k]=0;
         }
         
         for(int l = extentionIndex; l < extentIndex * 32 + 12; l++) {
            blockBuffer[l]=0;
         }
         
         blockWrite(blockBuffer, (uint8_t) 0);
         return 0;
      }
      // if dot not present
      else {
         int startIndex = extentIndex * 32 + 1;
         for (int i = 0; i < strlen(newName); i++)
         {
            blockBuffer[startIndex] = newName[i];
            startIndex++;
         }
         blockWrite(blockBuffer, (uint8_t) 0);
         return 0;
      }
   }
}

int cpmDelete(char *name) {
   uint8_t block0[1024];
   DirStructType *dir;
  // read the directory from the disk
   blockRead(block0, (uint8_t) 0);
   int extentIndex = findExtentWithName(name, block0);
   // check if file exist
   if (extentIndex < 0 ) {
      return extentIndex; 
   } 
   else {
      // mark used blocks as used in freeList
      dir = mkDirStruct(extentIndex, block0);
      for (int i = 0; i < 16; i++) {
         if (dir->blocks[i] != 0) {
            freeList[dir->blocks[i]] = true;
         }
      }
      // mark as unused
      block0[extentIndex * EXTENT_SIZE] = 0xe5;
      blockWrite(block0, (uint8_t) 0);
      return 0;
   }
}