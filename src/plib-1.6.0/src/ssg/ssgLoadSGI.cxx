/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id$
*/


#include "ssgLocal.h"

/*eliminated by J. Nathan Matias
static FILE          *image_fd ;
static char           image_fname [ 512 ] ;
static int            isSwapped ;
static unsigned char *rle_temp ;*/


/* Some magic constants in the file header. */

#define SGI_IMG_MAGIC           0x01DA
#define SGI_IMG_SWABBED_MAGIC   0xDA01   /* This is how it appears on a PC */
#define SGI_IMG_VERBATIM        0
#define SGI_IMG_RLE             1

class ssgSGIHeader
{
public:    /* Yuk!  Need to hide some of this public stuff! */
  unsigned short magic ;
  int            max ;
  int            min ;
  int            colormap ;
  char           type ;
  char           bpp ;
  unsigned int  *start ;
  int           *leng ;
  unsigned short dim ;
  unsigned short xsize ;
  unsigned short ysize ;
  unsigned short zsize ;
  int           tablen ;
  /* added by J. Nathan Matias */
  FILE          *image_fd;
  char          image_fname [ 512 ];
  int           isSwapped;
  unsigned char *rle_temp;
  bool          loadSGI_bool;


  ssgSGIHeader () ;
  ssgSGIHeader(const char *fname, ssgTextureInfo* info ); 
  ~ssgSGIHeader () ;

  bool openFile(const char *fname);
  void makeConsistant () ;
  void getRow   ( unsigned char *buf, int y, int z ) ;
  void getPlane ( unsigned char *buf, int z ) ;
  void getImage ( unsigned char *buf ) ;
  void readHeader () ;
  
protected:
  void swab_short ( unsigned short *x );
  void swab_int ( unsigned int *x );
  void swab_int_array ( int *x, int leng );
  unsigned char readByte ();
  unsigned short readShort ();
  unsigned int readInt ();
   
} ;


#ifdef SSG_LOAD_SGI_SUPPORTED

void ssgSGIHeader::makeConsistant ()
{
  /*
    Sanity checks - and a workaround for buggy RGB files generated by
    the MultiGen Paint program because it will sometimes get confused
    about the way to represent maps with more than one component.

    eg   Y > 1, Number of dimensions == 1
         Z > 1, Number of dimensions == 2
  */

  if ( ysize > 1 && dim < 2 ) dim = 2 ;
  if ( zsize > 1 && dim < 3 ) dim = 3 ;
  if ( dim < 1 ) ysize = 1 ;
  if ( dim < 2 ) zsize = 1 ;
  if ( dim > 3 ) dim   = 3 ;
  if ( zsize < 1 && ysize == 1 ) dim = 1 ;
  if ( zsize < 1 && ysize != 1 ) dim = 2 ;
  if ( zsize >= 1 ) dim = 3 ;

  /*
    A very few SGI image files have 2 bytes per component - this
    library cannot deal with those kinds of files. 
  */

  if ( bpp == 2 )
  {
    ulSetError ( UL_FATAL, "ssgLoadTexture: Can't work with SGI images with %d bpp", bpp ) ;
  }

  bpp = 1 ;
  min = 0 ;
  max = 255 ;
  magic = SGI_IMG_MAGIC ;
  colormap = 0 ;
}


void ssgSGIHeader::swab_short ( unsigned short *x )
{
  if ( isSwapped )
    *x = (( *x >>  8 ) & 0x00FF ) | 
         (( *x <<  8 ) & 0xFF00 ) ;
}

void ssgSGIHeader::swab_int ( unsigned int *x )
{
  if ( isSwapped )
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
}

void ssgSGIHeader::swab_int_array ( int *x, int leng )
{
  if ( ! isSwapped )
    return ;

  for ( int i = 0 ; i < leng ; i++ )
  {
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
    x++ ;
  }
}


unsigned char ssgSGIHeader::readByte ()
{
  unsigned char x ;
  fread ( & x, sizeof(unsigned char), 1, image_fd ) ;
  return x ;
}

unsigned short ssgSGIHeader::readShort ()
{
  unsigned short x ;
  fread ( & x, sizeof(unsigned short), 1, image_fd ) ;
  swab_short ( & x ) ;
  return x ;
}

unsigned int ssgSGIHeader::readInt ()
{
  unsigned int x ;
  fread ( & x, sizeof(unsigned int), 1, image_fd ) ;
  swab_int ( & x ) ;
  return x ;
}


void ssgSGIHeader::getRow ( unsigned char *buf, int y, int z )
{
  if ( y >= ysize ) y = ysize - 1 ;
  if ( z >= zsize ) z = zsize - 1 ;

  fseek ( image_fd, start [ z * ysize + y ], SEEK_SET ) ;

  if ( type == SGI_IMG_RLE )
  {
    unsigned char *tmpp = rle_temp ;
    unsigned char *bufp = buf ;

    fread ( rle_temp, 1, leng [ z * ysize + y ], image_fd ) ;

    unsigned char pixel, count ;

    while ( TRUE )
    {
      pixel = *tmpp++ ;

      count = ( pixel & 0x7f ) ;

      if ( count == 0 )
	break ;

      if ( pixel & 0x80 )
      {
        while ( count-- )
	  *bufp++ = *tmpp++ ;
      }
      else
      {
        pixel = *tmpp++ ;

	while ( count-- )
          *bufp++ = pixel ;
      }
    }
  }
  else
    fread ( buf, 1, xsize, image_fd ) ;
}


void ssgSGIHeader::getPlane ( unsigned char *buf, int z )
{
  if ( image_fd == NULL )
    return ;

  if ( z >= zsize ) z = zsize - 1 ;

  for ( int y = 0 ; y < ysize ; y++ )
    getRow ( & buf [ y * xsize ], y, z ) ;
}



void ssgSGIHeader::getImage ( unsigned char *buf )
{
  if ( image_fd == NULL )
    return ;

  for ( int y = 0 ; y < ysize ; y++ )
    for ( int z = 0 ; z < zsize ; z++ )
      getRow ( & buf [ ( z * ysize + y ) * xsize ], y, z ) ;
}


ssgSGIHeader::ssgSGIHeader ()
{
  dim   = 0 ;
  start = NULL ;
  leng  = NULL ;
  rle_temp = NULL ;
  image_fd = NULL ;
}

ssgSGIHeader::ssgSGIHeader ( const char *fname, ssgTextureInfo* info )
{
  ssgSGIHeader *sgihdr = this ;

  start = NULL ;
  leng = NULL ;

  bool success=openFile(fname);

  if ( !success )
  {
    loadSGI_bool = false ;
    return ;
  }

  GLubyte *image = new GLubyte [ sgihdr->xsize *
                                 sgihdr->ysize *
                                 sgihdr->zsize ] ;

  GLubyte *ptr = image ;

  unsigned char *rbuf =                     new unsigned char [ sgihdr->xsize ] ;
  unsigned char *gbuf = (sgihdr->zsize>1) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;
  unsigned char *bbuf = (sgihdr->zsize>2) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;
  unsigned char *abuf = (sgihdr->zsize>3) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;

  for ( int y = 0 ; y < sgihdr->ysize ; y++ )
  {
    int x ;

    switch ( sgihdr->zsize )
    {
      case 1 :
	sgihdr->getRow ( rbuf, y, 0 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	  *ptr++ = rbuf [ x ] ;

	break ;

      case 2 :
	sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	}
	break ;

      case 3 :
        sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;
	sgihdr->getRow ( bbuf, y, 2 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	}
	break ;

      case 4 :
        sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;
	sgihdr->getRow ( bbuf, y, 2 ) ;
	sgihdr->getRow ( abuf, y, 3 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	  *ptr++ = abuf [ x ] ;
	}
	break ;
    }
  }

  fclose ( image_fd ) ;
  image_fd = NULL ;

  delete [] rbuf   ;
  delete [] gbuf   ;
  delete [] bbuf   ;
  delete [] abuf   ;

  if ( info != NULL )
  {
    info -> width = sgihdr->xsize ;
    info -> height = sgihdr->ysize ;
    info -> depth = sgihdr->zsize ;
    info -> alpha = ( sgihdr->zsize == 4 ) ;
  }

  bool result =
    ssgMakeMipMaps ( image, sgihdr->xsize, sgihdr->ysize, sgihdr->zsize ) ;


  loadSGI_bool= result ;
}



ssgSGIHeader::~ssgSGIHeader()
{
  if (start != NULL)
    delete [] start;

  if (leng != NULL)
    delete [] leng;
  
  if (image_fd != NULL)
    fclose(image_fd);
}


void ssgSGIHeader::readHeader ()
{
  isSwapped = FALSE ;

  magic = readShort () ;

  if ( magic != SGI_IMG_MAGIC && magic != SGI_IMG_SWABBED_MAGIC )
  {
    ulSetError ( UL_FATAL, "%s: Unrecognised magic number 0x%04x",
                                         image_fname, magic ) ;
  }

  if ( magic == SGI_IMG_SWABBED_MAGIC )
  {
    isSwapped = TRUE ;
    swab_short ( & magic ) ;
  }

  type  = readByte  () ;
  bpp   = readByte  () ;
  dim   = readShort () ;

  /*
    This is a backstop test - if for some reason the magic number isn't swabbed, this
    test will still catch a swabbed file. Of course images with more than 256 dimensions
    are not catered for :-)
  */

  if ( dim > 255 )
  {
    ulSetError ( UL_WARNING, "%s: Bad swabbing?!?", image_fname ) ;
    isSwapped = ! isSwapped ;
    swab_short ( & dim ) ;
    magic = SGI_IMG_MAGIC ;
  }

  xsize = readShort () ;
  ysize = readShort () ;
  zsize = readShort () ;
  min   = readInt   () ;  
  max   = readInt   () ;  
                 readInt   () ;  /* Dummy field */

  int i ;

  for ( i = 0 ; i < 80 ; i++ )
    readByte () ;         /* Name field */

  colormap = readInt () ;

  for ( i = 0 ; i < 404 ; i++ )
    readByte () ;         /* Dummy field */

  makeConsistant () ;

  tablen = ysize * zsize ;
  start = new unsigned int [ tablen ] ;
  leng  = new int [ tablen ] ;
}

bool ssgSGIHeader::openFile(const char *fname)
{
  ssgSGIHeader *sgihdr=this;
  
  strcpy ( image_fname, fname ) ;
  image_fd = fopen ( image_fname, "rb" ) ;

  if ( image_fd == NULL )
  {
    /* perror ( "ssgLoadTexture" ) ;*/
    ulSetError ( UL_WARNING, "ssgSGIHeader::: Failed to open '%s' for reading.", image_fname ) ;
    return false ;
  }

  sgihdr -> readHeader () ;

  if ( sgihdr -> type == SGI_IMG_RLE )
  {
    fread ( sgihdr->start, sizeof (unsigned int), sgihdr->tablen, image_fd ) ;
    fread ( sgihdr->leng , sizeof (int), sgihdr->tablen, image_fd ) ;
    swab_int_array ( (int *) sgihdr->start, sgihdr->tablen ) ;
    swab_int_array ( (int *) sgihdr->leng , sgihdr->tablen ) ;

    int maxlen = 0 ;

    for ( int i = 0 ; i < sgihdr->tablen ; i++ )
      if ( sgihdr->leng [ i ] > maxlen )
        maxlen = sgihdr->leng [ i ] ;

    rle_temp = new unsigned char [ maxlen ] ;
  }
  else
  {
    rle_temp = NULL ;

    for ( int i = 0 ; i < sgihdr->zsize ; i++ )
      for ( int j = 0 ; j < sgihdr->ysize ; j++ )
      {
        sgihdr->start [ i * sgihdr->ysize + j ] = sgihdr->xsize * ( i * sgihdr->ysize + j ) + 512 ;
        sgihdr->leng  [ i * sgihdr->ysize + j ] = sgihdr->xsize ;
      }
  }

  if ( sgihdr->zsize <= 0 || sgihdr->zsize > 4 )
  {
    ulSetError ( UL_FATAL, "ssgLoadTexture: '%s' is corrupted.", image_fname ) ;
  }

  return true ;
} 


//wrapper function that does the same job as the previous one
//except we now have a better architecture

bool ssgLoadSGI ( const char *fname, ssgTextureInfo* info )
{
  ssgSGIHeader *sgihdr=new ssgSGIHeader( fname, info);
  bool returnval=sgihdr->loadSGI_bool;
  delete sgihdr;
  return returnval;
}


#else

  /*if there's no SGI support, allow things to compile, but print an error*/
ssgSGIHeader::ssgSGIHeader()
{
  ulSetError ( UL_WARNING,
    "ssgLoadTexture: '%s' - SGI support not configured", fname ) ;
  return;
}


ssgSGIHeader::ssgSGIHeader(const char *fname, ssgTextureInfo* info )
{
  ulSetError ( UL_WARNING,
    "ssgLoadTexture: '%s' - SGI support not configured", fname ) ;
  return;
}

void SGIHeader::makeConsistant (){return;} 
void SGIHeader::getRow   ( unsigned char *buf, int y, int z ) {return;}
void SGIHeader::getPlane ( unsigned char *buf, int z ) {return;}
void SGIHeader::getImage ( unsigned char *buf ) {return;}
void SGIHeader::readHeader () {return;}
void SGIHeader::swab_short ( unsigned short *x ){return;}
void SGIHeader::swab_int ( unsigned int *x ){return;}
void SGIHeader::swab_int_array ( int *x, int leng ){return;}
unsigned char SGIHeader::readByte (){return;}
unsigned short SGIHeader::readShort (){return;}
unsigned int SGIHeader::readInt (){return;}

bool ssgLoadSGI ( const char *fname, ssgTextureInfo* info )
{
  ulSetError ( UL_WARNING,
    "ssgLoadTexture: '%s' - SGI support not configured", fname ) ;
  return false ;
}

#endif
