
/*********************************************************************************
 *  This code developed at Argonne National Laboratory
 *  Author: Timothy Madden
 *  tmadden@anl.gov
 *
 *  March, 2015
 *  This file was copied from the detectorMPI program's directories
 *  in order to simplify this plugin.
 *
 *********************************************************************************/



#include <stdio.h>
#include "pipebinaryformat.h"



char pipeBinaryFormat::zzz[]="zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";



pipeBinaryFormat::pipeBinaryFormat()
{
}



int pipeBinaryFormat::checkErrors(int nread)
{
    //return 1 for break out of here...
    return(1);
}


 int pipeBinaryFormat::readDataBlock(
       FILE *fp,
         unsigned short *memdata,
         int *size_x,
         int *size_y,
         int *num_pixels,
         int *frame_number,
         int *inp_img_cnt,
         int *error_code)
 {

       // read an image of this format:
     // unsigned shorts litte endian
     // start with 64 'Z' bytes
     // then have x size as a int, num of x puixels
     // y size as a int. num of y pixels
     //then bunch of shorts for the imate x*y pixels


     char zzz[64];
     int nread;
    int k;
    int stat = 0;


    //
    // Seawrch for zzzzzzz's , 64 zs in a row. we read 64 bytes, see if we have any z's.
    // if all z's we have found the beginning of image.
    // of some zs we read the rest of the 64 z's and hope we get them
   // if we dont find z's we seawrch until we find them
    //


    bool all_zzzs=false;

     bool is_found_zz=false;

     while(!is_found_zz)
     {
         //read in 64 bytes, search for 1 z.
         nread = fread(zzz,1,64,fp);

         if (nread!=64)
             if (checkErrors(nread)!=0)
                 return(-1);


         //
         // Search for 64 z's
         //
         int first_z = -1;
         int N_zs=0;

         all_zzzs=true;
         for (k=0;k<64;k++)
         {
             if (first_z == -1 && zzz[k]=='z')
                     first_z=k;

             if (zzz[k]!='z')
                 all_zzzs=false;

             if (zzz[k]=='z')
                 N_zs++;

         }

         // we found 64 z's in a row so we are lined iup w/ data. now read the header and image data
         if (all_zzzs)
             is_found_zz=true;
         // we found some number of Z's, N_zs. Find the rest of them
         else if (first_z>0)
         {
             // num z's we found.
             int N_rest_of_zs=64-N_zs;

             nread = fread(zzz,1,N_rest_of_zs,fp);

             if (nread!=N_rest_of_zs)
                 if (checkErrors(nread)!=0)
                     return(-1);



             all_zzzs=true;
             for (k=0;k<N_rest_of_zs;k++)
             {


                 if (zzz[k]!='z')
                     all_zzzs=false;

             }
             is_found_zz=all_zzzs;

         }


     }


     //
     // If we are here we have found the beginning of an image.
     // read x and y size as ints, then read in the image.
     //


     nread=fread(size_x,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);;


     nread=fread(size_y,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);

     nread=fread(num_pixels,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);




     nread=fread(frame_number,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);

     nread=fread(inp_img_cnt,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);

     nread=fread(error_code,4,1,fp);

     if (nread!=1)
         if (checkErrors(nread)!=0)
             return(-1);












     int n_s_pixels = *num_pixels;


     int total_read=0;
     unsigned short *ptr = memdata;
     do{
         nread= fread(ptr,sizeof(short),1024,fp);

         if (nread!=1024)
             if (checkErrors(nread)!=0)
                 return(-1);

         ptr+=nread;
         total_read+=nread;

     }while(total_read<n_s_pixels);




         return(stat);

 }

 int pipeBinaryFormat::writeDataBlock(
         FILE *fp,
         unsigned short *memdata,
         int size_x,
         int size_y,
         int num_shorts,
         int frame_number,
         int inp_img_cnt,
         int error_code)
 {
     //
     //write image to pipe
     //

  


     //write zzz's
     fwrite(pipeBinaryFormat::zzz,1,64,fp);
     //write image size x,y
     fwrite(&size_x,4,1,fp);
     fwrite(&size_y,4,1,fp);
     fwrite(&num_shorts,4,1,fp);

     fwrite(&frame_number,4,1,fp);
     fwrite(&inp_img_cnt,4,1,fp);
     fwrite(&error_code,4,1,fp);
     //
     //write image data
    //

     int numpix = num_shorts;
     int nwrite = 0;
     int total_write = 0;
     int num_to_write = 1024;
     do
     {
         if ((numpix-total_write) > 1024) {
            num_to_write = 1024;
         }
         else {
            num_to_write = numpix-total_write;
         }

         nwrite = fwrite(memdata+total_write, 2,num_to_write,fp);
         total_write += nwrite;
     }while(total_write<numpix);

     fflush(fp);

 }
