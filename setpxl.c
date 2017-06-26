#include <dos.h>
#include <malloc.h>

#define INCL_SUB
#include <os2.h>


/*********************/
/* globale Variablen */
/*********************/

static int scr_lock;      /* Return Code von Screen-Lock */

static long *video_plane0;  /* savearea fuer videoplanes */
static long *video_plane1;  /* used for SaveRedrawWait   */
static long *video_plane2;
static long *video_plane3;

/*******************/
/*   EGA-Ports     */
/*******************/

#define EGA_control       0x3ce        /* Grafik-Controller Control-Port */
#define EGA_control_data  0x3cf        /* Grafik-Controller Daten-Port   */

#define EGA_read_map      0x04         /* Read-Map-Register im  */
                                       /* Grafik-Controller     */
#define EGA_mode          0x05         /* Mode-Register im      */
                                       /* Grafik-Controller     */
#define EGA_bit_mask      0x08         /* Bitmasken-Register im */
                                       /* Grafik-Controller     */

#define EGA_sequence      0x3c4        /* Sequencer  Control-Port */
#define EGA_sequence_data 0x3c5        /* Sequencer  Daten-Port   */

#define EGA_map_mask      0x02         /* Map-Mask-Register im */
                                       /* Sequencer            */
                                       
extern int   EGA_base_adr;
extern char *EGA_font_descr;
/******************/
/******************/
setpxl(x_koord,y_koord,color)

int x_koord, y_koord, color;

{
  char *p_pixel;
  int   bit_maske;
  int   dummy;

  /* Screen Lock holen */
  
  VioScrLock(LOCKIO_WAIT,&scr_lock,0);
      
  /* Pixeladresse berechnen */
      
  FP_SEG(p_pixel) = EGA_base_adr;
  FP_OFF(p_pixel) = y_koord*80 + x_koord/8;
      
  /* Bit-Maske berechnen */
      
  bit_maske = 0x80 >>(x_koord % 8);
      
  /* Setze Bitmasken-Register */
      
  outp(EGA_control,EGA_bit_mask);
  outp(EGA_control_data,bit_maske);
      
  /* folgende Befehlssequenz ist durch die Hardware bedingt */
      
  outp(EGA_control,EGA_mode);          /* setze Write-Mode 2 */
  outp(EGA_control_data,2);

  dummy = *p_pixel;                    /* lade Planes in Latch-Register */
  
  *p_pixel = color;                    /* setze Pixel */
      
  /* Setze alle Register zurueck */
      
  outp(EGA_control,EGA_mode);          /* default Mode   */
  outp(EGA_control_data,0);
  
  outp(EGA_control,EGA_bit_mask);      /* enable alle Bits   */
  outp(EGA_control_data,0xff);
  
  /* Screen wieder freigeben */
  
  VioScrUnLock(0);
      
}

/***********/

write_char(x_pos,y_pos,c,color)

int x_pos,y_pos,c,color;

{
  char *p_char;
  char *p_char_descr;
  int   i;
  int   dummy;
  
  /* Screen Lock holen */
  
  VioScrLock(LOCKIO_WAIT,&scr_lock,0);
      
  /* Adresse der Fontbeschreibung berechnen */
  
  p_char_descr = EGA_font_descr + 16*c;  /* Fontsize 8*16 */
  
  /* Adresse des 1. Bytes berechnen */
  
  FP_SEG(p_char) = EGA_base_adr;
  FP_OFF(p_char) = y_pos*80*16 +x_pos;   /* Fontsize 8*16 */
  
  /* setze alle Bytes */
  
  for (i = 0; i < 14; i++, p_char+=80, p_char_descr++)
  {
    dummy = *p_char;                    /* lade Planes in Latch Register */
    *p_char = 0;                        /* setze Pixel auf Hintergrund   */
    
    outp(EGA_control,EGA_bit_mask);     /* erlaube die zu setzenden Pixel */
    outp(EGA_control_data,*p_char_descr);
    
    outp(EGA_sequence,EGA_map_mask);    /* enable zu setzende Planes */
    outp(EGA_sequence_data,color);
    
    *p_char = 0xff;                     /* setze Pixel */

    /* Setze Alle Register zurueck */
      
    outp(EGA_sequence,EGA_map_mask);     /* enable alle Planes */
    outp(EGA_sequence_data,0xff);
  
    outp(EGA_control,EGA_bit_mask);      /* enable alle Bits   */
    outp(EGA_control_data,0xff);
  }
  
  /* Screen wieder freigeben */
  
  VioScrUnLock(0);
      
}

write_text(x_pos,y_pos,text,color)

int    x_pos,y_pos;
char *text;
int   color;

{
  while (*text)
  {
    write_char(x_pos,y_pos,*text,color);
    text++;
    x_pos++;
  }
}

/******************/

save_redraw_thread()

{
  int  save_redraw_cmd;  /* indicates, whether to save or to redraw screen */
  
  unsigned  count;
  long     *buffer;
  long      dummy;
  
  VIOMODEINFO  mode_info;   /* to save and restore video mode */
  
  unsigned  save_seg_adr;  /* Segmentadresse des Save-Buffers */

  /* allocate video plane buffers */
  
  DosAllocSeg(38400,&save_seg_adr,0);
  FP_SEG(video_plane0) = save_seg_adr;
  FP_OFF(video_plane0) = 0;
  
  DosAllocSeg(38400,&save_seg_adr,0);
  FP_SEG(video_plane1) = save_seg_adr;
  FP_OFF(video_plane1) = 0;
  
  DosAllocSeg(38400,&save_seg_adr,0);
  FP_SEG(video_plane2) = save_seg_adr;
  FP_OFF(video_plane2) = 0;
  
  DosAllocSeg(38400,&save_seg_adr,0);
  FP_SEG(video_plane3) = save_seg_adr;
  FP_OFF(video_plane3) = 0;
  
  while (1)              /* this thread continues forever */
  {
    VioSavRedrawWait(VSRWI_SAVEANDREDRAW,&save_redraw_cmd,0);

    if (save_redraw_cmd == VSRWN_SAVE)
    {
      /* save screen */
      
      mode_info.cb     = sizeof(VIOMODEINFO);
      VioGetMode(&mode_info,0);
  
      FP_SEG(buffer) = EGA_base_adr;
      
      /* Video Plane 0 */
      
      outp(EGA_control,EGA_read_map);    /* enable Plane 0 */
      outp(EGA_control_data,0);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        video_plane0[count>>2] = *buffer;
      }
      /* Video Plane 1 */
      
      outp(EGA_control,EGA_read_map);    /* enable Plane 1 */
      outp(EGA_control_data,1);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        video_plane1[count>>2] = *buffer;
      }
      /* Video Plane 2 */
      
      outp(EGA_control,EGA_read_map);    /* enable Plane 2 */
      outp(EGA_control_data,2);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        video_plane2[count>>2] = *buffer;
      }
      /* Video Plane 3 */
      
      outp(EGA_control,EGA_read_map);    /* enable Plane 3 */
      outp(EGA_control_data,3);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        video_plane3[count>>2] = *buffer;
      }
      
      /* Register ruecksetzen */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable alle Planes */
      outp(EGA_sequence_data,0xff);
    }
    else
    {
      PVIOPALSTATE palette_save;
      PVIOPALSTATE palette_tmp;
      
      /* redraw screen */

      VioSetMode(&mode_info,0);

      /* do invisible screen redraw */
      
      palette_save = (PVIOPALSTATE) malloc(sizeof(VIOPALSTATE)+30);
      palette_tmp  = (PVIOPALSTATE) malloc(sizeof(VIOPALSTATE)+30);
      
      palette_save->cb     = sizeof(VIOPALSTATE)+30;
      palette_save->type   = 0;   /* get palette registers    */
      palette_save->iFirst = 0;   /* starting with register 0 */
      
      VioGetState(palette_save,0);
      
      memcpy(palette_tmp,palette_save,palette_save->cb);
      
      for (count = 0; count < 16; count++)
        palette_tmp->acolor[count] = 0;
      
      VioSetState(palette_tmp,0);
      
      /* now do redraw */
      
      FP_SEG(buffer) = EGA_base_adr;
      
      /* Video Plane 0 */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable Plane 0 */
      outp(EGA_sequence_data,1);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        dummy = *buffer;
        *buffer = video_plane0[count>>2];
      }
      /* Video Plane 1 */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable Plane 1 */
      outp(EGA_sequence_data,2);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        dummy = *buffer;
        *buffer = video_plane1[count>>2];
      }
      /* Video Plane 2 */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable Plane 2 */
      outp(EGA_sequence_data,4);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        dummy = *buffer;
        *buffer = video_plane2[count>>2];
      }
      /* Video Plane 3 */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable Plane 3 */
      outp(EGA_sequence_data,8);
      
      for (count = 0; count < 38400; count+=4)
      {
        FP_OFF(buffer) = count;
        dummy = *buffer;
        *buffer = video_plane3[count>>2];
      }
      
      /* Register ruecksetzen */
      
      outp(EGA_sequence,EGA_map_mask);    /* enable alle Planes */
      outp(EGA_sequence_data,0xff);
      
      /* now make screen visible */
      
      VioSetState(palette_save,0);
      
    }
  }
}

/*****************/

mode_thread()

{
  int          mode_cmd;
  VIOMODEINFO  mode_info;

  while (1)                /* this thread continues forever */
  {
    VioModeWait(0,&mode_cmd,0);
    
    /* setting EGA-640*350 */

    mode_info.cb     = sizeof(VIOMODEINFO);
    VioGetMode(&mode_info,0);
  
    mode_info.fbType = VGMT_GRAPHICS | VGMT_OTHER;
    mode_info.color  = 4;
    mode_info.col    = 80;
    mode_info.row    = 25;
    mode_info.hres   = 640;
    mode_info.vres   = 350;
           
    VioSetMode(&mode_info,0);
  }
}
