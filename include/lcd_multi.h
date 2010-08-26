#ifndef __LCD_MULTI_H__
#define __LCD_MULTI_H__ "$Id$"

/*
 * lcd_multi.h
 *
 * This header file declares a set of interfaces for
 * supporting multiple displays on a given platform.
 *
 * Change History :
 *
 * $Log$
 *
 *
 * Copyright Boundary Devices, Inc. 2007
 */
#include "common.h"
#include "lcd_panels.h"
#include "bmp_layout.h"

struct lcd_t {
   void                     *fbAddr ;
   unsigned                  fbMemSize ;
   unsigned		     stride;
   struct lcd_panel_info_t   info ;
   unsigned                  x ;
   unsigned                  y ;
   unsigned                  colorCount ;
   unsigned 	             bg ;
   unsigned 	             fg ;

   void          (*set_palette)(unsigned long *colors, unsigned colorCount);
   unsigned long (*get_palette_color)(unsigned char idx);
   void          (*disable)(void);
   int		 (*display_bmp)(struct lcd_t *panel, bmp_image_t *bmp, int x, int y);
   void		 (*drawchar)(struct lcd_t *lcd,unsigned char const *c, unsigned w, unsigned h);
};

/*
 * This is called at the end of machine initialization to
 * initialize the set of panels with the 'panel' environment
 * variable.
 */
int lcd_multi_init(void);

/*
 * Use this at initialization time to construct and
 * assign panels. Note that each platform has an implicit
 * order of controllers (i.e. SM-501 first, then PXA), and
 * SM-501 supporting boards will allocate drivers based
 * on whether each panel matches a CRT or native LCD panel.
 *
 * Note that this routine is defined in a board specific
 * module to sort all that out:
 *    board/neon/neon.c
 *    board/halogen/halogen.c
 *    board/mercury/mercury.c
 *
 * This routine is called before the panel count is incremented,
 * so implementations may use getPanelCount() and getPanel() to
 * refer to prior instantiations.
 *
 */
struct lcd_t *newPanel( struct lcd_panel_info_t const *info );

/*
 * Internal call from cmd_lcdpanel.c
 */
struct lcd_t * addPanel(struct lcd_panel_info_t const *panel, const char *msg);

/*
 * After initialization, these routines should be used to
 * access the panels.
 *
 */
unsigned getPanelCount( void );
struct lcd_t *getPanel( unsigned idx );

/*
 * Used to disable all panels (usually to re-initialize them later)
 */
void disablePanels( void );

/*
 * The 'curpanel' command can be used to support commands that
 * haven't been plumbed for multiple panels.
 */
unsigned getCurrentPanel( void );
void setCurrentPanel( unsigned idx );

int display_bmp (bmp_image_t *bmp,int x, int y);

#endif

