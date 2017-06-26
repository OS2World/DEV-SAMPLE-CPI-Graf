#include <dos.h>

#define INCL_BASE
#include <os2.h>

#define NULL (void *)0

/*****************/

int EGA_base_adr;
extern setpxl(int,int,int);

char *EGA_font_descr;
extern write_text(int,int,char *,int);

char  *save_redraw_thread_stack;
unsigned    stack_seg;
#define     stack_size 4096

int    save_redraw_id;
extern save_redraw_thread();

char  *mode_thread_stack;
int    mode_id;
extern mode_thread();
/****************/
graf()

{
  VIOPHYSBUF   phys_buf;
           
  VIOMODEINFO  mode_info;
  
  VIOFONTINFO  font_info;
  
  /* setting up SaveRedrawWait-Thread */
  
  DosAllocSeg(stack_size,&stack_seg,0);
  FP_SEG(save_redraw_thread_stack) = stack_seg;
  FP_OFF(save_redraw_thread_stack) = stack_size - 2;
  
  DosCreateThread(save_redraw_thread,&save_redraw_id,save_redraw_thread_stack);
           
  /* setting up ModeWait-Thread */
  
  DosAllocSeg(stack_size,&stack_seg,0);
  FP_SEG(mode_thread_stack) = stack_seg;
  FP_OFF(mode_thread_stack) = stack_size - 2;
  
  DosCreateThread(mode_thread,&mode_id,mode_thread_stack);
           
  /* setting VGA-640*480 */

  mode_info.cb     = sizeof(VIOMODEINFO);
  VioGetMode(&mode_info,0);
  
  mode_info.fbType = VGMT_GRAPHICS | VGMT_OTHER;
  mode_info.color  = 4;
  mode_info.col    = 80;
  mode_info.row    = 30;
  mode_info.hres   = 640;
  mode_info.vres   = 480;
           
  VioSetMode(&mode_info,0);
  
  FP_SEG(phys_buf.pBuf) = 0x000a;
  FP_OFF(phys_buf.pBuf) = 0;
  phys_buf.cb           = 38400;
  
  VioGetPhysBuf(&phys_buf,0);
  EGA_base_adr = phys_buf.asel[0];
  
  {
    /* zeichne 480 Pixel lange Diagonale */
    /* vorher Loeschen des Schirms       */
    
    unsigned i;
    int   color;
    
    /* Bildschirm Loeschen */
    
    {
      char *buffer;
  
      FP_SEG(buffer) = phys_buf.asel[0];
    
      for (i = 0; i < 38400; i++)
      {
        FP_OFF(buffer) = i;
        *buffer = 0;
      }
    }
    
    /* Gerade Zeichnen */
    
    for (i = 0; i <= 479; i++)
    {
      color = i/30;
      setpxl(i,479 - i,color);
    }
  }

  font_info.cb     = sizeof(VIOFONTINFO);
  font_info.type   = VGFI_GETROMFONT;
  font_info.cxCell = 8;
  font_info.cyCell = 16;
  font_info.pbData = NULL;
  font_info.cbData = 0;
  
  VioGetFont(&font_info,0);
  EGA_font_descr = font_info.pbData;
    
  /* Textausgabe */
  {
    write_text(0,0,"Text links oben",3);
    write_text(20,15,"Text in der Mitte",5);
    write_text(60,29,"Text rechts unten",12);
  }
    
  
  scanf("ende");
  
  mode_info.cb     = sizeof(VIOMODEINFO);
  VioGetMode(&mode_info,0);
  
  mode_info.fbType = VGMT_OTHER;
  mode_info.color  = 4;
  mode_info.col    = 80;
  mode_info.row    = 25;
  mode_info.hres   = 720;
  mode_info.vres   = 400;
           
  VioSetMode(&mode_info,0);
           
  printf("handle was %d\n",phys_buf.asel[0]);
                                   
  printf("fertig!\n");
  
}
